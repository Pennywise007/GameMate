#pragma once

#include "afxdialogex.h"

#include "core/Actions.h"

class MainSettingsDlg : public CDialogEx
{
	DECLARE_DYNAMIC(MainSettingsDlg)

public:
	MainSettingsDlg(CWnd* pParent = nullptr);
	~MainSettingsDlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_MAIN_SETTINGS };
#endif

protected:
	DECLARE_MESSAGE_MAP()

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnBnClickedButtonChageShowTimerBind();
	afx_msg void OnBnClickedButtonChangeActiveProcessToolkitBind();
	afx_msg void OnBnClickedButtonChangeActionsExecutorBind();

private:
	bool IsRunAtStartupEnabled() const;
	void ApplySettings() const;

private:
	CButton m_checkboxRunOnStartup;
	CButton m_checkboxEnableTraces;
	CStatic m_staticShowTimerBind;
	CStatic m_staticActiveProcessToolkitBind;
	CStatic m_staticActionsExecutorBind;

private:
	Bind m_bindShowTimer;
	Bind m_bindActiveProcessToolkit;
	Bind m_bindActionsExecutor;
};
