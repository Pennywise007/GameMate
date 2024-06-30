
#pragma once

#include <Controls/Button/IconButton/IconButton.h>
#include <Controls/TabControl/TabControl.h>
#include <Controls/TabControl/CustomDrawWidgets.h>

#include "UI/Dlg/TimerDlg.h"

class CMainDlg : public CDialogEx, ext::events::ScopeSubscription<ITimerNotifications>
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
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnTcnSelchangeTabcontrolGames(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCbnSelchangeComboInputDriver();
	afx_msg void OnBnClickedMfcbuttonInputSimulatorInfo();
	afx_msg void OnBnClickedMfcbuttonTimerHotkey();
	afx_msg void OnBnClickedCheckTimer();

private: // ITimerNotifications
	void OnShowHideTimer() override;
	void OnStartOrPauseTimer() override {}
	void OnResetTimer() override {}

private:
	void updateDriverInfoButton();
	void updateTimerButton();

private:
	HICON m_hIcon;
	CComboBox m_inputSimulator;
	CIconButton m_buttonInputSimulatorInfo;
	CButton m_buttonShowTimer;
	CIconButton m_buttonShowTimerHotkey;
	CButtonsTabCtrl<CTabControl> m_tabControlModes;

private:
	CTimerDlg m_timerDlg;
};
