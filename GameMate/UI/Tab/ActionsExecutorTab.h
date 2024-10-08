#pragma once
#include "afxdialogex.h"

#include "core/events.h"
#include "core/Settings.h"

#include "UI/ActionEditors/EditActions.h"

#include <Controls/Edit/SpinEdit/SpinEdit.h>
#include <Controls/Button/IconButton/IconButton.h>

class CActionsExecutorTab : public CDialogEx, ext::events::ScopeSubscription<ISettingsChanged>
{
	DECLARE_DYNAMIC(CActionsExecutorTab)

public:
	CActionsExecutorTab(CWnd* pParent);

	enum { IDD = IDD_DIALOG_ACTIONS_EXECUTOR };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;
	virtual void OnOK() override;
	virtual void OnCancel() override;
	virtual BOOL OnInitDialog() override;
	afx_msg void OnEnChangeEditIntervalMin();
	afx_msg void OnEnChangeEditIntervalSec();
	afx_msg void OnEnChangeEditIntervalMillisec();
	afx_msg void OnEnChangeEditRepeatTimes();
	afx_msg void OnBnClickedRadioRepeatUntilStopped();
	afx_msg void OnBnClickedRadioRepeatTimes();
	afx_msg void OnBnClickedCheckEnabled();
	afx_msg void OnBnClickedMfcbuttonHotkey();

	DECLARE_MESSAGE_MAP()

private: // ISettingsChanged
	void OnSettingsChanged(ISettingsChanged::ChangedType changedType) override;

private:
	void UpdateEnableButtonText();
	void UpdateSettingsFromControl(CSpinEdit& edit, unsigned& setting);
	void EditActions();

private: // Controls
	CStatic m_actionsGroup;
	CActionsEditorView* m_actionsEditView = nullptr;

	CSpinEdit m_editIntervalMinutes;
	CSpinEdit m_editIntervalSeconds;
	CSpinEdit m_editIntervalMilliseconds;

	CButton m_radioRepeatUntilStop;
	CButton m_radioRepeatTimes;
	CSpinEdit m_editRepeatTimes;

	CButton m_buttonEnable;
	CIconButton m_buttonHotkey;
};
