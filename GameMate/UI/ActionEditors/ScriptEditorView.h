#pragma once

#include "../ActionsEditor.h"

#include <Controls/MFC/CMFCEditBrowseCtrlEx.h>

class CScriptEditorView : public ActionsEditor
{
	DECLARE_DYNCREATE(CScriptEditorView)

protected:
	CScriptEditorView();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_VIEW_EDIT_SCRIPT_PATH };
#endif

	DECLARE_MESSAGE_MAP()
	void DoDataExchange(CDataExchange* pDX) override;

protected: // ActionsEditor
	bool CanClose() const override;
	void SetAction(const Action& action) override;
	Action GetAction() override;

protected:
	CMFCEditBrowseCtrlEx m_scriptPath;
};
