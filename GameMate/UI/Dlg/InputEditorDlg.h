#pragma once

#include "afxdialogex.h"

#include <optional>

#include "core/Settings.h"

#include "UI/ActionEditors/InputEditorView.h"

class CInputEditorDlg : protected CDialogEx
{
public:
	[[nodiscard]] static std::optional<Bind> EditBind(
		CWnd* pParent,
		const std::optional<Bind>& editKey = std::nullopt);
	[[nodiscard]] static std::optional<Key> EditKey(
		CWnd* pParent,
		const std::optional<Key>& editKey = std::nullopt);

protected:
	CInputEditorDlg(
		CWnd* pParent,
		InputEditor::EditorType editorType,
		std::shared_ptr<IBaseInput> baseInput,
		bool addingInput);
	DECLARE_DYNAMIC(CInputEditorDlg)

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_INPUT_EDITOR };
#endif
protected:
	BOOL OnInitDialog() override;
	void OnOK() override;

protected:
	InputEditor* m_editor;
	const bool m_addingInput;
	const InputEditor::EditorType m_editorType;
	std::shared_ptr<IBaseInput> m_baseInput;
};
