#include "pch.h"

#include <algorithm>
#include <map>
#include <regex>

#include "tlhelp32.h"
#include "afxdialogex.h"
#include "resource.h"

#include "UI/Tab/ActiveProcessToolkitTab.h"
#include "UI/ActionEditors/EditActions.h"
#include "UI/Dlg/InputEditorDlg.h"
#include "UI/Dlg/AddingProcessToolkitDlg.h"

#include "core/Crosshairs.h"
#include "core/DisplayBrightnessController.h"

#include <ext/scope/defer.h>
#include <ext/reflection/enum.h>
#include <ext/std/filesystem.h>
#include <ext/std/string.h>

#include <Controls/DefaultWindowProc.h>
#include <Controls/Layout/Layout.h>
#include <Controls/Tooltip/ToolTip.h>

using namespace controls::list::widgets;

namespace {

enum MacrosesColumns {
	eKeybind = 0,
	eMacros,
	eRandomizeDelay
};

enum RebingingColumns {
	eOriginal = 0,
	eNew,
};

const std::map kDefaultIgnoredKeys = {
	std::pair{ 0, Key(VK_LWIN) },
	std::pair{ 1, Key(VK_ESCAPE) },
	std::pair{ 2, Key(VK_TAB) },
	std::pair{ 3, Key(VK_SHIFT) },
	std::pair{ 4, Key(VK_CONTROL) },
	std::pair{ 5, Key(VK_MENU) },
	std::pair{ 6, Key(VK_CAPITAL) },
};

} // namespace

IMPLEMENT_DYNAMIC(CActiveProcessToolkitTab, CDialogEx)

CActiveProcessToolkitTab::CActiveProcessToolkitTab(CWnd* pParent)
	: CDialogEx(IDD_TAB_GAME_SETTINGS, pParent)
{
}

CActiveProcessToolkitTab::~CActiveProcessToolkitTab()
{
	if (m_demoIcon != nullptr)
		::DestroyIcon(m_demoIcon);
}

void CActiveProcessToolkitTab::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHECK_ENABLED, m_enabled);
	DDX_Control(pDX, IDC_COMBO_EXE_NAME, m_exeName);
	DDX_Control(pDX, IDC_MACROSES_PLACEHOLDER, m_groupMacrosesPlaceholder);
	DDX_Control(pDX, IDC_SPLITTER, m_splitterForKeys);
	DDX_Control(pDX, IDC_KEYS_REMAPPING_PLACEHOLDER, m_groupRemappingPlaceholder);
	DDX_Control(pDX, IDC_COMBO_CROSSHAIR_SELECTION, m_comboCrosshairs);
	DDX_Control(pDX, IDC_CHECK_SHOW_CROSSHAIR, m_checkboxShowCrosshair);
	DDX_Control(pDX, IDC_COMBO_CROSSHAIR_SIZE, m_comboboxCrosshairSize);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_CROSSHAIR_COLOR, m_colorPickerCrosshairColor);
	DDX_Control(pDX, IDC_STATIC_CROSSHAIR_INFO, m_staticCrosshairInfo);
	DDX_Control(pDX, IDC_STATIC_CROSSHAIR_DEMO, m_crosshairDemo);
	DDX_Control(pDX, IDC_COMBO_CONFIGURATION, m_comboConfigurations);
	DDX_Control(pDX, IDC_CHECK_ACTIVE_PROCESS_TOOLKIT_ENABLE, m_checkEnabled);
	DDX_Control(pDX, IDC_BUTTON_ADD_CONFIGURATION, m_buttonAddConfiguration);
	DDX_Control(pDX, IDC_STATIC_EXE_NAME_INFO, m_staticExeNameInfo);
	DDX_Control(pDX, IDC_COMBO_ACCIDENTAL_PRESS, m_comboAccidentalPress);
	DDX_Control(pDX, IDC_SLIDER_BRIGHTNESS, m_brightness);
	DDX_Control(pDX, IDC_CHECK_CHANGE_BRIGHTNESS, m_checkChangeBrightness);
	DDX_Control(pDX, IDC_STATIC_BRIGHTNESS_INFO, m_staticBrightnessInfo);
	DDX_Control(pDX, IDC_MACROSES_GROUP3, m_groupAccidentalPress);
	DDX_Control(pDX, IDC_MACROSES_GROUP4, m_groupBrightness);
	DDX_Control(pDX, IDC_MACROSES_GROUP2, m_groupCrosshair);
}

BEGIN_MESSAGE_MAP(CActiveProcessToolkitTab, CDialogEx)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_CHECK_ACTIVE_PROCESS_TOOLKIT_ENABLE, &CActiveProcessToolkitTab::OnBnClickedCheckActiveProcessToolkitEnable)
	ON_CBN_SELCHANGE(IDC_COMBO_CONFIGURATION, &CActiveProcessToolkitTab::OnCbnSelchangeComboConfiguration)
	ON_BN_CLICKED(IDC_BUTTON_ADD_CONFIGURATION, &CActiveProcessToolkitTab::OnBnClickedButtonAddConfiguration)
	ON_BN_CLICKED(IDC_BUTTON_RENAME_CONFIGURATION, &CActiveProcessToolkitTab::OnBnClickedButtonRenameConfiguration)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_CONFIGURATION, &CActiveProcessToolkitTab::OnBnClickedButtonRemoveConfiguration)
	ON_BN_CLICKED(IDC_CHECK_ENABLED, &CActiveProcessToolkitTab::OnBnClickedCheckEnabled)
	// ON_CBN_SELENDOK and ON_CBN_SELENDCANCEL got the same callback for this combobox since user can enter
	// text and change focus(we receive SELENDCANCEL) and we want to save the value
	ON_CBN_SELENDOK(IDC_COMBO_EXE_NAME, &CActiveProcessToolkitTab::OnCbnSelendChangedComboExeName)
	ON_CBN_SELENDCANCEL(IDC_COMBO_EXE_NAME, &CActiveProcessToolkitTab::OnCbnSelendChangedComboExeName)
	ON_CBN_SETFOCUS(IDC_COMBO_EXE_NAME, &CActiveProcessToolkitTab::OnCbnSetfocusComboExeName)
	ON_CBN_EDITCHANGE(IDC_COMBO_EXE_NAME, &CActiveProcessToolkitTab::OnCbnEditchangeComboExeName)
	ON_BN_CLICKED(IDC_CHECK_SHOW_CROSSHAIR, &CActiveProcessToolkitTab::OnBnClickedCheckShowCrosshair)
	ON_CBN_SELENDOK(IDC_COMBO_CROSSHAIR_SELECTION, &CActiveProcessToolkitTab::OnCbnSelendokComboCrosshairSelection)
	ON_BN_CLICKED(IDC_MFCCOLORBUTTON_CROSSHAIR_COLOR, &CActiveProcessToolkitTab::OnBnClickedMfccolorbuttonCrosshairColor)
	ON_CBN_SELENDOK(IDC_COMBO_CROSSHAIR_SIZE, &CActiveProcessToolkitTab::OnCbnSelendokComboCrosshairSize)
	ON_BN_CLICKED(IDC_BUTTON_ACCIDENTAL_PRESS_ADD_CUSTOM, &CActiveProcessToolkitTab::OnBnClickedButtonAccidentalPressAddCustom)
	ON_CBN_SELCHANGE(IDC_COMBO_ACCIDENTAL_PRESS, &CActiveProcessToolkitTab::OnCbnSelchangeComboAccidentalPress)
	ON_BN_CLICKED(IDC_CHECK_CHANGE_BRIGHTNESS, &CActiveProcessToolkitTab::OnBnClickedCheckChangeBrightness)
	ON_NOTIFY(TRBN_THUMBPOSCHANGING, IDC_SLIDER_BRIGHTNESS, &CActiveProcessToolkitTab::OnTRBNThumbPosChangingSliderBrightness)
END_MESSAGE_MAP()

BOOL CActiveProcessToolkitTab::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	GetDlgItem(IDC_STATIC_CROSSHAIR_DEMO)->ModifyStyle(0, SS_REALSIZECONTROL);
	GetDlgItem(IDC_STATIC_CROSSHAIR_INFO)->ModifyStyle(0, SS_REALSIZECONTROL);

	// Green when enabled
	m_checkEnabled.SetBkColor(RGB(213, 255, 219));

	// Yellow when we need user to press button
	m_buttonAddConfiguration.SetBkColor(RGB(253, 243, 166));

	m_exeName.AllowCustomText();
	m_exeName.SetCueBanner(L"Enter process name...(example: notepad*.exe)");

	auto addInfoIconAndTooltip = [](CStatic& control, CString tooltip) {
		CRect rect;
		control.GetClientRect(rect);
		HICON icon;
		auto res = SUCCEEDED(::LoadIconWithScaleDown(NULL, IDI_INFORMATION, rect.Height(), rect.Height(), &icon));
		ASSERT(res);
		control.ModifyStyle(0, SS_ICON);
		control.SetIcon(icon);
		controls::SetTooltip(control, tooltip);
	};
	addInfoIconAndTooltip(m_staticExeNameInfo,
		L"You can use `*` to match any exe name. For example:\n"
		L"- `notepad*.exe` will match `notepad++.exe` and `notepad.exe`.\n"
		L"- `*` will match every exe name.\n"
		L"Note: if active application exe name matches multiple configurations, the first active match configuration will be used."
	);
	addInfoIconAndTooltip(m_staticCrosshairInfo,
		L"If you want to add your own crosshair put file with name crosshair_X with .png or .ico extension to the folder:\n"
		L"$GAME_MATE_FOLDER$\\res\n"
		L"Note: all images bigger than 32x32 will be ignored."
	);
	addInfoIconAndTooltip(m_staticBrightnessInfo, L"");

	m_comboboxCrosshairSize.AddString(L"Small(16x16)");
    m_comboboxCrosshairSize.AddString(L"Medium(24x24)");
    m_comboboxCrosshairSize.AddString(L"Large(32x32)");

	{
		m_crosshairDemo.ModifyStyle(0, SS_ICON);
		controls::SetTooltip(m_crosshairDemo, L"Demo of the crosshair");
	}

	initMacrosesTable();
	initKeyRebindingsTable();

	for (auto&& [ind, key] : kDefaultIgnoredKeys)
	{
		auto res = m_comboAccidentalPress.InsertString(ind, key.ToString().c_str());
		EXT_ASSERT(res == ind);
		m_comboAccidentalPress.SetItemDataPtr(res, new Key(key));
	}
	m_comboAccidentalPress.SetCueBanner(L"Select buttons to ignore the first press...");
	m_brightness.SetTooltipTextFormat(L"%.lf");
	m_brightness.SetIncrementStep(1);

	CRect rect;
	m_keyRemappingDlg.GetClientRect(rect);

	m_splitterForKeys.AttachSplitterToWindow(*this, CMFCDynamicLayout::MoveHorizontal(100), CMFCDynamicLayout::SizeVertical(100));
	m_splitterForKeys.SetControlBounds(CSplitter::BoundsType::eOffsetFromParentBounds,
		CRect(300, CSplitter::kNotSpecified, rect.Width(), CSplitter::kNotSpecified));
	
	UpdateControlsData();

	return TRUE;
}

void CActiveProcessToolkitTab::createTableDlg(CTableDlg& view, CStatic& placeholder)
{
	view.Create(CTableDlg::IDD, this);

	CRect rect;
	placeholder.GetWindowRect(rect);
	ScreenToClient(rect);

	view.MoveWindow(rect);
	view.ModifyStyle(0, WS_TABSTOP);
	view.ShowWindow(SW_NORMAL);
	placeholder.ShowWindow(SW_HIDE);
}

void CActiveProcessToolkitTab::initMacrosesTable()
{
	createTableDlg(m_macrosesDlg, m_groupMacrosesPlaceholder);
	m_macrosesDlg.Init(L"Macroses", L"Add bind",
		[&]() {
			auto bind = CInputEditorDlg::EditBind(this);
			if (!bind.has_value())
				return;

			const auto it = m_configuration->actionsByBind.find(bind.value());
			const bool actionExists = it != m_configuration->actionsByBind.end();

			auto& table = m_macrosesDlg.GetTable();
			Actions editableActions;
			if (actionExists)
			{
				auto res = MessageBox((L"Do you want to edit action '" + bind->ToString() + L"'?").c_str(), L"Action already has actionsByBind", MB_OKCANCEL | MB_ICONWARNING);
				if (res == IDCANCEL)
				{
					table.ClearSelection();
					table.SelectItem((int)std::distance(m_configuration->actionsByBind.begin(), it));
					return;
				}

				editableActions = it->second;
			}
			else {
				// Adding new item to the table to show user what we adding
				auto item = table.InsertItem(table.GetItemCount(), bind->ToString().c_str());
				table.SelectItem(item);
			}

			auto newActions = CActionsEditDlg::ExecModal(this, editableActions);

			// removing previously added item with bind name
			if (!actionExists)
				table.DeleteItem(table.GetItemCount() - 1);

			if (!newActions.has_value())
				return;

			// removing old copy of edited actionsByBind
			if (actionExists)
			{
				auto sameItem = (int)std::distance(m_configuration->actionsByBind.begin(), it);
				m_configuration->actionsByBind.erase(it);
				table.DeleteItem(sameItem);
			}

			AddNewActions(bind.value(), std::move(newActions.value()));

			ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);
		},
		[&]() {
			auto& table = m_macrosesDlg.GetTable();
			std::vector<int> selectedActions = table.GetSelectedItems();

			for (auto it = selectedActions.rbegin(), end = selectedActions.rend(); it != end; ++it)
			{
				m_configuration->actionsByBind.erase(std::next(m_configuration->actionsByBind.begin(), *it));
				table.DeleteItem(*it);
			}

			ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);
		});

	CRect rect;
	auto& list = m_macrosesDlg.GetTable();
	list.GetClientRect(rect);

	constexpr int kKeybindColumnWidth = 80;
	constexpr int kRandomizeDelayColumnWidth = 50;
	list.InsertColumn(MacrosesColumns::eKeybind, L"Key bind", LVCFMT_CENTER, kKeybindColumnWidth);
	list.InsertColumn(MacrosesColumns::eMacros, L"Macros", LVCFMT_CENTER, rect.Width() - kKeybindColumnWidth - kRandomizeDelayColumnWidth);
	list.InsertColumn(MacrosesColumns::eRandomizeDelay, L"Randomize delay, ms", LVCFMT_CENTER, kRandomizeDelayColumnWidth);
	list.SetProportionalResizingColumns({ MacrosesColumns::eMacros });

	list.setSubItemEditorController(MacrosesColumns::eKeybind,
		[&](CListCtrl* pList, CWnd* parentWindow, const LVSubItemParams* pParams)
		{
			auto& actionsByBind = m_configuration->actionsByBind;

			ASSERT((int)actionsByBind.size() > pParams->iItem);
			auto editableActionsIt = std::next(actionsByBind.begin(), pParams->iItem);

			auto bind = CInputEditorDlg::EditBind(parentWindow, editableActionsIt->first);
			if (!bind.has_value())
				return nullptr;

			if (auto sameBindIt = actionsByBind.find(bind.value()); sameBindIt != actionsByBind.end())
			{
				if (sameBindIt == editableActionsIt)
					return nullptr;

				if (MessageBox((L"Bind '" + bind->ToString() + L"' already exists, do you want to replace it?").c_str(),
					L"This bind already exist", MB_ICONWARNING | MB_OKCANCEL) == IDCANCEL)
					return nullptr;

				auto sameItem = (int)std::distance(actionsByBind.begin(), sameBindIt);
				auto actions = std::move(editableActionsIt->second);
				actionsByBind.erase(editableActionsIt);
				actionsByBind.erase(sameBindIt);
				m_macrosesDlg.GetTable().DeleteItem(std::max<int>(pParams->iItem, sameItem));
				m_macrosesDlg.GetTable().DeleteItem(std::min<int>(pParams->iItem, sameItem));

				AddNewActions(std::move(bind.value()), std::move(actions));
			}
			else
			{
				auto currentActions = std::move(editableActionsIt->second);
				actionsByBind.erase(editableActionsIt);
				m_macrosesDlg.GetTable().DeleteItem(pParams->iItem);
				AddNewActions(std::move(bind.value()), std::move(currentActions));
			}

			ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);

			return nullptr;
		});
	const auto actionsEdit = [&](CListCtrl* pList, CWnd* parentWindow, const LVSubItemParams* pParams)
		{
			auto& actionsByBind = m_configuration->actionsByBind;

			ASSERT((int)actionsByBind.size() > pParams->iItem);
			auto editableActionsIt = std::next(actionsByBind.begin(), pParams->iItem);

			auto actions = CActionsEditDlg::ExecModal(this, editableActionsIt->second);
			if (!actions.has_value())
				return nullptr;

			auto bind = editableActionsIt->first;

			actionsByBind.erase(editableActionsIt);
			m_macrosesDlg.GetTable().DeleteItem(pParams->iItem);

			AddNewActions(std::move(bind), std::move(actions.value()));

			ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);

			return nullptr;
		};

	list.setSubItemEditorController(MacrosesColumns::eMacros, actionsEdit);
	list.setSubItemEditorController(MacrosesColumns::eRandomizeDelay, actionsEdit);

	Layout::AnchorWindow(m_macrosesDlg, m_splitterForKeys, { AnchorSide::eRight }, AnchorSide::eLeft, 100);
	Layout::AnchorWindow(m_macrosesDlg, *this, { AnchorSide::eBottom }, AnchorSide::eBottom, 100);
}

void CActiveProcessToolkitTab::initKeyRebindingsTable()
{
	createTableDlg(m_keyRemappingDlg, m_groupRemappingPlaceholder);
	m_keyRemappingDlg.Init(L"Keys remapping", L"Add remapping key",
		[&]() {
			auto keyToAdd = CInputEditorDlg::EditKey(this);
			if (!keyToAdd.has_value())
				return;

			auto& keysRemapping = m_configuration->keysRemapping;

			auto it = keysRemapping.find(keyToAdd.value());
			if (it != keysRemapping.end())
			{
				MessageBox((L"Key " + keyToAdd.value().ToString() + L" already exist").c_str(), L"Key already exist", MB_OK);
				return;
			}

			// Deleting line with old key and replacing text
			it = keysRemapping.emplace(std::move(keyToAdd.value()), std::move(Key(0))).first;
			int itemIndex = std::distance(keysRemapping.begin(), it);

			auto& table = m_keyRemappingDlg.GetTable();
			table.InsertItem(itemIndex, it->first.ToString().c_str());
			table.SetItemText(itemIndex, RebingingColumns::eNew, it->second.ToString().c_str());
			table.SelectItem(itemIndex);
			ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);
		},
		[&]() {
			auto& table = m_keyRemappingDlg.GetTable();
			std::vector<int> selectedActions = table.GetSelectedItems();

			for (auto it = selectedActions.rbegin(), end = selectedActions.rend(); it != end; ++it)
			{
				m_configuration->keysRemapping.erase(std::next(m_configuration->keysRemapping.begin(), *it));
				table.DeleteItem(*it);
			}

			ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);
		});

	auto& list = m_keyRemappingDlg.GetTable();
	CRect rect;
	list.GetClientRect(rect);

	list.InsertColumn(RebingingColumns::eOriginal, L"Original", LVCFMT_CENTER, rect.Width() / 2);
	list.InsertColumn(RebingingColumns::eNew, L"New", LVCFMT_CENTER, rect.Width() / 2);
	list.SetProportionalResizingColumns({ RebingingColumns::eOriginal, RebingingColumns::eNew });
	const auto keyEdit = [&](CListCtrl* pList, CWnd* parentWindow, const LVSubItemParams* pParams)
		{
			auto& keysRemapping = m_configuration->keysRemapping;

			ASSERT((int)keysRemapping.size() > pParams->iItem);
			auto editableKeyIt = std::next(keysRemapping.begin(), pParams->iItem);

			const Key& keyToEdit = pParams->iSubItem == RebingingColumns::eOriginal ? editableKeyIt->first : editableKeyIt->second;

			auto keyToAdd = CInputEditorDlg::EditKey(this, keyToEdit);
			if (!keyToAdd.has_value() || keyToAdd->ToString() == keyToEdit.ToString())
				return nullptr;

			auto& list = m_keyRemappingDlg.GetTable();

			switch (pParams->iSubItem)
			{
			case RebingingColumns::eOriginal:
			{
				auto it = keysRemapping.find(keyToAdd.value());
				if (it != keysRemapping.end())
				{
					auto res = MessageBox((L"Key " + keyToAdd.value().ToString() + L" already exist, do you want to replace it?").c_str(),
						L"Key already exist", MB_YESNO);
					if (res == IDNO)
						return nullptr;
				}

				// Deleting line with old key and replacing text
				auto newKeyValue = std::move(editableKeyIt->second);
				keysRemapping.erase(editableKeyIt);
				list.DeleteItem(pParams->iItem);

				if (it != keysRemapping.end())
				{
					it->second = std::move(newKeyValue);
					int itemIndex = std::distance(keysRemapping.begin(), it);
					list.SetItemText(itemIndex, RebingingColumns::eNew, it->second.ToString().c_str());
					list.SelectItem(itemIndex);
				}
				else
				{
					it = keysRemapping.emplace(std::move(keyToAdd.value()), std::move(newKeyValue)).first;
					int itemIndex = std::distance(keysRemapping.begin(), it);

					list.InsertItem(itemIndex, it->first.ToString().c_str());
					list.SetItemText(itemIndex, RebingingColumns::eNew, it->second.ToString().c_str());
					list.SelectItem(itemIndex);
				}
			}
			break;
			case RebingingColumns::eNew:
				editableKeyIt->second = std::move(keyToAdd.value());

				list.SetItemText(pParams->iItem, RebingingColumns::eNew, editableKeyIt->second.ToString().c_str());
				break;
			default:
				static_assert(ext::reflection::get_enum_size<RebingingColumns>() == 2);
				break;
			}

			ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);

			return nullptr;
		};
	list.setSubItemEditorController(RebingingColumns::eOriginal, keyEdit);
	list.setSubItemEditorController(RebingingColumns::eNew, keyEdit);

	Layout::AnchorWindow(m_keyRemappingDlg, m_splitterForKeys, { AnchorSide::eLeft }, AnchorSide::eRight, 100);
	Layout::AnchorWindow(m_keyRemappingDlg, *this, { AnchorSide::eRight }, AnchorSide::eRight, 100);
	Layout::AnchorWindow(m_keyRemappingDlg, *this, { AnchorSide::eBottom }, AnchorSide::eBottom, 100);
}

void CActiveProcessToolkitTab::UpdateControlsData()
{
	auto& settings = ext::get_singleton<Settings>().process_toolkit;

	if (settings.activeConfiguration != -1)
	{
		EXT_EXPECT(settings.activeConfiguration < (int)settings.processConfigurations.size());
		m_configuration = *std::next(settings.processConfigurations.begin(), settings.activeConfiguration);
	}
	else
		m_configuration = nullptr;

	UpdateEnableButton();

	constexpr UINT controlIdsToIgnoreVisibilityChange [] = {
		IDC_CHECK_ACTIVE_PROCESS_TOOLKIT_ENABLE,
		IDC_STATIC_CONFIGURATION,
		IDC_COMBO_CONFIGURATION,
		IDC_BUTTON_ADD_CONFIGURATION,
		IDC_MACROSES_PLACEHOLDER,
		IDC_KEYS_REMAPPING_PLACEHOLDER,
	};

	// Disable editable controls if no configuration selected
	CWnd* pChildWnd = GetWindow(GW_CHILD);
	while (pChildWnd)
	{
		const auto controlId = pChildWnd->GetDlgCtrlID();

		bool canChangeVisibilityStatus = true;
		for (const auto id : controlIdsToIgnoreVisibilityChange)
		{
			if (controlId == id)
			{
				canChangeVisibilityStatus = false;
				break;
			}
		}
		if (canChangeVisibilityStatus)
			pChildWnd->ShowWindow(!!m_configuration ? SW_SHOW : SW_HIDE);

		// Move to the next child control
		pChildWnd = pChildWnd->GetNextWindow(GW_HWNDNEXT);
	}

	m_comboConfigurations.ResetContent();
	for (const auto& config : settings.processConfigurations)
	{
		m_comboConfigurations.AddString(config->name.c_str());
	}
	m_comboConfigurations.SetCurSel(settings.activeConfiguration);

	m_buttonAddConfiguration.UseDefaultBkColor(!!m_configuration);

	GetDlgItem(IDC_BUTTON_RENAME_CONFIGURATION)->EnableWindow(!settings.processConfigurations.empty());
	GetDlgItem(IDC_BUTTON_REMOVE_CONFIGURATION)->EnableWindow(!settings.processConfigurations.empty());

	GetDlgItem(IDC_STATIC_NO_CONFIGURATION)->ShowWindow(m_configuration ? SW_HIDE : SW_SHOW);
	if (!m_configuration)
		return;

	EXT_ASSERT(!!m_configuration);

	m_enabled.SetCheck(m_configuration->enabled);
	m_exeName.SetWindowTextW(m_configuration->GetExeName().c_str());
	// updating control BK color
	OnCbnEditchangeComboExeName();

	for (const auto& bind : m_configuration->keysToIgnoreAccidentalPress)
	{
		addKeyToIgnore(bind);
	}

	{
		auto& table = m_macrosesDlg.GetTable();
		table.DeleteAllItems();
		decltype(m_configuration->actionsByBind) current;
		std::swap(current, m_configuration->actionsByBind);
		for (auto&& [bind, actionsByBind] : current)
		{
			AddNewActions(bind, std::move(actionsByBind));
		}
		m_macrosesDlg.UpdateRemoveButtonState();
	}

	{
		auto& table = m_keyRemappingDlg.GetTable();
		table.DeleteAllItems();
		int i = 0;
		for (auto&& [originalKey, newKey] : m_configuration->keysRemapping)
		{
			auto item = table.InsertItem(i++, originalKey.ToString().c_str());
			table.SetItemText(item, RebingingColumns::eNew, newKey.ToString().c_str());
		}
		m_keyRemappingDlg.UpdateRemoveButtonState();
	}

	m_checkChangeBrightness.SetCheck(m_configuration->changeBrightness);
	OnBnClickedCheckChangeBrightness();
	m_brightness.SetPositions(std::make_pair(m_configuration->brightnessLevel, m_configuration->brightnessLevel));

	auto& crosshair = m_configuration->crosshairSettings;
	m_checkboxShowCrosshair.SetCheck(crosshair.show);
	m_comboboxCrosshairSize.EnableWindow(crosshair.show);
	m_colorPickerCrosshairColor.EnableWindow(crosshair.show);
	m_comboCrosshairs.EnableWindow(crosshair.show);

	if (crosshair.customCrosshairName.empty())
		m_comboCrosshairs.SetCurSel((int)crosshair.type);
	else
	{
		auto selectedCrosshair = m_comboCrosshairs.FindStringExact(0, crosshair.customCrosshairName.c_str());
		if (selectedCrosshair == -1)
		{
			MessageBox(
				(L"Previously saved cursor '" + crosshair.customCrosshairName +
					L"' not found, cursor will be switched to default one.").c_str(),
				L"Custom cursor not found", MB_ICONWARNING);
			crosshair.customCrosshairName.clear();
			ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);

			m_comboCrosshairs.SetCurSel((int)crosshair.type);
		}
		else
			m_comboCrosshairs.SetCurSel(selectedCrosshair);
	}

	m_colorPickerCrosshairColor.SetColor(crosshair.color);
	ASSERT((int)m_configuration->crosshairSettings.size <= 3);
	m_comboboxCrosshairSize.SetCurSel((int)crosshair.size);

	InitCrosshairsList();

	UpdateDemoCrosshair();
}

void CActiveProcessToolkitTab::UpdateEnableButton()
{
	const auto& settings = ext::get_singleton<Settings>().process_toolkit;
	const bool enabled = settings.enabled;
	m_checkEnabled.UseDefaultBkColor(enabled);

	CString buttonText = enabled ? L"Disable" : L"Enable";
	buttonText += (L" active process toolkit(" + settings.enableBind.ToString() + L")").c_str();
	m_checkEnabled.SetWindowTextW(buttonText);
	m_checkEnabled.SetCheck(enabled ? BST_CHECKED : BST_UNCHECKED);
}

void CActiveProcessToolkitTab::AddNewActions(const Bind& keybind, Actions&& newActions)
{
	auto& actionsByBind = m_configuration->actionsByBind;
	auto it = actionsByBind.try_emplace(keybind, std::move(newActions));
	ASSERT(it.second);

	auto item = (int)std::distance(actionsByBind.begin(), it.first);

	auto& table = m_macrosesDlg.GetTable();

	item = table.InsertItem(item, it.first->first.ToString().c_str());
	std::wstring actions = it.first->second.description;
	if (actions.empty())
	{
		for (const auto& action : it.first->second.actions) {
			if (!actions.empty())
				actions += L" → ";

			if (action.delayInMilliseconds != 0)
				actions += L"(" + std::to_wstring(action.delayInMilliseconds) + L"ms) ";

			actions += action.ToString();
		}
	}
	table.SetItemText(item, MacrosesColumns::eMacros, actions.c_str());

	std::wostringstream str;
	if (it.first->second.enableRandomDelay)
		str << it.first->second.randomizeDelayMs;
	else
		str << L'-';
	table.SetItemText(item, MacrosesColumns::eRandomizeDelay, str.str().c_str());
	table.SelectItem(item);
}

void CActiveProcessToolkitTab::UpdateDemoCrosshair()
{
	CBitmap bitmap;
	try {
		process_toolkit::crosshair::LoadCrosshair(m_configuration->crosshairSettings, bitmap);
	}
	catch (...)
	{
		MessageBox(ext::ManageExceptionText(L"").c_str(), L"Failed to load crosshair for demo", MB_ICONERROR);
	}

	// Convert bitmap to icon because we don't want to change control size
	BITMAP bmp;
	bitmap.GetBitmap(&bmp);

	HBITMAP hbmMask = ::CreateCompatibleBitmap(::GetDC(NULL), bmp.bmWidth, bmp.bmHeight);

	ICONINFO ii = { 0 };
	ii.fIcon = TRUE;
	ii.hbmColor = bitmap;
	ii.hbmMask = hbmMask;

	if (m_demoIcon != nullptr)
		::DestroyIcon(m_demoIcon);
	m_demoIcon = ::CreateIconIndirect(&ii);

	::DeleteObject(hbmMask);

	m_crosshairDemo.SetIcon(m_demoIcon);
}

void CActiveProcessToolkitTab::InitCrosshairsList()
{
	m_crosshairs.clear();
	m_comboCrosshairs.ResetContent();

	process_toolkit::crosshair::Settings crosshairSettings;
	crosshairSettings.size = process_toolkit::crosshair::Size::eSmall;
	crosshairSettings.color = m_configuration->crosshairSettings.color;

	const auto insertCrosshair = [&](process_toolkit::crosshair::Type type) {
		try
		{
			m_crosshairs.emplace_back();

			crosshairSettings.type = type;
			process_toolkit::crosshair::LoadCrosshair(crosshairSettings, m_crosshairs.back());

			const std::wstring name = L"Crosshair " + std::to_wstring(m_crosshairs.size());
			m_comboCrosshairs.InsertItem((int)m_crosshairs.size() - 1, name.c_str(), (int)m_crosshairs.size() - 1);
		}
		catch (const std::exception&)
		{
			EXT_EXPECT(false) << ext::ManageExceptionText("Failed to load standard crosshair");
		}
	};

	insertCrosshair(process_toolkit::crosshair::Type::eDot);
	insertCrosshair(process_toolkit::crosshair::Type::eCross);
	insertCrosshair(process_toolkit::crosshair::Type::eCrossWithCircle);
	insertCrosshair(process_toolkit::crosshair::Type::eCircleWithCrossInside);
	insertCrosshair(process_toolkit::crosshair::Type::eCircleWithCrossAndDot);
	insertCrosshair(process_toolkit::crosshair::Type::eCrossWithCircleAndDot);
	insertCrosshair(process_toolkit::crosshair::Type::eCrossWithCircleAndCircleInside);
	insertCrosshair(process_toolkit::crosshair::Type::eDashedCircleAndDot);
	insertCrosshair(process_toolkit::crosshair::Type::eDashedBoxWithCross);

	constexpr int kMaxAllowedCrosshairSize = 32;

	std::map<std::wstring, int> crosshairsByName;

	std::wstring customCrosshairsLoadingError;

	namespace fs = std::filesystem;
	const auto crosshairsDir = fs::get_exe_directory() / L"res";
	if (fs::exists(crosshairsDir))
	{
		// Iterate over files in the folder
		for (const auto& entry : fs::directory_iterator(crosshairsDir))
		{
			if (!entry.is_regular_file())
				continue;

			const static std::regex filenameRegex("^crosshair_.*\\.(png|ico)");

			if (!std::regex_match(entry.path().filename().string(), filenameRegex))
				continue;

			process_toolkit::crosshair::Settings crosshairSettings;
			crosshairSettings.customCrosshairName = entry.path().filename().wstring();

			static std::set<std::wstring> customCrossairsWithErrors;
			if (customCrossairsWithErrors.find(crosshairSettings.customCrosshairName) != customCrossairsWithErrors.end())
				continue;

			CBitmap bitmap;
			try
			{
				process_toolkit::crosshair::LoadCrosshair(crosshairSettings, bitmap);
			}
			catch (...)
			{
				customCrossairsWithErrors.emplace(crosshairSettings.customCrosshairName);
				customCrosshairsLoadingError += ext::ManageExceptionText(L"") + L"\n";
				continue;
			}

			BITMAP bm;
			bitmap.GetBitmap(&bm);
			if (bm.bmWidth > 32 || bm.bmHeight > 32)
			{
				customCrossairsWithErrors.emplace(crosshairSettings.customCrosshairName);
				continue;
			}

			// resize it to fit the list
			process_toolkit::crosshair::ResizeCrosshair(bitmap, CSize(15, 15));

			m_crosshairs.emplace_back();
			m_crosshairs.back().Attach(bitmap.Detach());
			auto item = m_comboCrosshairs.InsertItem((int)m_crosshairs.size() - 1, crosshairSettings.customCrosshairName.c_str(), (int)m_crosshairs.size() - 1);
			crosshairsByName[crosshairSettings.customCrosshairName] = item;
		}
	}

	if (m_configuration->crosshairSettings.customCrosshairName.empty())
		m_comboCrosshairs.SetCurSel((int)m_configuration->crosshairSettings.type);
	else
	{
		if (auto it = crosshairsByName.find(m_configuration->crosshairSettings.customCrosshairName); it == crosshairsByName.end())
		{
			MessageBox(
				(L"Previously saved cursor '" + m_configuration->crosshairSettings.customCrosshairName +
				L"' not found, cursor will be switched to default one.").c_str(),
				L"Custom cursor not found", MB_ICONWARNING);
			m_configuration->crosshairSettings.customCrosshairName.clear();
			ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);

			m_comboCrosshairs.SetCurSel((int)m_configuration->crosshairSettings.type);
		}
		else
		{
			m_comboCrosshairs.SetCurSel(it->second);
		}
	}

	std::list<CBitmap*> crosshairs;
	for (auto& crosshair : m_crosshairs)
	{
		crosshairs.emplace_back(&crosshair);
	}
	m_comboCrosshairs.SetBitmapsList(crosshairs);

	// m_comboCrosshairs control recreated, we need to restore its anchers
	Layout::AnchorWindow(m_comboCrosshairs, *this, { AnchorSide::eRight }, AnchorSide::eRight, 100);
	Layout::AnchorWindow(m_comboCrosshairs, *this, { AnchorSide::eTop, AnchorSide::eBottom }, AnchorSide::eBottom, 100);
}

void CActiveProcessToolkitTab::addKeyToIgnore(const Key& key)
{
	CString itemText, bindText = key.ToString().c_str();
	for (int i = 0, count = m_comboAccidentalPress.GetCount(); i < count; ++i)
	{
		m_comboAccidentalPress.GetLBText(i, itemText);
		if (bindText == itemText)
		{
			m_comboAccidentalPress.SetCheck(i, true);
			return;
		}
	}

	auto res = m_comboAccidentalPress.AddString(bindText.GetString());
	m_comboAccidentalPress.SetItemDataPtr(res, new Key(key));
	m_comboAccidentalPress.SetCheck(m_comboAccidentalPress.GetCount() - 1, true);
}

void CActiveProcessToolkitTab::OnOK()
{
	// CDialogEx::OnOK();
}

void CActiveProcessToolkitTab::OnCancel()
{
	// CDialogEx::OnCancel();
}

void CActiveProcessToolkitTab::OnDestroy()
{
	for (int i = 0, count = m_comboAccidentalPress.GetCount(); i < count; ++i)
	{
		std::unique_ptr<Bind> deletter((Bind*)m_comboAccidentalPress.GetItemDataPtr(i));
	}

	__super::OnDestroy();
}

void CActiveProcessToolkitTab::OnBnClickedCheckActiveProcessToolkitEnable()
{
	auto& settings = ext::get_singleton<Settings>().process_toolkit;
	settings.enabled = !settings.enabled;

	UpdateEnableButton();

	ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);
}

void CActiveProcessToolkitTab::OnCbnSelchangeComboConfiguration()
{
	auto& settings = ext::get_singleton<Settings>().process_toolkit;
	settings.activeConfiguration = m_comboConfigurations.GetCurSel();

	UpdateControlsData();

	ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);
}

void CActiveProcessToolkitTab::OnBnClickedButtonAddConfiguration()
{
	auto configuration = CAddingProcessToolkitDlg::ExecModal(this);
	if (!configuration.has_value())
		return;

	auto& settings = ext::get_singleton<Settings>().process_toolkit;
	settings.activeConfiguration = (int)settings.processConfigurations.size();
	settings.processConfigurations.emplace_back(
		std::make_shared<process_toolkit::ProcessConfiguration>(std::move(configuration.value())));

	UpdateControlsData();
	ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);
}

void CActiveProcessToolkitTab::OnBnClickedButtonRenameConfiguration()
{
	auto& settings = ext::get_singleton<Settings>().process_toolkit;
	EXT_EXPECT(settings.activeConfiguration != -1);
	EXT_EXPECT(!settings.processConfigurations.empty());
	
	auto activetab = *std::next(settings.processConfigurations.begin(), settings.activeConfiguration);

	auto configuration = CAddingProcessToolkitDlg::ExecModal(this, activetab.get());
	if (!configuration.has_value())
		return;

	*activetab = std::move(configuration.value());

	m_comboConfigurations.ResetContent();
	for (const auto& config : settings.processConfigurations)
	{
		m_comboConfigurations.AddString(config->name.c_str());
	}
	m_comboConfigurations.SetCurSel(settings.activeConfiguration);

	ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);
}

void CActiveProcessToolkitTab::OnBnClickedButtonRemoveConfiguration()
{
	auto& settings = ext::get_singleton<Settings>().process_toolkit;
	EXT_EXPECT(settings.activeConfiguration != -1);
	EXT_EXPECT(!settings.processConfigurations.empty());

	settings.processConfigurations.erase(std::next(settings.processConfigurations.begin(), settings.activeConfiguration--));
	if (settings.processConfigurations.empty())
	{
		settings.processConfigurations.emplace_back(
			std::make_shared<process_toolkit::ProcessConfiguration>());
		settings.activeConfiguration = 0;
	}
	UpdateControlsData();

	ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);
}

void CActiveProcessToolkitTab::OnBnClickedCheckEnabled()
{
	m_configuration->enabled = m_enabled.GetCheck() == BST_CHECKED;

	ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);
}

void CActiveProcessToolkitTab::OnCbnEditchangeComboExeName()
{
	CString text;
	m_exeName.GetWindowTextW(text);

	std::optional<COLORREF> color = text.IsEmpty() ? std::optional<COLORREF>(RGB(255, 128, 128)) : std::nullopt;
	m_exeName.SetBkColor(std::move(color));
}

void CActiveProcessToolkitTab::OnCbnSelendChangedComboExeName()
{
	// When user select values from combobox OnCbnEditchangeComboExeName is not called
	OnCbnEditchangeComboExeName();

	CString text;
	m_exeName.GetWindowText(text);

	m_configuration->SetExeName(text.GetString());

	ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);
}

void CActiveProcessToolkitTab::OnCbnSetfocusComboExeName()
{
	auto hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE) {
		ASSERT(false);
		return;
	}
	EXT_DEFER(CloseHandle(hProcessSnap));

	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hProcessSnap, &pe32)) {
		EXT_DUMP_IF(true) << "Process32First failed, last error: " << GetLastError();
	}

	std::unordered_set<std::wstring> runningProcessesNames;
	do {
		runningProcessesNames.emplace(pe32.szExeFile);
	} while (Process32Next(hProcessSnap, &pe32));

	CString currentText;
	m_exeName.GetWindowText(currentText);
	m_exeName.ResetContent();
	for (const auto& name : runningProcessesNames)
	{
		m_exeName.AddString(name.c_str());
	}
	m_exeName.SetWindowText(currentText);
	m_exeName.SetEditSel(0, currentText.GetLength());
	m_exeName.AdjustComboBoxToContent();
}

void CActiveProcessToolkitTab::OnBnClickedCheckShowCrosshair()
{
	auto& show = m_configuration->crosshairSettings.show;
	show = m_checkboxShowCrosshair.GetCheck() == BST_CHECKED;

	m_comboboxCrosshairSize.EnableWindow(show);
	m_colorPickerCrosshairColor.EnableWindow(show);
	m_comboCrosshairs.EnableWindow(show);

	ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);
}

void CActiveProcessToolkitTab::OnCbnSelendokComboCrosshairSelection()
{
	auto selectedCrosshair = m_comboCrosshairs.GetCurSel();

	bool selectedCustomCrosshair = selectedCrosshair > (int)process_toolkit::crosshair::Type::eDashedBoxWithCross;
	if (selectedCustomCrosshair)
	{
		CString text;
		m_comboCrosshairs.GetWindowText(text);

		m_configuration->crosshairSettings.customCrosshairName = text;
	}
	else
	{
		m_configuration->crosshairSettings.customCrosshairName.clear();
		m_configuration->crosshairSettings.type = process_toolkit::crosshair::Type(selectedCrosshair);
	}

	m_comboboxCrosshairSize.EnableWindow(!selectedCustomCrosshair);

	ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);

	UpdateDemoCrosshair();
}

void CActiveProcessToolkitTab::OnBnClickedMfccolorbuttonCrosshairColor()
{
	m_configuration->crosshairSettings.color = m_colorPickerCrosshairColor.GetColor();

	std::list<CBitmap*> crosshairs;
	for (auto& crosshair : m_crosshairs)
	{
		process_toolkit::crosshair::ChangeCrosshairColor(crosshair, m_configuration->crosshairSettings.color);
		crosshairs.emplace_back(&crosshair);
	}
	m_comboCrosshairs.SetBitmapsList(crosshairs);

	ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);

	UpdateDemoCrosshair();
}

void CActiveProcessToolkitTab::OnCbnSelendokComboCrosshairSize()
{
	m_configuration->crosshairSettings.size = process_toolkit::crosshair::Size(m_comboboxCrosshairSize.GetCurSel());

	ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);

	UpdateDemoCrosshair();
}

void CActiveProcessToolkitTab::OnBnClickedButtonAccidentalPressAddCustom()
{
	auto keyToAdd = CInputEditorDlg::EditKey(this);
	if (!keyToAdd.has_value())
		return;

	addKeyToIgnore(keyToAdd.value());

	auto& keys = m_configuration->keysToIgnoreAccidentalPress;
	auto it = std::find_if(keys.cbegin(), keys.cend(), [code = keyToAdd->vkCode](const auto& a) { return a.vkCode == code; });
	if (it == keys.cend())
	{
		m_configuration->keysToIgnoreAccidentalPress.emplace_back(*keyToAdd);
		ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);
	}
}

void CActiveProcessToolkitTab::OnCbnSelchangeComboAccidentalPress()
{
	m_configuration->keysToIgnoreAccidentalPress.clear();
	for (int i = 0, count = m_comboAccidentalPress.GetCount(); i < count; ++i)
	{
		if (m_comboAccidentalPress.GetCheck(i))
		{
			Key* key = (Key*)m_comboAccidentalPress.GetItemDataPtr(i);
			m_configuration->keysToIgnoreAccidentalPress.emplace_back(*key);
		}
	}
	ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);
}

void CActiveProcessToolkitTab::OnBnClickedCheckChangeBrightness()
{
	m_configuration->changeBrightness = m_checkChangeBrightness.GetCheck();

	const bool brightnessControllSupported = DisplayBrightnessController::BrightnessControlAvailable();
	if (!brightnessControllSupported)
	{
		m_checkChangeBrightness.EnableWindow(FALSE);
		m_brightness.EnableWindow(FALSE);
	}
	else
	{
		m_checkChangeBrightness.EnableWindow(TRUE);
		m_brightness.EnableWindow(m_configuration->changeBrightness);
	}

	CString tooltip;
	if (!brightnessControllSupported)
		tooltip = L"Can't find any display which can support brightness control.\n";
	tooltip += L"This feature works only if you enabled DDC/CI on your HDMI monitor or if you use build-in monitor.";
	controls::SetTooltip(m_staticBrightnessInfo, tooltip);

	ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);
}

void CActiveProcessToolkitTab::OnTRBNThumbPosChangingSliderBrightness(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMTRBTHUMBPOSCHANGING* pNMTPC = reinterpret_cast<NMTRBTHUMBPOSCHANGING*>(pNMHDR);

	ASSERT(pNMTPC->nReason == (int)CSlider::TrackMode::TRACK_LEFT);
	m_configuration->brightnessLevel = pNMTPC->dwPos;

	ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);

	*pResult = 0;
}

void CActiveProcessToolkitTab::OnSettingsChanged(ISettingsChanged::ChangedType changedType)
{
	if (changedType == ISettingsChanged::ChangedType::eProcessToolkit)
		UpdateEnableButton();
}
