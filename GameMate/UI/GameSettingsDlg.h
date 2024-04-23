#pragma once

#include "afxdialogex.h"
#include <memory>

#include "core/Settings.h"

#include <Controls/ComboBox/CComboBoxWithSearch/ComboWithSearch.h>
#include <Controls/ComboBox/CIconComboBox/IconComboBox.h>
#include <Controls/Tables/List/ListGroupCtrl/ListGroupCtrl.h>
#include <Controls/Tables/List/Widgets/SubItemsEditor/SubItemsEditor.h>

class CGameSettingsDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CGameSettingsDlg)

public:
	CGameSettingsDlg(std::shared_ptr<TabConfiguration> configuration, CWnd* pParent = nullptr);   // standard constructor
	~CGameSettingsDlg();

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
	afx_msg void OnCbnSelendokComboExeName();
	afx_msg void OnCbnDropdownComboExeName();
	afx_msg void OnBnClickedCheckDisableWin();
	afx_msg void OnBnClickedCheckUse();
	afx_msg void OnBnClickedMfccolorbuttonCrosshairColor();
	afx_msg void OnCbnSelendokComboCrosshairSelection();
	afx_msg void OnCbnSelendokComboCrosshairSize();
	afx_msg void OnLvnItemchangedListMacroses(NMHDR* pNMHDR, LRESULT* pResult);

private:
	void AddNewMacros(const Bind& bind, Macros&& macros);
	void UpdateDemoCrosshair();
	void InitCrosshairsList();

private: // controls
	CButton m_enabled;
	ComboWithSearch m_exeName;
	CButton m_checkDisableWinButton;

	CButton m_checkboxShowCrosshair;
	CIconComboBox m_comboCrosshairs;
	CStatic m_staticCrosshairInfo;
	CComboBox m_comboboxCrosshairSize;
	CMFCColorButton m_colorPickerCrosshairColor;
	controls::list::widgets::SubItemsEditor<CListGroupCtrl> m_listMacroses;
	CStatic m_crosshairDemo;

private:
	const std::shared_ptr<TabConfiguration> m_configuration;
	std::list<CBitmap> m_crosshairs;
	HICON m_demoIcon = nullptr;
public:
};
