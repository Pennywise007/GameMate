#pragma once

#include "afxdialogex.h"
#include "memory"

#include "Settings.h"

class CAddingTabDlg : private CDialogEx
{
	DECLARE_DYNAMIC(CAddingTabDlg)

public:
	CAddingTabDlg(CWnd* pParent, const TabConfiguration* configuration = nullptr);

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_ADDING_TAB };
#endif

public:
	// @brief Open modal dialog with user to add new tab
	// @returns TabConfiguration if user decided to create it
	std::shared_ptr<TabConfiguration> ExecModal();

protected:
	DECLARE_MESSAGE_MAP()

	virtual void OnOK();
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

private:
	CEdit m_editName;
	CComboBox m_comboboxCopySettings;

	TabConfiguration m_dialogResult;
};
