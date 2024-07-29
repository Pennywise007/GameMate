#pragma once

#include "afxdialogex.h"
#include <memory>

#include "core/events.h"
#include "core/Settings.h"

#include <Controls/Button/IconButton/IconButton.h>
#include <Controls/ComboBox/CComboBoxWithSearch/ComboWithSearch.h>
#include <Controls/ComboBox/CheckCombobox/CheckComboBox.h>
#include <Controls/ComboBox/CIconComboBox/IconComboBox.h>
#include <Controls/Tables/List/ListGroupCtrl/ListGroupCtrl.h>
#include <Controls/Tables/List/Widgets/SubItemsEditor/SubItemsEditor.h>
#include <Controls/Splitter/Splitter.h>
#include <Controls/Slider/Slider.h>

#include "UI/Controls/CenteredLineStatic.h"

#include "UI/Dlg/TableDlg.h"

class CActiveProcessToolkitTab : public CDialogEx, ext::events::ScopeSubscription<ISettingsChanged>
{
	DECLARE_DYNAMIC(CActiveProcessToolkitTab)

public:
	CActiveProcessToolkitTab(CWnd* pParent);
	~CActiveProcessToolkitTab();

// Dialog Data
	enum { IDD = IDD_TAB_ACTIVE_PROCESS_TOOLKIT };

protected:
	DECLARE_MESSAGE_MAP()

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;
	virtual BOOL OnInitDialog() override;
	virtual void OnOK() override;
	virtual void OnCancel() override;
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedCheckActiveProcessToolkitEnable();
	afx_msg void OnCbnSelchangeComboConfiguration();
	afx_msg void OnBnClickedButtonAddConfiguration();
	afx_msg void OnBnClickedButtonRenameConfiguration();
	afx_msg void OnBnClickedButtonRemoveConfiguration();
	afx_msg void OnBnClickedCheckEnabled();
	afx_msg void OnCbnEditchangeComboExeName();
	afx_msg void OnCbnSelendChangedComboExeName();
	afx_msg void OnCbnSetfocusComboExeName();
	afx_msg void OnBnClickedCheckShowCrosshair();
	afx_msg void OnCbnSelendokComboCrosshairSelection();
	afx_msg void OnBnClickedMfccolorbuttonCrosshairColor();
	afx_msg void OnCbnSelendokComboCrosshairSize();
	afx_msg void OnBnClickedButtonAccidentalPressAddCustom();
	afx_msg void OnCbnSelchangeComboAccidentalPress();
	afx_msg void OnBnClickedCheckChangeBrightness();
	afx_msg void OnTRBNThumbPosChangingSliderBrightness(NMHDR* pNMHDR, LRESULT* pResult);

private: // ISettingsChanged
	void OnSettingsChanged(ISettingsChanged::ChangedType changedType) override;

private:
	void createTableDlg(CTableDlg& view, CStatic& placeholder);
	void initMacrosesTable();
	void initKeyRebindingsTable();
	void UpdateControlsData();
	void UpdateEnableButton();

	void AddNewActions(const Bind& bind, Actions&& actions);
	void UpdateDemoCrosshair();
	void InitCrosshairsList();
	void addKeyToIgnore(const Key& key);

private: // controls
	CIconButton m_checkEnabled;
	CComboBox m_comboConfigurations;
	CIconButton m_buttonAddConfiguration;
	CButton m_enabled;
	ComboWithSearch m_exeName;
	CStatic m_staticExeNameInfo;
	CStatic m_groupMacrosesPlaceholder;
	CSplitter m_splitterForKeys;
	CStatic m_groupRemappingPlaceholder;
	CCenteredLineStatic m_groupAccidentalPress;
	CCheckComboBox m_comboAccidentalPress;
	CCenteredLineStatic m_groupBrightness;
	CButton m_checkChangeBrightness;
	CSlider m_brightness;
	CStatic m_staticBrightnessInfo;
	CCenteredLineStatic m_groupCrosshair;
	CButton m_checkboxShowCrosshair;
	CIconComboBox m_comboCrosshairs;
	CStatic m_staticCrosshairInfo;
	CComboBox m_comboboxCrosshairSize;
	CMFCColorButton m_colorPickerCrosshairColor;
	CStatic m_crosshairDemo;

private:
	std::shared_ptr<process_toolkit::ProcessConfiguration> m_configuration;
	std::list<CBitmap> m_crosshairs;
	HICON m_demoIcon = nullptr;

private:
	CTableDlg m_macrosesDlg = this;
	CTableDlg m_keyRemappingDlg = this;
};
