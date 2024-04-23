#pragma once

#include "afxdialogex.h"

#include <optional>

#include "core/Settings.h"

class CActionEditDlg : protected CDialogEx
{
	DECLARE_DYNAMIC(CActionEditDlg)

	enum class Type {
		eAction,	// editing actions (key down/up, mouse)
		eBind		// editing bind (key press, mouse click)
	};
	CActionEditDlg(CWnd* pParent, Type type, std::optional<Action>& action);
public:
	[[nodiscard]] static std::optional<Bind> EditBind(CWnd* pParent, const std::optional<Bind>& editBind = std::nullopt);
	[[nodiscard]] static std::optional<MacrosAction> EditMacros(CWnd* pParent, const std::optional<MacrosAction>& editMacros = std::nullopt);

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_ACTION_EDIT };
#endif

protected:
	DECLARE_MESSAGE_MAP()
	
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

private:
	std::wstring GetActionText() const;

private:
	const Type m_editingType;
	std::optional<Action>& m_currentAction;

private:
	CEdit m_editAction;
};
