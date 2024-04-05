#pragma once

#include "afxdialogex.h"

#include <chrono>

#include "Settings.h"

#include <Controls/Tables/List/ListGroupCtrl/ListGroupCtrl.h>
#include <Controls/Tables/List/Widgets/SubItemsEditor/SubItemsEditor.h>

class CEditMacrosDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CEditMacrosDlg)

public:
	CEditMacrosDlg(const std::list<Macros::Action>& actions, CWnd* pParent = nullptr);   // standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_EDIT_MACROS };
#endif

protected:
	DECLARE_MESSAGE_MAP()
	
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnBnClickedButtonRemove();
	afx_msg void OnBnClickedButtonRecord();

private:
	void addActionToTable(const Macros::Action& action, int ind = -1);

public:
	controls::list::widgets::SubItemsEditor<CListGroupCtrl> m_listMacroses;
	CButton m_buttonRecord;
	
	std::list<Macros::Action> m_actions;
	std::chrono::steady_clock::time_point m_lastActionTime;
};
