#pragma once

#include "afxdialogex.h"

#include <chrono>
#include <list>

#include "core/Settings.h"

#include <ext/thread/thread.h>

#include <Controls/Button/IconButton/IconButton.h>
#include <Controls/Edit/SpinEdit/SpinEdit.h>
#include <Controls/Tables/List/ListGroupCtrl/ListGroupCtrl.h>
#include <Controls/Tables/List/Widgets/SubItemsEditor/SubItemsEditor.h>

// Dialog to edit actions
class CActionsEditorView : public CFormView
{
	enum Columns {
		eDelay = 0,
		eAction
	};

	static inline constexpr UINT kTimerInterval1Sec = 1000;

	static inline constexpr UINT kRecordingTimer0Id = 0;
	static inline constexpr UINT kRecordingTimer1Id = 1;
	static inline constexpr UINT kRecordingTimer2Id = 2;

	DECLARE_DYNCREATE(CActionsEditorView)
	CActionsEditorView();

public:
	using OnSettingsChangedCallback = std::function<void()>;
	void Init(Actions& actions, OnSettingsChangedCallback callback, bool captureMousePositions);

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_VIEW_ACTIONS_EDITOR };
#endif

protected:
	DECLARE_MESSAGE_MAP()

	void DoDataExchange(CDataExchange* pDX) override;
	BOOL PreCreateWindow(CREATESTRUCT& cs) override;
	BOOL PreTranslateMessage(MSG* pMsg) override;
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnBnClickedButtonRemove();
	afx_msg void OnBnClickedButtonRecord();
	afx_msg void OnBnClickedButtonMoveUp();
	afx_msg void OnBnClickedButtonMoveDown();
	afx_msg void OnLvnItemchangedListActions(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEnChangeEditRandomizeDelays();
	afx_msg void OnBnClickedCheckUniteMovements();

private:
	void startRecording();
	void stopRecoring();
	void addActions(const std::list<Action>& actions);
	void addAction(Action&& action);
	int addAction(int item, Action action);
	void updateButtonStates();
	void onSettingsChanged();

protected:
	controls::list::widgets::SubItemsEditor<CListGroupCtrl> m_listActions;
	CIconButton m_buttonRecord;
	CSpinEdit m_editRandomizeDelays;
	CStatic m_staticDelayHelp;
	CIconButton m_buttonAdd;
	CIconButton m_buttonMoveUp;
	CIconButton m_buttonMoveDown;
	CIconButton m_buttonDelete;
	CButton m_checkUniteMouseMovements;
	CComboBox m_comboRecordMode;

private:
	int m_keyPressedSubscriptionId = -1;
	int m_mouseMoveSubscriptionId = -1;
	Actions* m_actions = nullptr;
	std::optional<std::chrono::steady_clock::time_point> m_lastActionTime;
	OnSettingsChangedCallback m_onSettingsChangedCallback;
	bool m_showMovementsTogether = false;

private: // we add recorded actions after finishing recording just to avoid any delays during actions processing
	std::list<Action> m_recordedActions;
};

// Dialog to edit actions
class CActionsEditDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CActionsEditDlg)
	CActionsEditDlg(CWnd* pParent, Actions& macros);

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_EDIT_ACTIONS };
#endif

public:
	[[nodiscard]] static std::optional<Actions> ExecModal(CWnd* pParent, const Actions& macros);

protected:
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnInitDialog() override;

private:
	Actions& m_actions;
	CActionsEditorView* m_editorView = nullptr;
};
