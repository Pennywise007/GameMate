#pragma once

#include "afxdialogex.h"

#include <chrono>

#include "Settings.h"

#include <Controls/Tooltip/ToolTip.h>
#include <Controls/Edit/SpinEdit/SpinEdit.h>
#include <Controls/Tables/List/ListGroupCtrl/ListGroupCtrl.h>
#include <Controls/Tables/List/Widgets/SubItemsEditor/SubItemsEditor.h>

class CMacrosEditDlg : protected CDialogEx
{
	DECLARE_DYNAMIC(CMacrosEditDlg)

public:
	CMacrosEditDlg(const Macros& macros, CWnd* pParent = nullptr);   // standard constructor

	[[nodiscard]] std::optional<Macros> ExecModal();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_MACROS_EDIT };
#endif

protected:
	DECLARE_MESSAGE_MAP()
	
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnClose();
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnBnClickedButtonRemove();
	afx_msg void OnBnClickedButtonRecord();
	afx_msg void OnLvnItemchangedListMacroses(NMHDR* pNMHDR, LRESULT* pResult);

private:
	void addAction(Macros::Action&& action);

private:
	controls::list::widgets::SubItemsEditor<CListGroupCtrl> m_listMacroses;
	CButton m_buttonRecord;
	CSpinEdit m_editRandomizeDelays;
	CStatic m_staticDelayHelp;
	controls::CToolTip m_staticDelayTooltip;

	Macros m_macros;
	std::chrono::steady_clock::time_point m_lastActionTime;
};
