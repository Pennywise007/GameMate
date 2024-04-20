#pragma once

#include "afxdialogex.h"
#include <memory>

#include "Settings.h"

#include <Controls/ComboBox/CComboBoxWithSearch/ComboWithSearch.h>
#include <Controls/ComboBox/CIconComboBox/IconComboBox.h>
#include <Controls/Tables/List/ListGroupCtrl/ListGroupCtrl.h>
#include <Controls/Tables/List/Widgets/SubItemsEditor/SubItemsEditor.h>
#include <Controls/Tooltip/ToolTip.h>

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
	afx_msg void OnCbnSetfocusComboExeName();
	afx_msg void OnCbnEditchangeComboExeName();
	afx_msg void OnCbnSelendokComboExeName();
	afx_msg void OnBnClickedCheckUse();
	afx_msg void OnBnClickedMfccolorbuttonCrosshairColor();
	afx_msg void OnCbnSelendokComboCrosshairSelection();
	afx_msg void OnCbnSelendokComboCrosshairSize();
	afx_msg void OnLvnItemchangedListMacroses(NMHDR* pNMHDR, LRESULT* pResult);

private:
	void AddNewMacros(TabConfiguration::Keybind keybind, Macros&& macros);
	void ChangeCrosshairColors(COLORREF newColor);

private:
	const std::shared_ptr<TabConfiguration> m_configuration;
	std::list<CBitmap> m_crosshairs;

	CButton m_enabled;
	ComboWithSearch m_exeName;
	CIconComboBox m_comboCrosshairs;
	controls::list::widgets::SubItemsEditor<CListGroupCtrl> m_listMacroses;

	CStatic m_staticCrosshairInfo;
	controls::CToolTip m_staticCrosshairInfoTooltip;
	CButton m_checkboxShowCrosshair;
	CComboBox m_comboboxCrosshairSize;
	CMFCColorButton m_colorPickerCrosshairColor;
};
