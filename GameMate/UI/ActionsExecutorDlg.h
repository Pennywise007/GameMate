#pragma once
#include "afxdialogex.h"

#include "core/Settings.h"

#include <Controls/Edit/SpinEdit/SpinEdit.h>
#include <Controls/Button/IconButton/IconButton.h>
#include <Controls/Tables/List/ListGroupCtrl/ListGroupCtrl.h>

class CActionsExecutorDlg : public CDialogEx, ext::events::ScopeSubscription<ISettingsChanged>
{
	DECLARE_DYNAMIC(CActionsExecutorDlg)

public:
	CActionsExecutorDlg(CWnd* pParent = nullptr);   // standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_ACTIONS_EXECUTOR };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnEnChangeEditIntervalMin();
	afx_msg void OnEnChangeEditIntervalSec();
	afx_msg void OnEnChangeEditIntervalMillisec();
	afx_msg void OnEnChangeEditRepeatTimes();
	afx_msg void OnBnClickedRadioRepeatUntilStopped();
	afx_msg void OnBnClickedRadioRepeatTimes();
	afx_msg void OnBnClickedCheckEnabled();
	afx_msg void OnBnClickedMfcbuttonHotkey();
	afx_msg void OnNMClickListActions(NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()

private: // ISettingsChanged
	void OnSettingsChanged() override;

private:
	void UpdateEnableButtonText();
	void UpdateSettingsFromControl(CSpinEdit& edit, unsigned& setting);
	void EditActions();
	void UpdateActions();

private: // Controls
	CListGroupCtrl m_listActions;

	CSpinEdit m_editIntervalMinutes;
	CSpinEdit m_editIntervalSeconds;
	CSpinEdit m_editIntervalMilliseconds;

	CButton m_radioRepeatUntilStop;
	CButton m_radioRepeatTimes;
	CSpinEdit m_editRepeatTimes;

	CButton m_buttonEnable;
	CIconButton m_buttonHotkey;
};
