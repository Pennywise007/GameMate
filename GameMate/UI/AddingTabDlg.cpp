#include "pch.h"
#include "GameMate.h"
#include "afxdialogex.h"
#include "AddingTabDlg.h"

namespace {

enum Mode {
	eNewTab = 0,
	eCopyTab
};

} // namespace

IMPLEMENT_DYNAMIC(CAddingTabDlg, CDialogEx)

CAddingTabDlg::CAddingTabDlg(CWnd* pParent, const TabConfiguration* configuration /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_ADDING_TAB, pParent)
{
	if (configuration)
		m_dialogResult = *configuration;
}

void CAddingTabDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_TAB_NAME, m_editName);
	DDX_Control(pDX, IDC_COMBO_COPY_TAB_SELECTION, m_comboboxCopySettings);
}

BEGIN_MESSAGE_MAP(CAddingTabDlg, CDialogEx)
END_MESSAGE_MAP()

BOOL CAddingTabDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_editName.SetCueBanner(L"Enter tab name...", TRUE);

	if (!m_dialogResult.tabName.empty())
	{
		// editing tab
		m_editName.SetWindowTextW(m_dialogResult.tabName.c_str());
		m_comboboxCopySettings.ShowWindow(SW_HIDE);
		GetDlgItem(IDC_STATIC_COPY_SETTINGS)->ShowWindow(SW_HIDE);

		SetWindowText(L"Rename tab");
	}

	const auto& tabs = ext::get_singleton<Settings>().tabs;
	if (tabs.empty())
	{
		m_comboboxCopySettings.SetWindowTextW(L"No tabs yet");
		m_comboboxCopySettings.EnableWindow(FALSE);
	}
	else
	{
		const auto newInd = m_comboboxCopySettings.AddString(L"Create new");
		for (const auto& tab : tabs)
		{
			m_comboboxCopySettings.AddString(tab->tabName.c_str());
		}
		m_comboboxCopySettings.SetCurSel(newInd);
	}

	return TRUE;
}

std::shared_ptr<TabConfiguration> CAddingTabDlg::ExecModal()
{
	if (CDialogEx::DoModal() != IDOK)
		return nullptr;

	return std::make_shared<TabConfiguration>(m_dialogResult);
}

void CAddingTabDlg::OnOK()
{
	CString tabName;
	m_editName.GetWindowTextW(tabName);

	if (tabName.IsEmpty())
	{
		MessageBox(L"Please enter a tab name", L"Tab name is empty", MB_ICONERROR);
		return;
	}

	const auto curSel = m_comboboxCopySettings.GetCurSel() - 1;
	if (curSel >= 0)
	{
		const auto& tabs = ext::get_singleton<Settings>().tabs;
		EXT_ASSERT(curSel < (int)tabs.size());
		m_dialogResult = *std::next(tabs.begin(), curSel)->get();
	}
	m_dialogResult.tabName = tabName;

	CDialogEx::OnOK();
}
