#pragma once

#include "afxdialogex.h"

#include <optional>

#include "core/Crosshairs.h"

#include "Controls/Edit/SpinEdit/SpinEdit.h"
#include "Controls/ToolWindow/ToolWindow.h"

#include "UI/ActionEditors/InputEditor.h"

// Base view for mouse editing
class CMouseMovementEditorView : public InputEditor
{
protected:
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_VIEW_EDIT_MOUSE_MOVE };
#endif

	CMouseMovementEditorView();
	DECLARE_MESSAGE_MAP()

	void DoDataExchange(CDataExchange* pDX) override;
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedButtonMousePositionSelect();

protected: // InputEditor
	virtual void PostInit(const std::shared_ptr<IBaseInput>& baseInput) override;
	virtual std::shared_ptr<IBaseInput> TryFinishDialog() override;

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
	std::shared_ptr<Action> m_action;
};

// Editor of the cursor position
class CCursorPositionEditorView : public CMouseMovementEditorView
{
protected:
	CCursorPositionEditorView() = default;
	DECLARE_DYNCREATE(CCursorPositionEditorView)

protected: // InputEditor
	void PostInit(const std::shared_ptr<IBaseInput>& baseInput) override;
	std::shared_ptr<IBaseInput> TryFinishDialog() override;
};

// Editor of the mouse delta
class CMouseMoveEditorView : public CMouseMovementEditorView
{
protected:
	CMouseMoveEditorView() = default;
	DECLARE_DYNCREATE(CMouseMoveEditorView)

protected: // InputEditor
	void PostInit(const std::shared_ptr<IBaseInput>& baseInput) override;
	std::shared_ptr<IBaseInput> TryFinishDialog() override;
};
