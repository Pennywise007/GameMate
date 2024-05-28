#pragma once

#include "afxdialogex.h"

#include <optional>

#include "core/Crosshairs.h"

#include "Controls/Edit/SpinEdit/SpinEdit.h"
#include "Controls/ToolWindow/ToolWindow.h"

#include "../ActionsEditor.h"

class CCursorPositionEditorView : public ActionsEditor
{
protected:
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_VIEW_EDIT_MOUSE_MOVE };
#endif

	CCursorPositionEditorView();
	DECLARE_DYNCREATE(CCursorPositionEditorView)
	DECLARE_MESSAGE_MAP()

	void DoDataExchange(CDataExchange* pDX) override;
	void OnInitialUpdate() override;
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedButtonMousePositionSelect();

protected: // ActionsEditor
	bool CanClose() const override;
	void SetAction(const Action& action) override;
	Action GetAction() override;

private:
	void updateMousePositionControlsStates();
	void syncCrosshairWindowWithCursor();

protected:
	CButton m_buttonMousePositionSelect;
	CSpinEdit m_editMousePositionX;
	CSpinEdit m_editMousePositionY;

private:
	Action m_action;
	int m_keyPressedSubscriptionId = -1;
	int m_mouseMoveSubscriptionId = -1;
	CToolWindow m_toolWindow;
	process_toolkit::crosshair::CursorReplacingWindow m_cursorReplacingWindow;
};
