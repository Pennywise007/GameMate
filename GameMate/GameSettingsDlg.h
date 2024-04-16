#pragma once

#include "afxdialogex.h"
#include <memory>

#include "Settings.h"

#include <Controls/Tables/List/ListGroupCtrl/ListGroupCtrl.h>
#include <Controls/Tables/List/Widgets/SubItemsEditor/SubItemsEditor.h>

class CGameSettingsDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CGameSettingsDlg)

public:
	CGameSettingsDlg(std::shared_ptr<TabConfiguration> configuration, CWnd* pParent = nullptr);   // standard constructor

// Dialog Data
	enum { IDD = IDD_TAB_GAME_SETTINGS };

protected:
	DECLARE_MESSAGE_MAP()

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnBnClickedButtonRemove();
	afx_msg void OnBnClickedCheckEnabled();
	afx_msg void OnEnChangeEditGameName();

private:
	void AddNewMacros(TabConfiguration::Keybind keybind, Macros&& macros);

private:
	const std::shared_ptr<TabConfiguration> m_configuration;
	
	CButton m_enabled;
	CEdit m_exeName;
	controls::list::widgets::SubItemsEditor<CListGroupCtrl> m_listMacroses;
public:
	afx_msg void OnLvnItemchangedListMacroses(NMHDR* pNMHDR, LRESULT* pResult);
};
