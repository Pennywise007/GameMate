#pragma once
#include "afxdialogex.h"

#include <optional>

#include "ActionsEditor.h"

class CAddActionDlg : protected CDialogEx
{
	CAddActionDlg(CWnd* pParent, Action& currentAction, bool editAction);
	DECLARE_DYNAMIC(CAddActionDlg)

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_ADD_ACTION };
#endif
	
public:
	[[nodiscard]] static std::optional<Action> EditAction(CWnd* parent, std::optional<Action> currentAction = std::nullopt);

protected:
	void DoDataExchange(CDataExchange* pDX) override;
	BOOL PreTranslateMessage(MSG* pMsg) override;
	BOOL OnInitDialog() override;
	virtual void OnOK();
	afx_msg void OnCbnSelendokComboType();

	DECLARE_MESSAGE_MAP()

protected:
	void updateCurrentView();

protected:
	CComboBox m_type;
	CStatic m_editor;

private:
	ActionsEditor* m_activeEditor = nullptr;
	std::vector<ActionsEditor*> m_editors;
	Action& m_currentAction;
	const bool m_editAction;
};
