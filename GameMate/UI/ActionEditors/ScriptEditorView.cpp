#include "pch.h"
#include "resource.h"

#include <filesystem>

#include <ext/std/filesystem.h>

#include "UI/ActionEditors/ScriptEditorView.h"

IMPLEMENT_DYNCREATE(CScriptEditorView, InputEditor)

BEGIN_MESSAGE_MAP(CScriptEditorView, InputEditor)
END_MESSAGE_MAP()

CScriptEditorView::CScriptEditorView()
	: InputEditor(IDD_VIEW_EDIT_SCRIPT_PATH)
{
}

void CScriptEditorView::DoDataExchange(CDataExchange* pDX)
{
	InputEditor::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MFCEDITBROWSE, m_scriptPath);
}

void CScriptEditorView::PostInit(const std::shared_ptr<IBaseInput>& baseInput)
{
	m_action = std::dynamic_pointer_cast<Action>(baseInput);
	m_scriptPath.SetWindowTextW(m_action->scriptPath.c_str());
}

std::shared_ptr<IBaseInput> CScriptEditorView::TryFinishDialog()
{
	CString text;
	m_scriptPath.GetWindowText(text);

	auto path = std::filesystem::get_exe_directory() / text.GetString();
	if (!std::filesystem::exists(path) && !std::filesystem::exists(text.GetString()) &&
		::MessageBox(m_hWnd, L"Script file '" + text + "' not found, continue?", L"Script file not found", MB_YESNO) == IDNO)
	{
		return nullptr;
	}

	m_action->type = Action::Type::eRunScript;
	m_action->scriptPath = text.GetString();

	return m_action;
}
