#include "pch.h"
#include "resource.h"

#include <filesystem>

#include <ext/std/filesystem.h>

#include "UI/ActionEditors/ScriptEditorView.h"

IMPLEMENT_DYNCREATE(CScriptEditorView, ActionsEditor)

BEGIN_MESSAGE_MAP(CScriptEditorView, ActionsEditor)
END_MESSAGE_MAP()

CScriptEditorView::CScriptEditorView()
	: ActionsEditor(IDD_VIEW_EDIT_SCRIPT_PATH)
{
}

void CScriptEditorView::DoDataExchange(CDataExchange* pDX)
{
	ActionsEditor::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MFCEDITBROWSE, m_scriptPath);
}

bool CScriptEditorView::CanClose() const
{
	CString text;
	m_scriptPath.GetWindowText(text);

	auto path = std::filesystem::get_exe_directory() / text.GetString();
	if (!std::filesystem::exists(path) && !std::filesystem::exists(text.GetString()) &&
		::MessageBox(m_hWnd, L"Script file '" + text + "' not found, continue?", L"Script file not found", MB_YESNO) == IDNO)
	{
		return false;
	}

	return true;
}

void CScriptEditorView::SetAction(const Action& action)
{
	m_scriptPath.SetWindowTextW(action.scriptPath.c_str());
}

Action CScriptEditorView::GetAction()
{
	CString text;
	m_scriptPath.GetWindowText(text);

	Action action;
	action.type = Action::Type::eRunScript;
	action.scriptPath = text.GetString();

	return action;
}
