#include "pch.h"
#include "afxdialogex.h"
#include "resource.h"

#include "AddingProcessToolkitDlg.h"

IMPLEMENT_DYNAMIC(CAddingProcessToolkitDlg, CDialogEx)

CAddingProcessToolkitDlg::CAddingProcessToolkitDlg(CWnd* pParent, const process_toolkit::ProcessConfiguration* configuration)
	: CDialogEx(IDD_DIALOG_ADD_PROCESS_TOOLKIT, pParent)
{
	if (configuration)
		m_configuration = *configuration;
}

void CAddingProcessToolkitDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_TAB_NAME, m_editName);
	DDX_Control(pDX, IDC_COMBO_COPY_TAB_SELECTION, m_comboboxCopySettings);
}

BEGIN_MESSAGE_MAP(CAddingProcessToolkitDlg, CDialogEx)
END_MESSAGE_MAP()

BOOL CAddingProcessToolkitDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_editName.SetCueBanner(L"Enter configuration name...", TRUE);

	if (!m_configuration.configurationName.empty())
	{
		// editing tab
		m_editName.SetWindowTextW(m_configuration.configurationName.c_str());
		m_comboboxCopySettings.ShowWindow(SW_HIDE);
		GetDlgItem(IDC_STATIC_COPY_SETTINGS)->ShowWindow(SW_HIDE);

		SetWindowText(L"Rename configuration");
	}

	const auto& configurations = ext::get_singleton<Settings>().process_toolkit.processConfigurations;
	if (configurations.empty())
	{
		m_comboboxCopySettings.SetWindowTextW(L"No configurations yet");
		m_comboboxCopySettings.EnableWindow(FALSE);
	}
	else
	{
		const auto newInd = m_comboboxCopySettings.AddString(L"Create new");
		for (const auto& tab : configurations)
		{
			m_comboboxCopySettings.AddString(tab->configurationName.c_str());
		}
		m_comboboxCopySettings.SetCurSel(newInd);
	}

	return TRUE;
}

void CAddingProcessToolkitDlg::OnOK()
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
		const auto& configurations = ext::get_singleton<Settings>().process_toolkit.processConfigurations;
		EXT_ASSERT(curSel < (int)configurations.size());
		m_configuration = *std::next(configurations.begin(), curSel)->get();
	}
	m_configuration.configurationName = tabName;

	CDialogEx::OnOK();
}

std::optional<process_toolkit::ProcessConfiguration> CAddingProcessToolkitDlg::ExecModal(
	CWnd* pParent, const process_toolkit::ProcessConfiguration* configuration)
{
	CAddingProcessToolkitDlg dlg(pParent, configuration);
	if (dlg.DoModal() != IDOK)
		return std::nullopt;

	return dlg.m_configuration;
}
