﻿#include "pch.h"
#include "resource.h"
#include "GameMate.h"
#include "afxdialogex.h"
#include "MacrosEditDlg.h"
#include "ActionEditDlg.h"

#include <Controls/Tables/List/Widgets/SubItemsEditor/SubItemsEditor.h>
#include <Controls/Tooltip/ToolTip.h>

#include <ext/core/check.h>

using namespace controls::list::widgets;

namespace {

enum Columns {
	eDelay = 0,
	eAction
};

constexpr UINT kTimerInterval1Sec = 1000;

constexpr UINT kRecordingTimer0Id = 0;
constexpr UINT kRecordingTimer1Id = 1;
constexpr UINT kRecordingTimer2Id = 2;

} // namespace

IMPLEMENT_DYNAMIC(CMacrosEditDlg, CDialogEx)

CMacrosEditDlg::CMacrosEditDlg(const Macros& macros, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_MACROS_EDIT, pParent)
	, m_macros(macros)
{
}

std::optional<Macros> CMacrosEditDlg::ExecModal()
{
	if (DoModal() != IDOK)
		return std::nullopt;

	return m_macros;
}

void CMacrosEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_MACROSES, m_listMacroses);
	DDX_Control(pDX, IDC_BUTTON_RECORD, m_buttonRecord);
	DDX_Control(pDX, IDC_EDIT_RANDOMIZE_DELAYS, m_editRandomizeDelays);
	DDX_Control(pDX, IDC_STATIC_DELAY_HELP, m_staticDelayHelp);
}

BEGIN_MESSAGE_MAP(CMacrosEditDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CMacrosEditDlg::OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, &CMacrosEditDlg::OnBnClickedButtonRemove)
	ON_BN_CLICKED(IDC_BUTTON_RECORD, &CMacrosEditDlg::OnBnClickedButtonRecord)
	ON_WM_CLOSE()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_MACROSES, &CMacrosEditDlg::OnLvnItemchangedListMacroses)
	ON_WM_TIMER()
END_MESSAGE_MAP()

BOOL CMacrosEditDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CRect rect;
	m_staticDelayHelp.GetClientRect(rect);

	HICON icon;
	auto res = SUCCEEDED(::LoadIconWithScaleDown(NULL, IDI_INFORMATION, rect.Height(), rect.Height(), &icon));
	ASSERT(res);

	m_staticDelayHelp.ModifyStyle(0, SS_ICON);
	m_staticDelayHelp.SetIcon(icon);

	controls::SetTooltip(m_staticDelayHelp, L"Some game guards may check if you press buttons with the same delay and can ban you.\n"
		L"To avoid this you can set a percentage of the delay which will be applied to te ation delay randomly.\n"
		L"Formula: delay ± random(percentage)\n");

	m_listMacroses.GetClientRect(rect);

	constexpr int kDelayColumnWidth = 120;
	m_listMacroses.InsertColumn(Columns::eDelay, L"Delay(ms)", LVCFMT_CENTER, kDelayColumnWidth);
	m_listMacroses.InsertColumn(Columns::eAction, L"Action", LVCFMT_LEFT, rect.Width() - kDelayColumnWidth);
	
	LVCOLUMN colInfo;
	colInfo.mask = LVCF_FMT;
	m_listMacroses.GetColumn(Columns::eDelay, &colInfo);
	colInfo.fmt |= LVCFMT_FIXED_WIDTH | LVCFMT_CENTER;
	m_listMacroses.SetColumn(Columns::eDelay, &colInfo);
	
	m_listMacroses.SetProportionalResizingColumns({ Columns::eAction });

	m_listMacroses.setSubItemEditorController(Columns::eDelay,
		[](CListCtrl* pList, CWnd* parentWindow, const LVSubItemParams* pParams)
		{
			auto edit = std::make_shared<CEditBase>();

			edit->Create(SubItemEditorControllerBase::getStandartEditorWndStyle() | 
				ES_CENTER | CBS_AUTOHSCROLL,
				CRect(), parentWindow, 0);
			edit->UsePositiveDigitsOnly();
			edit->SetUseOnlyIntegersValue();

			CString curSubItemText = pList->GetItemText(pParams->iItem, pParams->iSubItem);
			edit->SetWindowTextW(curSubItemText);

			return std::shared_ptr<CWnd>(edit);
		},
		[&](CListCtrl* pList, CWnd* editorControl, const LVSubItemParams* pParams, bool bAcceptResult)
		{
			if (!bAcceptResult)
				return false;

			CString currentEditorText;
			editorControl->GetWindowTextW(currentEditorText);

			auto it = std::next(m_macros.actions.begin(), pParams->iItem);
			it->delayInMilliseconds = _wtoi(currentEditorText);

			return true;
		});
	m_listMacroses.setSubItemEditorController(Columns::eAction,
		[&](CListCtrl* pList, CWnd* parentWindow, const LVSubItemParams* pParams)
		{
			auto action = std::next(m_macros.actions.begin(), pParams->iItem);

			auto newAction = CActionEditDlg::EditMacros(this, *action);
			if (newAction.has_value())
			{
                *action = std::move(*newAction);
                m_listMacroses.SetItemText(pParams->iItem, Columns::eAction, action->ToString().c_str());
            }

			return nullptr;
		});

	m_editRandomizeDelays.UsePositiveDigitsOnly();
	m_editRandomizeDelays.SetMinMaxLimits(0, 100);

	std::wostringstream str;
	str << m_macros.randomizeDelays;
	m_editRandomizeDelays.SetWindowTextW(str.str().c_str());

	decltype(m_macros.actions) actionsCopy;
	std::swap(actionsCopy, m_macros.actions);
	for (auto&& action : actionsCopy)
	{
		addAction(std::move(action));
	}

	m_buttonRecord.SetIcon(IDI_ICON_START_RECORDING, Alignment::LeftCenter);

	return TRUE;
}

BOOL CMacrosEditDlg::PreTranslateMessage(MSG* pMsg)
{
	if (m_lastActionTime.has_value())
	{
		switch (pMsg->message)
		{
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		{
			// Process stop recording and OK click
			if (pMsg->hwnd == m_buttonRecord.m_hWnd ||
				pMsg->hwnd == GetDlgItem(IDOK)->m_hWnd)
				break;

			[[fallthrough]];
		}
		default:
		{
			auto curTime = std::chrono::steady_clock::now();
			auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(curTime - *m_lastActionTime).count();

			auto action = MacrosAction::GetMacrosActionFromMessage(pMsg, delay);
			if (action.has_value())
			{
				addAction(std::move(*action));
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

void CMacrosEditDlg::addAction(MacrosAction&& action)
{
	std::vector<int> selectedActions;
	m_listMacroses.GetSelectedList(selectedActions, true);

	int item;
	if (selectedActions.empty())
		item = m_listMacroses.GetItemCount();
	else
		item = selectedActions.back() + 1;

	m_macros.actions.insert(std::next(m_macros.actions.begin(), item), std::move(action));

	item = m_listMacroses.InsertItem(item, std::to_wstring(action.delayInMilliseconds).c_str());
	m_listMacroses.SetItemText(item, Columns::eAction, action.ToString().c_str());
	m_listMacroses.SelectItem(item);
}

void CMacrosEditDlg::OnBnClickedButtonAdd()
{
	std::optional<MacrosAction> action = CActionEditDlg::EditMacros(this);
	if (!action.has_value())
		return;

	addAction(std::move(*action));
}

void CMacrosEditDlg::OnBnClickedButtonRemove()
{
	std::vector<int> selectedActions;
	m_listMacroses.GetSelectedList(selectedActions, true);

	for (auto it = selectedActions.rbegin(), end = selectedActions.rend(); it != end; ++it)
	{
		m_macros.actions.erase(std::next(m_macros.actions.begin(), *it));
		m_listMacroses.DeleteItem(*it);
	}
	m_listMacroses.SelectItem(selectedActions.back() - selectedActions.size() + 1);
}

void CMacrosEditDlg::OnBnClickedButtonRecord()
{
	KillTimer(kRecordingTimer0Id);
	KillTimer(kRecordingTimer1Id);
	KillTimer(kRecordingTimer2Id);

	if (m_buttonRecord.GetCheck() == BST_CHECKED)
	{
		SetTimer(kRecordingTimer0Id, kTimerInterval1Sec, nullptr);
		m_buttonRecord.SetWindowTextW(L"Record will start in 3...");
		m_buttonRecord.SetIcon(IDI_ICON_STOP_RECORDING, Alignment::LeftCenter);
	}
	else
	{
		m_buttonRecord.SetWindowTextW(L"Record actions");
		m_buttonRecord.SetIcon(IDI_ICON_START_RECORDING, Alignment::LeftCenter);
		m_lastActionTime.reset();
	}
}

void CMacrosEditDlg::OnTimer(UINT_PTR nIDEvent)
{
	KillTimer(nIDEvent);

	switch (nIDEvent)
	{
	case kRecordingTimer0Id:
		SetTimer(kRecordingTimer1Id, kTimerInterval1Sec, nullptr);
		m_buttonRecord.SetWindowTextW(L"Record will start in 2...");
		break;
	case kRecordingTimer1Id:
		SetTimer(kRecordingTimer2Id, kTimerInterval1Sec, nullptr);
		m_buttonRecord.SetWindowTextW(L"Record will start in 1...");
		break;
	case kRecordingTimer2Id:
		m_buttonRecord.SetWindowTextW(L"Cancel recording");
		m_lastActionTime = std::chrono::steady_clock::now();
		break;
	default:
		EXT_ASSERT(false) << "Unknown event id " << nIDEvent;
		break;
	}

	CDialogEx::OnTimer(nIDEvent);
}

void CMacrosEditDlg::OnClose()
{
	CString text;
	m_editRandomizeDelays.GetWindowTextW(text);

	std::wistringstream str(text.GetString());
	str >> m_macros.randomizeDelays;

	CDialogEx::OnClose();
}

void CMacrosEditDlg::OnLvnItemchangedListMacroses(NMHDR* pNMHDR, LRESULT* pResult)
{
	GetDlgItem(IDC_BUTTON_REMOVE)->EnableWindow(m_listMacroses.GetSelectedCount() > 0);
	*pResult = 0;
}
