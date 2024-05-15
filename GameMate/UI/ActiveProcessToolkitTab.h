#pragma once

#include "afxdialogex.h"
#include <memory>

#include "core/Settings.h"

#include <Controls/Button/IconButton/IconButton.h>
#include <Controls/ComboBox/CComboBoxWithSearch/ComboWithSearch.h>
#include <Controls/ComboBox/CIconComboBox/IconComboBox.h>
#include <Controls/Tables/List/ListGroupCtrl/ListGroupCtrl.h>
#include <Controls/Tables/List/Widgets/SubItemsEditor/SubItemsEditor.h>

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
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnBnClickedCheckActiveProcessToolkitEnable();
	afx_msg void OnCbnSelchangeComboConfiguration();
	afx_msg void OnBnClickedButtonAddConfiguration();
	afx_msg void OnBnClickedButtonRenameConfiguration();
	afx_msg void OnBnClickedButtonRemoveConfiguration();
	afx_msg void OnBnClickedCheckEnabled();
	afx_msg void OnCbnEditchangeComboExeName();
	afx_msg void OnCbnSelendokComboExeName();
	afx_msg void OnCbnSetfocusComboExeName();
	afx_msg void OnBnClickedCheckDisableWin();
	afx_msg void OnBnClickedButtonAddMacros();
	afx_msg void OnBnClickedButtonRemoveMacros();
	afx_msg void OnLvnItemchangedListActions(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedCheckShowCrosshair();
	afx_msg void OnCbnSelendokComboCrosshairSelection();
	afx_msg void OnBnClickedMfccolorbuttonCrosshairColor();
	afx_msg void OnCbnSelendokComboCrosshairSize();

private: // ISettingsChanged
	void OnSettingsChanged(ISettingsChanged::ChangedType changedType) override;

private:
	void UpdateControlsData();
	void UpdateEnableButton();

	void AddNewActions(const Bind& bind, Actions&& actions);
	void UpdateDemoCrosshair();
	void InitCrosshairsList();

private: // controls
	CIconButton m_checkEnabled;
	CComboBox m_comboConfigurations;
	CIconButton m_buttonAddConfiguration;
	CButton m_enabled;
	ComboWithSearch m_exeName;
	CButton m_checkDisableWinButton;

	CButton m_checkboxShowCrosshair;
	CIconComboBox m_comboCrosshairs;
	CStatic m_staticCrosshairInfo;
	CComboBox m_comboboxCrosshairSize;
	CMFCColorButton m_colorPickerCrosshairColor;
	controls::list::widgets::SubItemsEditor<CListGroupCtrl> m_listActions;
	CStatic m_crosshairDemo;

private:
	std::shared_ptr<process_toolkit::ProcessConfiguration> m_configuration;
	std::list<CBitmap> m_crosshairs;
	HICON m_demoIcon = nullptr;
public:
};
