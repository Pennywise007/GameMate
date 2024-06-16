#pragma once

#include "afxdialogex.h"

#include <optional>

#include "core/Settings.h"

#include "UI/ActionEditors/InputEditorView.h"

class CEditBindDlg : protected CDialogEx
{
	CEditBindDlg(CWnd* pParent, Bind& bind, bool editBind);
	DECLARE_DYNAMIC(CEditBindDlg)

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_EDIT_BIND };
#endif
public:
	[[nodiscard]] static std::optional<Bind> EditBind(CWnd* pParent, const std::optional<Bind>& editBind = std::nullopt);

private:
	virtual BOOL OnInitDialog();
	void OnOK() override;

private:
	CBindEditorView* m_bindEditor = nullptr;
	Bind& m_currentBind;
	const bool m_editBind;
};
