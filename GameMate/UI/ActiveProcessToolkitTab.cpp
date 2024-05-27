#include "pch.h"

#include <algorithm>
#include <regex>

#include "tlhelp32.h"
#include "afxdialogex.h"
#include "resource.h"

#include "ActiveProcessToolkitTab.h"
#include "EditActions.h"
#include "EditBindDlg.h"
#include "AddingProcessToolkitDlg.h"

#include "core/Crosshairs.h"

#include <ext/scope/defer.h>
#include <ext/std/filesystem.h>
#include <ext/std/string.h>

#include <Controls/DefaultWindowProc.h>
#include "Controls/Layout/Layout.h"
#include <Controls/Tooltip/ToolTip.h>

using namespace controls::list::widgets;

namespace {

enum Columns {
	eKeybind = 0,
	eMacros,
	eRandomizeDelay
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
	DDX_Control(pDX, IDC_LIST_ACTIONS, m_listActions);
	DDX_Control(pDX, IDC_COMBO_CROSSHAIR_SELECTION, m_comboCrosshairs);
	DDX_Control(pDX, IDC_CHECK_SHOW_CROSSHAIR, m_checkboxShowCrosshair);
	DDX_Control(pDX, IDC_COMBO_CROSSHAIR_SIZE, m_comboboxCrosshairSize);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_CROSSHAIR_COLOR, m_colorPickerCrosshairColor);
	DDX_Control(pDX, IDC_STATIC_CROSSHAIR_INFO, m_staticCrosshairInfo);
	DDX_Control(pDX, IDC_CHECK_DISABLE_WIN, m_checkDisableWinButton);
	DDX_Control(pDX, IDC_STATIC_CROSSHAIR_DEMO, m_crosshairDemo);
	DDX_Control(pDX, IDC_COMBO_CONFIGURATION, m_comboConfigurations);
	DDX_Control(pDX, IDC_CHECK_ACTIVE_PROCESS_TOOLKIT_ENABLE, m_checkEnabled);
	DDX_Control(pDX, IDC_BUTTON_ADD_CONFIGURATION, m_buttonAddConfiguration);
}

BEGIN_MESSAGE_MAP(CActiveProcessToolkitTab, CDialogEx)
	ON_BN_CLICKED(IDC_CHECK_ACTIVE_PROCESS_TOOLKIT_ENABLE, &CActiveProcessToolkitTab::OnBnClickedCheckActiveProcessToolkitEnable)
	ON_CBN_SELCHANGE(IDC_COMBO_CONFIGURATION, &CActiveProcessToolkitTab::OnCbnSelchangeComboConfiguration)
	ON_BN_CLICKED(IDC_BUTTON_ADD_CONFIGURATION, &CActiveProcessToolkitTab::OnBnClickedButtonAddConfiguration)
	ON_BN_CLICKED(IDC_BUTTON_RENAME_CONFIGURATION, &CActiveProcessToolkitTab::OnBnClickedButtonRenameConfiguration)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_CONFIGURATION, &CActiveProcessToolkitTab::OnBnClickedButtonRemoveConfiguration)
	ON_BN_CLICKED(IDC_CHECK_ENABLED, &CActiveProcessToolkitTab::OnBnClickedCheckEnabled)
	ON_CBN_SELENDOK(IDC_COMBO_EXE_NAME, &CActiveProcessToolkitTab::OnCbnSelendokComboExeName)
	ON_CBN_SETFOCUS(IDC_COMBO_EXE_NAME, &CActiveProcessToolkitTab::OnCbnSetfocusComboExeName)
	ON_BN_CLICKED(IDC_CHECK_DISABLE_WIN, &CActiveProcessToolkitTab::OnBnClickedCheckDisableWin)
	ON_BN_CLICKED(IDC_BUTTON_ADD_MACROS, &CActiveProcessToolkitTab::OnBnClickedButtonAddMacros)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_MACROS, &CActiveProcessToolkitTab::OnBnClickedButtonRemoveMacros)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_ACTIONS, &CActiveProcessToolkitTab::OnLvnItemchangedListActions)
	ON_BN_CLICKED(IDC_CHECK_SHOW_CROSSHAIR, &CActiveProcessToolkitTab::OnBnClickedCheckShowCrosshair)
	ON_CBN_SELENDOK(IDC_COMBO_CROSSHAIR_SELECTION, &CActiveProcessToolkitTab::OnCbnSelendokComboCrosshairSelection)
	ON_BN_CLICKED(IDC_MFCCOLORBUTTON_CROSSHAIR_COLOR, &CActiveProcessToolkitTab::OnBnClickedMfccolorbuttonCrosshairColor)
	ON_CBN_SELENDOK(IDC_COMBO_CROSSHAIR_SIZE, &CActiveProcessToolkitTab::OnCbnSelendokComboCrosshairSize)
	ON_CBN_EDITCHANGE(IDC_COMBO_EXE_NAME, &CActiveProcessToolkitTab::OnCbnEditchangeComboExeName)
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
	m_exeName.SetCueBanner(L"Enter process name...(example: steam.exe)");

	controls::SetTooltip(m_checkDisableWinButton,
		L"If enabled: ignore single press on windows button(every 1s) which prevents game from loosing focus accidentally");

	{
		CRect rect;
		m_staticCrosshairInfo.GetClientRect(rect);
		HICON icon;
		auto res = SUCCEEDED(::LoadIconWithScaleDown(NULL, IDI_INFORMATION, rect.Height(), rect.Height(), &icon));
		ASSERT(res);
		m_staticCrosshairInfo.ModifyStyle(0, SS_ICON);
		m_staticCrosshairInfo.SetIcon(icon);

		controls::SetTooltip(
			m_staticCrosshairInfo,
			L"If you want to add your own crosshair put file with name crosshair_X with .png or .ico extenstion to the folder:\n"
			L"$GAME_MATE_FOLDER$\\res\n"
			L"Note: all images bigger than 32x32 will be ignored.");
	}

	m_comboboxCrosshairSize.AddString(L"Small(16x16)");
    m_comboboxCrosshairSize.AddString(L"Medium(24x24)");
    m_comboboxCrosshairSize.AddString(L"Large(32x32)");

	{
		m_crosshairDemo.ModifyStyle(0, SS_ICON);
		controls::SetTooltip(m_crosshairDemo, L"Demo of the crosshair");
	}

	CRect rect;
	m_listActions.GetClientRect(rect);

	constexpr int kKeybindColumnWidth = 80;
	constexpr int kRandomizeDelayColumnWidth = 50;
	m_listActions.InsertColumn(Columns::eKeybind, L"Keybind", LVCFMT_CENTER, kKeybindColumnWidth);
	m_listActions.InsertColumn(Columns::eMacros, L"Macros", LVCFMT_CENTER, rect.Width() - kKeybindColumnWidth - kRandomizeDelayColumnWidth);
	m_listActions.InsertColumn(Columns::eRandomizeDelay, L"Randomize delay(%)", LVCFMT_CENTER, kRandomizeDelayColumnWidth);
	m_listActions.SetProportionalResizingColumns({ Columns::eMacros });
	
	m_listActions.setSubItemEditorController(Columns::eKeybind,
		[&](CListCtrl* pList, CWnd* parentWindow, const LVSubItemParams* pParams)
		{
			auto& actionsByBind = m_configuration->actionsByBind;

			ASSERT((int)actionsByBind.size() > pParams->iItem);
			auto editableActionsIt = std::next(actionsByBind.begin(), pParams->iItem);

			auto bind = CEditBindDlg::EditBind(parentWindow, editableActionsIt->first);
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
				m_listActions.DeleteItem(std::max<int>(pParams->iItem, sameItem));
				m_listActions.DeleteItem(std::min<int>(pParams->iItem, sameItem));

				AddNewActions(std::move(bind.value()), std::move(actions));
			}
			else
			{
				auto currentActions = std::move(editableActionsIt->second);
				actionsByBind.erase(editableActionsIt);
				m_listActions.DeleteItem(pParams->iItem);
				AddNewActions(std::move(bind.value()), std::move(currentActions));
			}

			ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);

			return nullptr;
		});
	const auto ActionsEdit = [&](CListCtrl* pList, CWnd* parentWindow, const LVSubItemParams* pParams)
		{
			auto& actionsByBind = m_configuration->actionsByBind;

			ASSERT((int)actionsByBind.size() > pParams->iItem);
			auto editableActionsIt = std::next(actionsByBind.begin(), pParams->iItem);

			auto actions = CActionsEditDlg::ExecModal(this, editableActionsIt->second);
			if (!actions.has_value())
				return nullptr;

			auto bind = editableActionsIt->first;

			actionsByBind.erase(editableActionsIt);
			m_listActions.DeleteItem(pParams->iItem);

			AddNewActions(std::move(bind), std::move(actions.value()));

			ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);

			return nullptr;
		};
	m_listActions.setSubItemEditorController(Columns::eMacros, ActionsEdit);
	m_listActions.setSubItemEditorController(Columns::eRandomizeDelay, ActionsEdit);

	UpdateControlsData();

	return TRUE;
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

	constexpr UINT notDisalableChilds[] = {
		IDC_CHECK_ACTIVE_PROCESS_TOOLKIT_ENABLE,
		IDC_STATIC_CONFIGURATION,
		IDC_COMBO_CONFIGURATION,
		IDC_BUTTON_ADD_CONFIGURATION,
	};

	// Disable editable controls if no configuration selected
	bool enableStatus = (settings.activeConfiguration != -1) ? TRUE : FALSE;
	CWnd* pChildWnd = GetWindow(GW_CHILD);
	while (pChildWnd)
	{
		const auto controlId = pChildWnd->GetDlgCtrlID();

		bool canChangeEnableState = true;
		for (const auto id : notDisalableChilds)
		{
			if (controlId == id)
			{
				canChangeEnableState = false;
				break;
			}
		}
		if (canChangeEnableState)
			pChildWnd->ShowWindow(!!m_configuration ? SW_SHOW : SW_HIDE);

		// Move to the next child control
		pChildWnd = pChildWnd->GetNextWindow(GW_HWNDNEXT);
	}

	m_comboConfigurations.ResetContent();
	for (const auto& config : settings.processConfigurations)
	{
		m_comboConfigurations.AddString(config->configurationName.c_str());
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
	m_exeName.SetWindowTextW(m_configuration->exeName.c_str());
	// updating control BK color
	OnCbnEditchangeComboExeName();
	m_checkDisableWinButton.SetCheck(m_configuration->disableWinButton);

	m_listActions.DeleteAllItems();
	decltype(m_configuration->actionsByBind) current;
	std::swap(current, m_configuration->actionsByBind);
	for (auto&& [bind, actionsByBind] : current)
	{
		AddNewActions(bind, std::move(actionsByBind));
	}
	GetDlgItem(IDC_BUTTON_REMOVE_MACROS)->EnableWindow(m_listActions.GetSelectedCount() > 0);

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
	const bool enabled = ext::get_singleton<Settings>().process_toolkit.enabled;
	m_checkEnabled.UseDefaultBkColor(enabled);

	CString buttonText = enabled ? L"Disable" : L"Enable";
	buttonText += L" active process toolkit(Shift + F8)";
	m_checkEnabled.SetWindowTextW(buttonText);
	m_checkEnabled.SetCheck(enabled ? BST_CHECKED : BST_UNCHECKED);
}

void CActiveProcessToolkitTab::AddNewActions(const Bind& keybind, Actions&& newActions)
{
	auto& actionsByBind = m_configuration->actionsByBind;
	auto it = actionsByBind.try_emplace(keybind, std::move(newActions));
	ASSERT(it.second);

	auto item = (int)std::distance(actionsByBind.begin(), it.first);

	item = m_listActions.InsertItem(item, it.first->first.ToString().c_str());
	std::wstring actions;
	for (const auto& action : it.first->second.actions) {
		if (!actions.empty())
			actions += L" → ";

		if (action.delayInMilliseconds != 0)
			actions += L"(" + std::to_wstring(action.delayInMilliseconds) + L"ms) ";

		actions += action.ToString();
	}
	m_listActions.SetItemText(item, Columns::eMacros, actions.c_str());

	std::wostringstream str;
	str << it.first->second.randomizeDelays;
	m_listActions.SetItemText(item, Columns::eRandomizeDelay, str.str().c_str());

	m_listActions.SelectItem(item);
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
			EXT_EXPECT(false) << ext::ManageExceptionText("Failed to load standart crosshair");
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

	// m_comboCrosshairs control recreated, we need to restor its anchers
	Layout::AnchorWindow(m_comboCrosshairs, *this, { AnchorSide::eRight }, AnchorSide::eRight, 100);
	Layout::AnchorWindow(m_comboCrosshairs, *this, { AnchorSide::eTop, AnchorSide::eBottom }, AnchorSide::eBottom, 100);
}

void CActiveProcessToolkitTab::OnOK()
{
	// CDialogEx::OnOK();
}

void CActiveProcessToolkitTab::OnCancel()
{
	// CDialogEx::OnCancel();
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
	settings.activeConfiguration = settings.processConfigurations.size();
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
		m_comboConfigurations.AddString(config->configurationName.c_str());
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

void CActiveProcessToolkitTab::OnCbnSelendokComboExeName()
{
	OnCbnEditchangeComboExeName();

	CString text;
	m_exeName.GetWindowText(text);

	m_configuration->exeName = text;

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

	// Extract exe names which already used in other tabs
	for (const auto& configuration : ext::get_singleton<Settings>().process_toolkit.processConfigurations)
	{
		if (configuration == m_configuration)
			continue;
		runningProcessesNames.erase(configuration->exeName);
	}

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

void CActiveProcessToolkitTab::OnBnClickedCheckDisableWin()
{
	m_configuration->disableWinButton = m_checkDisableWinButton.GetCheck() == BST_CHECKED;
	ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);
}

void CActiveProcessToolkitTab::OnBnClickedButtonAddMacros()
{
	auto bind = CEditBindDlg::EditBind(this);
	if (!bind.has_value())
		return;

	const auto it = m_configuration->actionsByBind.find(bind.value());
	const bool actionExists = it != m_configuration->actionsByBind.end();

	Actions editableActions;
	if (actionExists)
	{
		auto res = MessageBox((L"Do you want to edit action '" + bind->ToString() + L"'?").c_str(), L"Action already has actionsByBind", MB_OKCANCEL | MB_ICONWARNING);
		if (res == IDCANCEL)
		{
			m_listActions.ClearSelection();
			m_listActions.SelectItem((int)std::distance(m_configuration->actionsByBind.begin(), it));
			return;
		}

		editableActions = it->second;
	}
	else {
		// Adding new item to the table to show user what we adding
		auto item = m_listActions.InsertItem(m_listActions.GetItemCount(), bind->ToString().c_str());
		m_listActions.SelectItem(item);
	}

	auto newActions = CActionsEditDlg::ExecModal(this, editableActions);

	// removing previously added item with bind name
	if (!actionExists)
		m_listActions.DeleteItem(m_listActions.GetItemCount() - 1);

	if (!newActions.has_value())
		return;

	// removing old copy of eddited actionsByBind
	if (actionExists)
	{
		auto sameItem = (int)std::distance(m_configuration->actionsByBind.begin(), it);
		m_configuration->actionsByBind.erase(it);
		m_listActions.DeleteItem(sameItem);
	}

	AddNewActions(bind.value(), std::move(newActions.value()));

	ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);
}

void CActiveProcessToolkitTab::OnBnClickedButtonRemoveMacros()
{
	std::vector<int> selectedActions = m_listActions.GetSelectedItems();

	for (auto it = selectedActions.rbegin(), end = selectedActions.rend(); it != end; ++it)
	{
		m_configuration->actionsByBind.erase(std::next(m_configuration->actionsByBind.begin(), *it));
		m_listActions.DeleteItem(*it);
	}

	ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);
}

void CActiveProcessToolkitTab::OnLvnItemchangedListActions(NMHDR* pNMHDR, LRESULT* pResult)
{
	GetDlgItem(IDC_BUTTON_REMOVE_MACROS)->EnableWindow(m_listActions.GetSelectedCount() > 0);
	*pResult = 0;
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

void CActiveProcessToolkitTab::OnSettingsChanged(ISettingsChanged::ChangedType changedType)
{
	if (changedType == ISettingsChanged::ChangedType::eProcessToolkit)
		UpdateEnableButton();
}
