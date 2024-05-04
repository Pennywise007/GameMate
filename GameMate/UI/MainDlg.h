
#pragma once

#include <Controls/TabControl/TabControl.h>

#include "core/Settings.h"

#include <ext/thread/scheduler.h>

class CMainDlg : public CDialogEx, ext::events::ScopeSubscription<ISettingsChanged>
{
// Construction
public:
	CMainDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MAIN_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

// Implementation
protected:
	DECLARE_MESSAGE_MAP()

	// Generated message map functions
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnPaint();
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedButtonAddTab();
	afx_msg void OnBnClickedButtonRenameTab();
	afx_msg void OnBnClickedButtonDeleteTab();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnTcnSelchangeTabcontrolGames(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedCheckProgramWorking();
	afx_msg void OnCbnSelchangeComboInputDriver();
	afx_msg void OnBnClickedMfcbuttonInputDriverInfo();

private: // ISettingsChanged
	void OnSettingsChanged() override;

private:
	int AddTab(const std::shared_ptr<TabConfiguration>& tabSettings);
	void OnGamesTabChanged();
	void UpdateProgramWorkingButton();
	void UpdateDriverInfoButton();

private:
	HICON m_hIcon;
	CTabControl m_tabControlGames;
	CButton m_checkProgramWorking;
public:
	CComboBox m_inputDriver;
	CMFCButton m_buttonInputDriverInfo;
};
