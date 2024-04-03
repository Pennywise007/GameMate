#pragma once

#include "afxdialogex.h"
#include <memory>

#include "Settings.h"

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
	afx_msg void OnBnClickedButtonEdit();
	afx_msg void OnBnClickedButtonRemove();
	afx_msg void OnBnClickedCheckEnabled();
	afx_msg void OnEnChangeEditGameName();

private:
	const std::shared_ptr<TabConfiguration> m_configuration;

public:
	CButton m_enabled;
	CEdit m_exeName;
};
