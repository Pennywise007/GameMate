#include "pch.h"

#include <algorithm>
#include <regex>

#include "GameMate.h"
#include "tlhelp32.h"
#include "afxdialogex.h"
#include "GameSettingsDlg.h"
#include "MacrosEditDlg.h"
#include "ActionEditDlg.h"

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
	eActions,
	eRandomizeDelay
};

} // namespace

IMPLEMENT_DYNAMIC(CGameSettingsDlg, CDialogEx)

CGameSettingsDlg::CGameSettingsDlg(std::shared_ptr<TabConfiguration> configuration, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TAB_GAME_SETTINGS, pParent)
	, m_configuration(std::move(configuration))
{
}

CGameSettingsDlg::~CGameSettingsDlg()
{
	if (m_demoIcon != nullptr)
		::DestroyIcon(m_demoIcon);
}

void CGameSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHECK_ENABLED, m_enabled);
	DDX_Control(pDX, IDC_COMBO_EXE_NAME, m_exeName);
	DDX_Control(pDX, IDC_LIST_MACROSES, m_listMacroses);
	DDX_Control(pDX, IDC_COMBO_CROSSHAIR_SELECTION, m_comboCrosshairs);
	DDX_Control(pDX, IDC_CHECK_USE, m_checkboxShowCrosshair);
	DDX_Control(pDX, IDC_COMBO_CROSSHAIR_SIZE, m_comboboxCrosshairSize);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_CROSSHAIR_COLOR, m_colorPickerCrosshairColor);
	DDX_Control(pDX, IDC_STATIC_CROSSHAIR_INFO, m_staticCrosshairInfo);
	DDX_Control(pDX, IDC_CHECK_DISABLE_WIN, m_checkDisableWinButton);
	DDX_Control(pDX, IDC_STATIC_CROSSHAIR_DEMO, m_crosshairDemo);
}

BEGIN_MESSAGE_MAP(CGameSettingsDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CGameSettingsDlg::OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, &CGameSettingsDlg::OnBnClickedButtonRemove)
	ON_BN_CLICKED(IDC_CHECK_ENABLED, &CGameSettingsDlg::OnBnClickedCheckEnabled)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_MACROSES, &CGameSettingsDlg::OnLvnItemchangedListMacroses)
	ON_CBN_SELENDOK(IDC_COMBO_EXE_NAME, &CGameSettingsDlg::OnCbnSelendokComboExeName)
	ON_BN_CLICKED(IDC_CHECK_USE, &CGameSettingsDlg::OnBnClickedCheckUse)
	ON_BN_CLICKED(IDC_MFCCOLORBUTTON_CROSSHAIR_COLOR, &CGameSettingsDlg::OnBnClickedMfccolorbuttonCrosshairColor)
	ON_CBN_SELENDOK(IDC_COMBO_CROSSHAIR_SELECTION, &CGameSettingsDlg::OnCbnSelendokComboCrosshairSelection)
	ON_CBN_SELENDOK(IDC_COMBO_CROSSHAIR_SIZE, &CGameSettingsDlg::OnCbnSelendokComboCrosshairSize)
	ON_BN_CLICKED(IDC_CHECK_DISABLE_WIN, &CGameSettingsDlg::OnBnClickedCheckDisableWin)
	ON_CBN_DROPDOWN(IDC_COMBO_EXE_NAME, &CGameSettingsDlg::OnCbnDropdownComboExeName)
END_MESSAGE_MAP()

BOOL CGameSettingsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	GetDlgItem(IDC_STATIC_CROSSHAIR_DEMO)->ModifyStyle(0, SS_REALSIZECONTROL);
	GetDlgItem(IDC_STATIC_CROSSHAIR_INFO)->ModifyStyle(0, SS_REALSIZECONTROL);

	{
		CWnd* exenameEdit = m_exeName.GetWindow(GW_CHILD);
		ASSERT(exenameEdit);
		DefaultWindowProc::OnWindowMessage(*exenameEdit, WM_MOUSEACTIVATE, [&](HWND hWnd, WPARAM wParam, LPARAM lParam, LRESULT& result) {
			// Show dropdown on mouse click on edit
			m_exeName.ShowDropDown();
		}, this);
	}

	m_enabled.SetCheck(m_configuration->enabled ? BST_CHECKED : BST_UNCHECKED);
	m_exeName.SetWindowTextW(m_configuration->exeName.c_str());
	m_checkDisableWinButton.SetCheck(m_configuration->disableWinButton ? BST_CHECKED : BST_UNCHECKED);
	controls::SetTooltip(m_checkDisableWinButton,
		L"If enabled: ignore single press on windows button(every 1s) which prevents game from loosing focus accidentally");

	GetDlgItem(IDC_BUTTON_REMOVE)->EnableWindow(FALSE);

	m_checkboxShowCrosshair.SetCheck(m_configuration->crosshairSettings.show ? BST_CHECKED : BST_UNCHECKED);
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
	ASSERT((int)m_configuration->crosshairSettings.size <= 3);
    m_comboboxCrosshairSize.SetCurSel((int)m_configuration->crosshairSettings.size);

	m_colorPickerCrosshairColor.SetColor(m_configuration->crosshairSettings.color);

	{
		InitCrosshairsList();

		m_crosshairDemo.ModifyStyle(0, SS_ICON);
		controls::SetTooltip(m_crosshairDemo, L"Demo of the crosshair");

		UpdateDemoCrosshair();
	}

	OnBnClickedCheckUse();

	CRect rect;
	m_listMacroses.GetClientRect(rect);

	constexpr int kKeybindColumnWidth = 50;
	constexpr int kRandomizeDelayColumnWidth = 120;
	m_listMacroses.InsertColumn(Columns::eKeybind, L"Keybind", LVCFMT_CENTER, kKeybindColumnWidth);
	m_listMacroses.InsertColumn(Columns::eActions, L"Actions", LVCFMT_CENTER, rect.Width() - kKeybindColumnWidth - kRandomizeDelayColumnWidth);
	m_listMacroses.InsertColumn(Columns::eRandomizeDelay, L"Randomize delay(%)", LVCFMT_CENTER, kRandomizeDelayColumnWidth);

	LVCOLUMN colInfo;
	colInfo.mask = LVCF_FMT;
	m_listMacroses.GetColumn(Columns::eKeybind, &colInfo);
	colInfo.fmt |= LVCFMT_FIXED_WIDTH | LVCFMT_CENTER;
	m_listMacroses.SetColumn(Columns::eKeybind, &colInfo);

	m_listMacroses.SetProportionalResizingColumns({ Columns::eActions });
	
	m_listMacroses.setSubItemEditorController(Columns::eKeybind,
		[&](CListCtrl* pList, CWnd* parentWindow, const LVSubItemParams* pParams)
		{
			auto& macroses = m_configuration->macrosByBind;

			ASSERT((int)macroses.size() > pParams->iItem);
			auto editableMacrosIt = std::next(macroses.begin(), pParams->iItem);

			auto bind = CActionEditDlg::EditBind(parentWindow, editableMacrosIt->first);
			if (!bind.has_value())
				return nullptr;

			if (auto sameBindIt = macroses.find(bind.value()); sameBindIt != macroses.end())
			{
				if (MessageBox((L"Bind '" + bind->ToString() + L"' already exists, do you want to replace it?").c_str(),
							   L"This bind already exist", MB_ICONWARNING | MB_OKCANCEL) == IDCANCEL)
					return nullptr;

				auto sameItem = (int)std::distance(macroses.begin(), sameBindIt);
				auto macros = std::move(editableMacrosIt->second);
				macroses.erase(editableMacrosIt);
				macroses.erase(sameBindIt);
				m_listMacroses.DeleteItem(std::max<int>(pParams->iItem, sameItem));
				m_listMacroses.DeleteItem(std::min<int>(pParams->iItem, sameItem));

				AddNewMacros(std::move(bind.value()), std::move(macros));
			}
			else
			{
				auto currentMacros = std::move(editableMacrosIt->second);
				macroses.erase(editableMacrosIt);
				m_listMacroses.DeleteItem(pParams->iItem);
				AddNewMacros(std::move(bind.value()), std::move(currentMacros));
			}

			ext::send_event(&ISettingsChanged::OnSettingsChangedByUser);

			return nullptr;
		});
	const auto macrosEdit = [&](CListCtrl* pList, CWnd* parentWindow, const LVSubItemParams* pParams)
		{
			auto& macroses = m_configuration->macrosByBind;

			ASSERT((int)macroses.size() > pParams->iItem);
			auto editableMacrosIt = std::next(macroses.begin(), pParams->iItem);

			auto macros = CMacrosEditDlg(editableMacrosIt->second, this).ExecModal();
			if (!macros.has_value())
				return nullptr;

			auto bind = editableMacrosIt->first;

			macroses.erase(editableMacrosIt);
			m_listMacroses.DeleteItem(pParams->iItem);

			AddNewMacros(std::move(bind), std::move(macros.value()));

			ext::send_event(&ISettingsChanged::OnSettingsChangedByUser);

			return nullptr;
		};
	m_listMacroses.setSubItemEditorController(Columns::eActions, macrosEdit);
	m_listMacroses.setSubItemEditorController(Columns::eRandomizeDelay, macrosEdit);

	auto currentMacroses = std::move(m_configuration->macrosByBind);
	m_configuration->macrosByBind.clear();
	for (auto&& [bind, macros] : currentMacroses)
	{
		AddNewMacros(bind, std::move(macros));
	}

	return TRUE;
}

void CGameSettingsDlg::OnOK()
{
	// CDialogEx::OnOK();
}

void CGameSettingsDlg::OnCancel()
{
	// CDialogEx::OnCancel();
}

void CGameSettingsDlg::OnBnClickedButtonAdd()
{
	auto bind = CActionEditDlg::EditBind(this);
	if (!bind.has_value())
		return;

	const auto it = m_configuration->macrosByBind.find(bind.value());
	const bool actionExists = it != m_configuration->macrosByBind.end();

	Macros editableMacros;
	if (actionExists)
	{
		auto res = MessageBox((L"Do you want to edit action '" + bind->ToString() + L"'?").c_str(), L"Action already has macros", MB_OKCANCEL | MB_ICONWARNING);
		if (res == IDCANCEL)
		{
			m_listMacroses.ClearSelection();
			m_listMacroses.SelectItem((int)std::distance(m_configuration->macrosByBind.begin(), it));
			return;
		}

		editableMacros = it->second;
	}
	else {
		// Adding new item to the table to show user what we adding
		auto item = m_listMacroses.InsertItem(m_listMacroses.GetItemCount(), bind->ToString().c_str());
		m_listMacroses.SelectItem(item);
	}

	auto newMacros = CMacrosEditDlg(editableMacros, this).ExecModal();

	if (!actionExists)
		m_listMacroses.DeleteItem(m_listMacroses.GetItemCount() - 1);

	if (!newMacros.has_value())
		return;

	AddNewMacros(bind.value(), std::move(newMacros.value()));

	ext::send_event(&ISettingsChanged::OnSettingsChangedByUser);
}

void CGameSettingsDlg::OnBnClickedButtonRemove()
{
	std::vector<int> selectedActions;
	m_listMacroses.GetSelectedList(selectedActions, true);

	for (auto it = selectedActions.rbegin(), end = selectedActions.rend(); it != end; ++it)
	{
		m_configuration->macrosByBind.erase(std::next(m_configuration->macrosByBind.begin(), *it));
		m_listMacroses.DeleteItem(*it);
	}

	ext::send_event(&ISettingsChanged::OnSettingsChangedByUser);
}

void CGameSettingsDlg::OnBnClickedCheckEnabled()
{
	m_configuration->enabled = m_enabled.GetCheck() == BST_CHECKED;

	ext::send_event(&ISettingsChanged::OnSettingsChangedByUser);
}

void CGameSettingsDlg::OnCbnSelendokComboExeName()
{
	auto item = m_exeName.GetCurSel();
	CString text;
	if (item != -1)
		m_exeName.GetLBText(m_exeName.GetCurSel(), text);
	else
		m_exeName.GetWindowText(text);

	m_configuration->exeName = text;

	ext::send_event(&ISettingsChanged::OnSettingsChangedByUser);
}

void CGameSettingsDlg::OnCbnDropdownComboExeName()
{
	auto hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE) {
		return;
	}
	EXT_DEFER(CloseHandle(hProcessSnap));

	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hProcessSnap, &pe32)) {
		EXT_DUMP_IF(true) << "Process32First failed, last error: " << GetLastError();
		return;
	}

	std::unordered_set<std::wstring> runningProcessesNames;
	do {
		runningProcessesNames.emplace(pe32.szExeFile);
	} while (Process32Next(hProcessSnap, &pe32));

	// Extract exe names which already used in other tabs
	for (const auto& tab : ext::get_singleton<Settings>().tabs)
	{
		if (tab == m_configuration)
			continue;
		runningProcessesNames.erase(tab->exeName);
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
}

void CGameSettingsDlg::AddNewMacros(const Bind& keybind, Macros&& newMacros)
{
	auto& macros = m_configuration->macrosByBind;
	auto it = macros.try_emplace(keybind, std::move(newMacros));
	ASSERT(it.second);

	auto item = (int)std::distance(macros.begin(), it.first);

	item = m_listMacroses.InsertItem(item, it.first->first.ToString().c_str());
	std::wstring actions;
	for (const auto& action : it.first->second.actions) {
		if (!actions.empty())
			actions += L" → ";

		if (action.delayInMilliseconds != 0)
			actions += L"(" + std::to_wstring(action.delayInMilliseconds) + L"ms) ";

		actions += action.ToString();
	}
	m_listMacroses.SetItemText(item, Columns::eActions, actions.c_str());

	std::wostringstream str;
	str << it.first->second.randomizeDelays;
	m_listMacroses.SetItemText(item, Columns::eRandomizeDelay, str.str().c_str());

	m_listMacroses.SelectItem(item);
}

void CGameSettingsDlg::UpdateDemoCrosshair()
{
	CBitmap bitmap;
	try {
		crosshair::LoadCrosshair(m_configuration->crosshairSettings, bitmap);
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

void CGameSettingsDlg::InitCrosshairsList()
{
	crosshair::Settings crosshairSettings;
	crosshairSettings.size = crosshair::Size::eSmall;
	crosshairSettings.color = m_configuration->crosshairSettings.color;

	const auto insertCrosshair = [&](crosshair::Type type) {
		try
		{
			m_crosshairs.emplace_back();

			crosshairSettings.type = type;
			crosshair::LoadCrosshair(crosshairSettings, m_crosshairs.back());

			const std::wstring name = L"Crosshair " + std::to_wstring(m_crosshairs.size());
			m_comboCrosshairs.InsertItem((int)m_crosshairs.size() - 1, name.c_str(), (int)m_crosshairs.size() - 1);
		}
		catch (const std::exception&)
		{
			EXT_EXPECT(false) << ext::ManageExceptionText("Failed to load standart crosshair");
		}
	};

	insertCrosshair(crosshair::Type::eDot);
	insertCrosshair(crosshair::Type::eCross);
	insertCrosshair(crosshair::Type::eCrossWithCircle);
	insertCrosshair(crosshair::Type::eCircleWithCrossInside);
	insertCrosshair(crosshair::Type::eCircleWithCrossAndDot);
	insertCrosshair(crosshair::Type::eCrossWithCircleAndDot);
	insertCrosshair(crosshair::Type::eCrossWithCircleAndCircleInside);
	insertCrosshair(crosshair::Type::eDashedCircleAndDot);
	insertCrosshair(crosshair::Type::eDashedBoxWithCross);

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

			crosshair::Settings crosshairSettings;
			crosshairSettings.customCrosshairName = entry.path().filename().wstring();

			static std::set<std::wstring> customCrossairsWithErrors;
			if (customCrossairsWithErrors.find(crosshairSettings.customCrosshairName) != customCrossairsWithErrors.end())
				continue;

			CBitmap bitmap;
			try
			{
				crosshair::LoadCrosshair(crosshairSettings, bitmap);
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
			crosshair::ResizeCrosshair(bitmap, CSize(15, 15));

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
			ext::send_event(&ISettingsChanged::OnSettingsChangedByUser);

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
}

void CGameSettingsDlg::OnLvnItemchangedListMacroses(NMHDR* pNMHDR, LRESULT* pResult)
{
	GetDlgItem(IDC_BUTTON_REMOVE)->EnableWindow(m_listMacroses.GetSelectedCount() > 0);
	*pResult = 0;
}

void CGameSettingsDlg::OnBnClickedCheckDisableWin()
{
	m_configuration->disableWinButton = m_checkDisableWinButton.GetCheck() == BST_CHECKED;
	ext::send_event(&ISettingsChanged::OnSettingsChangedByUser);
}

void CGameSettingsDlg::OnBnClickedCheckUse()
{
	auto& show = m_configuration->crosshairSettings.show;
	show = m_checkboxShowCrosshair.GetCheck() == BST_CHECKED;

	m_comboboxCrosshairSize.EnableWindow(show);
	m_colorPickerCrosshairColor.EnableWindow(show);
	m_comboCrosshairs.EnableWindow(show);

	ext::send_event(&ISettingsChanged::OnSettingsChangedByUser);
}

void CGameSettingsDlg::OnCbnSelendokComboCrosshairSelection()
{
	auto selectedCrosshair = m_comboCrosshairs.GetCurSel();

	bool selectedCustomCrosshair = selectedCrosshair > (int)crosshair::Type::eDashedBoxWithCross;
	if (selectedCustomCrosshair)
	{
		CString text;
		m_comboCrosshairs.GetWindowText(text);

		m_configuration->crosshairSettings.customCrosshairName = text;
	}
	else
	{
		m_configuration->crosshairSettings.customCrosshairName.clear();
		m_configuration->crosshairSettings.type = crosshair::Type(selectedCrosshair);
	}

	m_comboboxCrosshairSize.EnableWindow(!selectedCustomCrosshair);

	ext::send_event(&ISettingsChanged::OnSettingsChangedByUser);

	UpdateDemoCrosshair();
}

void CGameSettingsDlg::OnCbnSelendokComboCrosshairSize()
{
	m_configuration->crosshairSettings.size = crosshair::Size(m_comboboxCrosshairSize.GetCurSel());
	
	ext::send_event(&ISettingsChanged::OnSettingsChangedByUser);

	UpdateDemoCrosshair();
}

void CGameSettingsDlg::OnBnClickedMfccolorbuttonCrosshairColor()
{
	m_configuration->crosshairSettings.color = m_colorPickerCrosshairColor.GetColor();

	std::list<CBitmap*> crosshairs;
	for (auto& crosshair : m_crosshairs)
	{
		crosshair::ChangeCrosshairColor(crosshair, m_configuration->crosshairSettings.color);
		crosshairs.emplace_back(&crosshair);
	}
	m_comboCrosshairs.SetBitmapsList(crosshairs);
	
	ext::send_event(&ISettingsChanged::OnSettingsChangedByUser);

	UpdateDemoCrosshair();
}
