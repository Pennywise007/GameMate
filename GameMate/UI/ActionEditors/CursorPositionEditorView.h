#pragma once

#include "afxdialogex.h"

#include <optional>

#include "core/Crosshairs.h"

#include "Controls/Edit/SpinEdit/SpinEdit.h"
#include "Controls/ToolWindow/ToolWindow.h"

#include "../ActionsEditor.h"

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
	void SetAction(const Action& action) override;
	Action GetAction() override;

private:
	void updateMousePositionControlsStates();
	void syncCrosshairWindowWithCursor();

protected:
	CButton m_buttonMousePositionSelect;
	CButton m_checkUseDirectInput;
	CSpinEdit m_editMousePositionX;
	CSpinEdit m_editMousePositionY;

protected:
	Action m_action;
	int m_keyPressedSubscriptionId = -1;
	int m_mouseMoveSubscriptionId = -1;
	CToolWindow m_toolWindow;
	process_toolkit::crosshair::CursorReplacingWindow m_cursorReplacingWindow;
};

class CCursorPositionEditorView : public CMouseMovementEditorView
{
protected:
	CCursorPositionEditorView();
	DECLARE_DYNCREATE(CCursorPositionEditorView)

	void OnInitialUpdate() override;

protected: // ActionsEditor
	bool CanClose() const override;
};

class CMouseMoveEditorView : public CMouseMovementEditorView
{
protected:
	CMouseMoveEditorView();
	DECLARE_DYNCREATE(CMouseMoveEditorView)

	void OnInitialUpdate() override;

protected: // ActionsEditor
	void OnInitDone() override;
	bool CanClose() const override;
};
