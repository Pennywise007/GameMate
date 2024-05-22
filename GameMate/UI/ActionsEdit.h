#pragma once

#include "afxdialogex.h"

#include <chrono>

#include "core/Settings.h"

#include <Controls/Button/IconButton/IconButton.h>
#include <Controls/Edit/SpinEdit/SpinEdit.h>
#include <Controls/Tables/List/ListGroupCtrl/ListGroupCtrl.h>
#include <Controls/Tables/List/Widgets/SubItemsEditor/SubItemsEditor.h>

// Dialog to edit actions
template <class CBaseWindow = CWnd>
class CActionsEditBase : public CBaseWindow
{
	enum Columns {
		eDelay = 0,
		eAction
	};

	static inline constexpr UINT kTimerInterval1Sec = 1000;

	static inline constexpr UINT kRecordingTimer0Id = 0;
	static inline constexpr UINT kRecordingTimer1Id = 1;
	static inline constexpr UINT kRecordingTimer2Id = 2;

public:
	//CActionsEditBase(Actions a = {});
	template <typename... Args>
	CActionsEditBase(Args&&... args);

	enum { IDD = IDD_DIALOG_ACTIONS_EDIT };

protected:
	virtual void OnSettingsChanged();

	DECLARE_MESSAGE_MAP()

	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	virtual BOOL PreTranslateMessage(MSG* pMsg) override;
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnBnClickedButtonRemove();
	afx_msg void OnBnClickedButtonRecord();
	afx_msg void OnBnClickedButtonMoveUp();
	afx_msg void OnBnClickedButtonMoveDown();
	afx_msg void OnLvnItemchangedListActions(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEnChangeEditRandomizeDelays();

protected:
	void OnInit(Actions& actions, bool captureMousePositions);

private:
	void subscribeOnInputEvents();
	void unsubscribeFromInputEvents();
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
	int m_mouseMoveSubscriptionId = -1;
	Actions* m_actions = nullptr;
	bool m_captureMousePositions = false;
	std::optional<std::chrono::steady_clock::time_point> m_lastActionTime;
};

// Dialog to edit actions
class CActionsEditDlg : public CActionsEditBase<CDialogEx>
{
	DECLARE_DYNAMIC(CActionsEditDlg)
	CActionsEditDlg(CWnd* pParent, Actions& macros);

public:
	[[nodiscard]] static std::optional<Actions> ExecModal(CWnd* pParent, const Actions& macros);

protected:
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnInitDialog() override;
	void PreSubclassWindow() override;

private:
	Actions& m_actions;
};

// The same as CActionsEditDlg but allows to insert it to another dlg
class CActionsEditView : public CActionsEditBase<CFormView>
{
	DECLARE_DYNCREATE(CActionsEditView)

public:
	CActionsEditView();

	using OnSettingsChangedCallback = std::function<void()>;
	void Init(Actions& actions, OnSettingsChangedCallback callback);

protected:
	DECLARE_MESSAGE_MAP()
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

protected: // CActionsEditDlg
	void OnSettingsChanged() override;

private:
	OnSettingsChangedCallback m_onSettingsChangedCallback;
};

#include "ActionsEditBase.hpp"