#pragma once

#include "afxdialogex.h"
#include <optional>

#include "core/Settings.h"

class CAddingProcessToolkitDlg : private CDialogEx
{
	DECLARE_DYNAMIC(CAddingProcessToolkitDlg)

public:
	CAddingProcessToolkitDlg(CWnd* pParent, const process_toolkit::ProcessConfiguration* m_selectedConfigurationName);

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_ADD_PROCESS_TOOLKIT };
#endif

public:
	// @brief Open modal dialog with user to add new tab
	// @returns process_toolkit::ProcessConfiguration if user decided to create it
	static std::optional<process_toolkit::ProcessConfiguration> ExecModal(
		CWnd* pParent, const process_toolkit::ProcessConfiguration* configuration = nullptr);

protected:
	DECLARE_MESSAGE_MAP()

	virtual void OnOK();
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

private:
	CEdit m_editName;
	CComboBox m_comboboxCopySettings;

private:
	const bool m_editingTabNameDialog;
	process_toolkit::ProcessConfiguration m_configuration;
};
