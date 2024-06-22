#pragma once

#include "UI/ActionEditors/InputEditor.h"

#include <Controls/MFC/CMFCEditBrowseCtrlEx.h>

class CScriptEditorView : public InputEditor
{
	DECLARE_DYNCREATE(CScriptEditorView)

protected:
	CScriptEditorView();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_VIEW_EDIT_SCRIPT_PATH };
#endif

	DECLARE_MESSAGE_MAP()
	void DoDataExchange(CDataExchange* pDX) override;

protected: // InputEditor
	void PostInit(const std::shared_ptr<IBaseInput>& baseInput) override;
	std::shared_ptr<IBaseInput> TryFinishDialog() override;

protected:
	CMFCEditBrowseCtrlEx m_scriptPath;

private:
	std::shared_ptr<Action> m_action;
};
