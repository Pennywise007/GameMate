#pragma once

#include "afxdialogex.h"

#include <optional>

#include "core/Crosshairs.h"

#include "Controls/Edit/SpinEdit/SpinEdit.h"
#include "Controls/ToolWindow/ToolWindow.h"

#include "UI/ActionEditors/ActionsEditor.h"

// Base view for mouse editing
class CMouseMovementEditorView : public ActionsEditor
{
protected:
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_VIEW_EDIT_MOUSE_MOVE };
#endif

	CMouseMovementEditorView();
	DECLARE_MESSAGE_MAP()

	void DoDataExchange(CDataExchange* pDX) override;
	virtual void OnInitialUpdate() override;
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedButtonMousePositionSelect();

protected: // ActionsEditor
	virtual void SetAction(const Action& action) override;
	virtual Action GetAction() override;

private:
	void updateMousePositionControlsStates();
	void syncCrosshairWindowWithCursor();
	void showProgramWindows(int nCmdShow);

protected:
	CButton m_buttonMousePositionSelect;
	CButton m_checkUseDirectInput;
	CSpinEdit m_editMousePositionX;
	CSpinEdit m_editMousePositionY;

protected:
	// Cursor position selection subscriptions
	int m_keyPressedSubscriptionId = -1;
	int m_mouseMoveSubscriptionId = -1;
	// Cursor image which will be drown on m_cursorReplacingWindow
	CToolWindow m_toolWindow;
	CBitmap m_cursorBitmap;
	process_toolkit::crosshair::CursorReplacingWindow m_cursorReplacingWindow;
	// Current action
	Action m_action;
};

// Editor of the cursor position
class CCursorPositionEditorView : public CMouseMovementEditorView
{
protected:
	CCursorPositionEditorView();
	DECLARE_DYNCREATE(CCursorPositionEditorView)

	void OnInitialUpdate() override;

protected: // ActionsEditor
	bool CanClose() const override;
	void SetAction(const Action& action) override;
};

// Editor of the mouse delta
class CMouseMoveEditorView : public CMouseMovementEditorView
{
protected:
	CMouseMoveEditorView() = default;
	DECLARE_DYNCREATE(CMouseMoveEditorView)

	void OnInitialUpdate() override;

protected: // ActionsEditor
	bool CanClose() const override;
	void SetAction(const Action& action) override;
	Action GetAction() override;
};
