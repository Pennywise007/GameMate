#pragma once

#include "afxdialogex.h"

#include <optional>

#include "core/Settings.h"

class CBindEditDlg : protected CDialogEx
{
	DECLARE_DYNAMIC(CBindEditDlg)

public:
	CBindEditDlg(CWnd* pParent, std::optional<Macros::Action> action = std::nullopt);

	[[nodiscard]] std::optional<Macros::Action> ExecModal();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_BIND_EDIT };
#endif

protected:
	DECLARE_MESSAGE_MAP()
	
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

private:
	CEdit m_editAction;
	std::optional<Macros::Action> m_lastAction;
	virtual void OnOK();
};
