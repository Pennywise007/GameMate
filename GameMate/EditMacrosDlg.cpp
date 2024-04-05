#include "pch.h"
#include "GameMate.h"
#include "afxdialogex.h"
#include "EditMacrosDlg.h"
#include "BindEdit.h"

#include <ext/core/check.h>

namespace {

enum Columns {
	eDelay = 0,
	eAction
};

} // namespace

IMPLEMENT_DYNAMIC(CEditMacrosDlg, CDialogEx)

CEditMacrosDlg::CEditMacrosDlg(const std::list<Macros::Action>& actions, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_EDIT_MACROS, pParent)
	, m_actions(actions)
{
}

void CEditMacrosDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_MACROSES, m_listMacroses);
	DDX_Control(pDX, IDC_BUTTON_RECORD, m_buttonRecord);
}

BEGIN_MESSAGE_MAP(CEditMacrosDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CEditMacrosDlg::OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, &CEditMacrosDlg::OnBnClickedButtonRemove)
	ON_BN_CLICKED(IDC_BUTTON_RECORD, &CEditMacrosDlg::OnBnClickedButtonRecord)
END_MESSAGE_MAP()

BOOL CEditMacrosDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CRect rect;
	m_listMacroses.GetClientRect(rect);

	constexpr int kDelayColumnWidth = 100;
	m_listMacroses.InsertColumn(Columns::eDelay, L"Delay(ms)", LVCFMT_CENTER, kDelayColumnWidth);
	m_listMacroses.InsertColumn(Columns::eAction, L"Action", LVCFMT_LEFT, rect.Width() - kDelayColumnWidth);
	
	LVCOLUMN colInfo;
	colInfo.mask = LVCF_FMT;
	m_listMacroses.GetColumn(Columns::eDelay, &colInfo);
	colInfo.fmt |= LVCFMT_FIXED_WIDTH | LVCFMT_CENTER;
	m_listMacroses.SetColumn(Columns::eDelay, &colInfo);
	
	m_listMacroses.SetProportionalResizingColumns({ Columns::eAction });

	for (const auto& action : m_actions)
	{
		addActionToTable(action);
	}

	return TRUE;
}

BOOL CEditMacrosDlg::PreTranslateMessage(MSG* pMsg)
{
	if (m_buttonRecord.GetCheck() == BST_CHECKED)
	{
		switch (pMsg->message)
		{
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		{
			// Ignore stop recording click
			CRect rect;
			m_buttonRecord.GetWindowRect(&rect);

			POINT cursorPoint;
			GetCursorPos(&cursorPoint);

			if (rect.PtInRect(cursorPoint))
				break;

			[[fallthrough]];
		}
		default:
		{
			auto curTime = std::chrono::steady_clock::now();

			auto action = Macros::Action::GetActionFromMessage(pMsg, std::chrono::duration_cast<std::chrono::milliseconds>(curTime - m_lastActionTime).count());
			if (action.has_value())
			{
				addActionToTable(*action);
				m_actions.emplace_back(std::move(*action));
				m_lastActionTime = std::move(curTime);
				return TRUE;
			}
			break;
		}
		}
	}

	switch (pMsg->message)
	{
	case WM_KEYDOWN:
	{
		switch (pMsg->wParam)
		{
		case VK_RETURN:
		case VK_ESCAPE:
			// Don't close window on enter and excape
			return TRUE;
		}
		break;
	}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CEditMacrosDlg::addActionToTable(const Macros::Action& action, int ind)
{
	if (ind == -1)
		ind = m_listMacroses.GetItemCount();

	int i = m_listMacroses.InsertItem(ind, std::to_wstring(action.delay).c_str());
	m_listMacroses.SetItemText(i, Columns::eAction, action.ToString().c_str());

	m_listMacroses.EnsureVisible(i, FALSE);
}

void CEditMacrosDlg::OnBnClickedButtonAdd()
{
	std::optional<Macros::Action> action = CBindEdit(this).ExecModal();
	if (!action.has_value())
		return;

	std::vector<int> selectedActions;
	m_listMacroses.GetSelectedList(selectedActions);

	if (selectedActions.empty())
	{
		addActionToTable(*action);
		m_actions.emplace_back(std::move(*action));
	}
	else
	{
		addActionToTable(*action, selectedActions.back());
		m_actions.insert(std::next(m_actions.begin(), selectedActions.back()), std::move(*action));
	}
}

void CEditMacrosDlg::OnBnClickedButtonRemove()
{
	std::vector<int> selectedActions;
	m_listMacroses.GetSelectedList(selectedActions);

	for (auto it = selectedActions.rbegin(), end = selectedActions.rend(); it != end; ++it)
	{
		m_actions.erase(std::next(m_actions.begin(), *it));
		m_listMacroses.DeleteItem(*it);
	}
}

void CEditMacrosDlg::OnBnClickedButtonRecord()
{
	if (m_buttonRecord.GetCheck() == BST_CHECKED)
	{
		m_lastActionTime = std::chrono::steady_clock::now();
	}
}
