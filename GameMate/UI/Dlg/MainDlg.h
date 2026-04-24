
#pragma once

#include <Controls/Button/IconButton/IconButton.h>
#include <Controls/TabControl/TabControl.h>
#include <Controls/TabControl/CustomDrawWidgets.h>

#include <core/events.h>

#include "UI/Dlg/TimerDlg.h"

class CMainDlg : public CDialogEx, ext::events::ScopeSubscription<ITimerNotifications, ISettingsChanged>
{
// Construction
public:
	CMainDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_MAIN };
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
	afx_msg void OnNcDestroy();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg UINT OnPowerBroadcast(UINT nID, LPARAM lParam);
	afx_msg void OnCbnSelchangeComboInputDriver();
	afx_msg void OnBnClickedMfcbuttonInputSimulatorInfo();
	afx_msg void OnBnClickedMfcbuttonMainSettings();
	afx_msg void OnBnClickedCheckTimer();
	afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);

private: // ITimerNotifications
	void OnShowHideTimer() override;
	void OnStartOrPauseTimer() override {}
	void OnResetTimer() override {}

private: // ISettiingsChangeg
    void OnSettingsChanged(ISettingsChanged::ChangedType changedMode) override;

private:
	void updateDriverInfoButton();
	void updateTimerButton();
	void restoreWindow();

private:
	HICON m_hIcon;
	CComboBox m_inputSimulator;
	CIconButton m_buttonInputSimulatorInfo;
	CButton m_buttonShowTimer;
	CIconButton m_buttonSetting;
	CButtonsTabCtrl<CTabControl> m_tabControlModes;
	// flag that app started in the hidden mode
	bool m_hideWindow = false;
	// previously active window before starting the app
	const HWND m_previouslyActiveWindow;

private:
	CTimerDlg m_timerDlg;
};
