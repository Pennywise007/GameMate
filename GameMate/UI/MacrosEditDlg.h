#pragma once

#include "afxdialogex.h"

#include <chrono>

#include "core/Settings.h"

#include <Controls/Button/IconButton/IconButton.h>
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
	
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	virtual BOOL PreTranslateMessage(MSG* pMsg) override;
	virtual BOOL OnInitDialog() override;
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnBnClickedButtonRemove();
	afx_msg void OnBnClickedButtonRecord();
	afx_msg void OnBnClickedButtonMoveUp();
	afx_msg void OnBnClickedButtonMoveDown();
	afx_msg void OnLvnItemchangedListMacroses(NMHDR* pNMHDR, LRESULT* pResult);

private:
	void addAction(MacrosAction&& action);

private:
	controls::list::widgets::SubItemsEditor<CListGroupCtrl> m_listMacroses;
	CIconButton m_buttonRecord;
	CSpinEdit m_editRandomizeDelays;
	CStatic m_staticDelayHelp;
	CIconButton m_buttonMoveUp;
	CIconButton m_buttonMoveDown;

private:
	int m_keyPressedSubscriptionId = -1;
	Macros m_macros;
	std::optional<std::chrono::steady_clock::time_point> m_lastActionTime;
};
