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

class ActionsTableSubItemEditorController;

// Dialog to edit actions
class CActionsEditorView : public CFormView
{
	friend class ActionsTableSubItemEditorController;

	DECLARE_DYNCREATE(CActionsEditorView)
	CActionsEditorView();

public:
	using OnSettingsChangedCallback = std::function<void()>;
	void Init(Actions& actions, OnSettingsChangedCallback callback);

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
	afx_msg void OnBnClickedCheckUniteMovements();
	afx_msg void OnBnClickedCheckRandomizeDelay();
	afx_msg void OnBnClickedButtonEditDelay();
	afx_msg void OnCbnSelendokComboRecordMode();
	afx_msg void OnLvnItemchangedListActions(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEnChangeEditRandomizeDelays();

private:
	void switchRandomizeDelayMode();
	void startRecording();
	void stopRecoring();
	bool canDynamicallyAddRecordedActions() const;
	void handleRecordedActionAsync(Action&& action);
	void addActions(const std::list<Action>& actions, bool lockRedraw);
	void addAction(Action action);
	int addAction(int item, Action action);
	void updateButtonStates();
	void copyItemsToClipboard();
	void pasteItemsFromClipboard();
	void onSettingsChanged(bool changesInActionsList);

protected:
	CIconButton m_buttonRecord;
	CButton m_checkUniteMouseMovements;
	CComboBox m_comboMouseRecordMode;
	CStatic m_staticMouseMoveHelp;
	CIconButton m_buttonAdd;
	CIconButton m_buttonDelete;
	CIconButton m_buttonMoveUp;
	CIconButton m_buttonMoveDown;
	CIconButton m_buttonEditDelay;
	controls::list::widgets::SubItemsEditor<CListGroupCtrl> m_listActions;
	CButton m_checkRandomizeDelay;
	CSpinEdit m_editRandomizeDelays;
	CStatic m_staticDelayHelp;

private:
	// Input events subscriptions
	int m_keyPressedSubscriptionId = -1;
	int m_mouseMoveSubscriptionId = -1;
	int m_mouseMoveDirectXSubscriptionId = -1;
	// Editable actions settings
	Actions* m_actions = nullptr;
	OnSettingsChangedCallback m_onSettingsChangedCallback;

private: // we add recorded actions after finishing recording just to avoid any delays during actions processing
	std::optional<std::chrono::steady_clock::time_point> m_lastActionTime;
	std::list<Action> m_recordedActions;
	std::mutex m_recordingActionsMutex;

public:
	bool m_ignoreCheckChanged = false;
	bool m_ignoreSelectionChanged = false;
	int m_columnDelayIndex = 0;
	int m_columnActionIndex = 1;
	const std::shared_ptr<ActionsTableSubItemEditorController> m_actionsTableSubItemEditor;
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
	virtual void DoDataExchange(CDataExchange* pDX) override;
	virtual BOOL OnInitDialog() override;
	virtual void OnOK() override;

private:
	Actions& m_actions;
	CEdit m_editDescription;
};

using controls::list::widgets::LVSubItemParams;
using controls::list::widgets::SubItemEditorControllerBase;

class ActionsTableSubItemEditorController : public SubItemEditorControllerBase
{
public:
	ActionsTableSubItemEditorController(CActionsEditorView* editorView) : m_editorView(editorView) {}

// ISubItemEditorController
public:
	std::shared_ptr<CWnd> createEditorControl(CListCtrl* pList, CWnd* parentWindow,	const LVSubItemParams* pParams) override;
	void onEndEditSubItem(CListCtrl* pList, CWnd* editorControl, const LVSubItemParams* pParams, bool bAcceptResult) override;

public:
	CActionsEditorView* const m_editorView;
};
