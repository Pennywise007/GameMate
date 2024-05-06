#pragma once

#include "afxdialogex.h"

#include <chrono>

#include "core/Settings.h"

#include <Controls/Button/IconButton/IconButton.h>
#include <Controls/Edit/SpinEdit/SpinEdit.h>
#include <Controls/Tables/List/ListGroupCtrl/ListGroupCtrl.h>
#include <Controls/Tables/List/Widgets/SubItemsEditor/SubItemsEditor.h>

// Dialog to edit actions
class CActionsEditDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CActionsEditDlg)
	CActionsEditDlg(CWnd* pParent, Actions& macros);   // standard constructor

public:
	[[nodiscard]] static std::optional<Actions> ExecModal(CWnd* pParent, const Actions& macros);

	enum { IDD = IDD_DIALOG_ACTIONS_EDIT };

protected:
	virtual void OnSettingsChanged();

	DECLARE_MESSAGE_MAP()

	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	virtual BOOL PreTranslateMessage(MSG* pMsg) override;
	virtual BOOL OnInitDialog() override;
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnBnClickedButtonRemove();
	afx_msg void OnBnClickedButtonRecord();
	afx_msg void OnBnClickedButtonMoveUp();
	afx_msg void OnBnClickedButtonMoveDown();
	afx_msg void OnLvnItemchangedListActions(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEnChangeEditRandomizeDelays();

private:
	void addAction(Action action);
	void updateButtonStates();

protected:
	controls::list::widgets::SubItemsEditor<CListGroupCtrl> m_listActions;
	CIconButton m_buttonRecord;
	CSpinEdit m_editRandomizeDelays;
	CStatic m_staticDelayHelp;
	CIconButton m_buttonAdd;
	CIconButton m_buttonMoveUp;
	CIconButton m_buttonMoveDown;
	CIconButton m_buttonDelete;

private:
	int m_keyPressedSubscriptionId = -1;
	Actions& m_macros;
	std::optional<std::chrono::steady_clock::time_point> m_lastActionTime;
};

// The same as CActionsEditDlg but allows to insert it to another dlg
class CActionsEditView : public CActionsEditDlg
{
	DECLARE_DYNAMIC(CActionsEditView)

public:
	using OnSettingsChangedCallback = std::function<void()>;
	CActionsEditView(CWnd* pParent, Actions& actions, OnSettingsChangedCallback callback);

	enum { IDD = IDD_DIALOG_ACTIONS_EDIT };
protected:
	DECLARE_MESSAGE_MAP()

	BOOL OnInitDialog() override;
	void PreSubclassWindow() override;
	void OnSettingsChanged() override;

private:
	const OnSettingsChangedCallback m_onSettingsChangedCallback;
};
