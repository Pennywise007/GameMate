#include "pch.h"
#include "GameMate.h"
#include "tlhelp32.h"
#include "afxdialogex.h"
#include "GameSettingsDlg.h"
#include "MacrosEditDlg.h"
#include "BindEditDlg.h"
#include "Crosshairs.h"

#include <ext/scope/defer.h>

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
}

BEGIN_MESSAGE_MAP(CGameSettingsDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CGameSettingsDlg::OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, &CGameSettingsDlg::OnBnClickedButtonRemove)
	ON_BN_CLICKED(IDC_CHECK_ENABLED, &CGameSettingsDlg::OnBnClickedCheckEnabled)
	ON_CBN_EDITCHANGE(IDC_COMBO_EXE_NAME, &CGameSettingsDlg::OnCbnEditchangeComboExeName)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_MACROSES, &CGameSettingsDlg::OnLvnItemchangedListMacroses)
	ON_CBN_SETFOCUS(IDC_COMBO_EXE_NAME, &CGameSettingsDlg::OnCbnSetfocusComboExeName)
	ON_CBN_SELENDOK(IDC_COMBO_EXE_NAME, &CGameSettingsDlg::OnCbnSelendokComboExeName)
	ON_BN_CLICKED(IDC_CHECK_USE, &CGameSettingsDlg::OnBnClickedCheckUse)
	ON_BN_CLICKED(IDC_MFCCOLORBUTTON_CROSSHAIR_COLOR, &CGameSettingsDlg::OnBnClickedMfccolorbuttonCrosshairColor)
	ON_CBN_SELENDOK(IDC_COMBO_CROSSHAIR_SELECTION, &CGameSettingsDlg::OnCbnSelendokComboCrosshairSelection)
	ON_CBN_SELENDOK(IDC_COMBO_CROSSHAIR_SIZE, &CGameSettingsDlg::OnCbnSelendokComboCrosshairSize)
END_MESSAGE_MAP()

void ChangeBitmapColor(CBitmap& bitmap, COLORREF color)
{
    BITMAP bm;
    bitmap.GetBitmap(&bm);

    std::unique_ptr<BYTE[]> pBits(new BYTE[bm.bmWidthBytes * bm.bmHeight]);
    ::ZeroMemory(pBits.get(), bm.bmWidthBytes * bm.bmHeight);
    ::GetBitmapBits(bitmap, bm.bmWidthBytes * bm.bmHeight, pBits.get());

	for (int i = 0; i < bm.bmWidth * bm.bmHeight; ++i) {
        BYTE* pPixel = pBits.get() + i * 4; // Assuming 32-bit RGBA format
		if (pPixel[3] != 0)  // Non-transparent pixel
		{
			pPixel[0] = GetBValue(color);
			pPixel[1] = GetGValue(color);
			pPixel[2] = GetRValue(color);
			// Alpha component remains unchanged (pPixel[3])
		}
	}
	::SetBitmapBits(bitmap, bm.bmWidthBytes * bm.bmHeight, pBits.get());
}

BOOL CGameSettingsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_enabled.SetCheck(m_configuration->enabled);
	m_exeName.SetWindowTextW(m_configuration->exeName.c_str());

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

		m_staticCrosshairInfoTooltip.SetTooltip(&m_staticCrosshairInfo,
												L"If you want to add your own crosshair put file with name crosshair_X and .png or .ico extenstions to the folder:\n"
												L"$EXE_FOLDER$\\res");
	}

	m_comboboxCrosshairSize.AddString(L"Small");
    m_comboboxCrosshairSize.AddString(L"Medium");
    m_comboboxCrosshairSize.AddString(L"Large");
	ASSERT((int)m_configuration->crosshairSettings.size <= 3);
    m_comboboxCrosshairSize.SetCurSel((int)m_configuration->crosshairSettings.size);

	m_colorPickerCrosshairColor.SetColor(m_configuration->crosshairSettings.color);

	{
		CPngImage pngImage;
		pngImage.Load(IDB_PNG_CROSSHAIR_1_16, AfxGetResourceHandle());

		m_crosshairs.emplace_back();
		CBitmap& bitmap = m_crosshairs.back();

		bitmap.Attach(pngImage.Detach());

		GetDlgItem(IDC_STATIC_CROSSHAIR)->ModifyStyle(0, SS_BITMAP);
		((CStatic*)(GetDlgItem(IDC_STATIC_CROSSHAIR)))->SetBitmap(bitmap);

		m_comboCrosshairs.InsertItem(0, L"123", 0, 0, 0);
		m_comboCrosshairs.SetCurSel(0);

		OnBnClickedMfccolorbuttonCrosshairColor();
	}
	OnBnClickedCheckUse();

	/*std::list<CBitmap*> crosshairs{&bitmap};

	CRect rect2;
	m_comboCrosshairs.GetClientRect(rect2);

	//m_comboCrosshairs.SetIconList(crosshairs2, CSize(32, 32));
	m_comboCrosshairs.SetBitmapsList(crosshairs, CSize(32, 32));
	m_comboCrosshairs.InsertItem(0, L"123", 0, 0, 0);
	m_comboCrosshairs.SetCurSel(0);

	m_crosshairs.emplace_back(std::move(bitmap));*/


	//ReplaceColor(bitmap, RGB(255, 0, 0));
	/*
	CDC dcMem;
	dcMem.CreateCompatibleDC(NULL);
	// load the bitmap into the memory device context
	CBitmap* oldBitmap = dcMem.SelectObject(&bitmap);
	RGBQUAD colorTable[2]; // for 1-bpp only
	// grab the color table
	UINT nColors = GetDIBColorTable(dcMem.GetSafeHdc(), 0, 2, colorTable);
	nColors = SetDIBColorTable(dcMem.GetSafeHdc(), 0, 2, colorTable);
	dcMem.SelectObject(oldBitmap);
	*/
	/* {
		CPngImage pngImage;
		pngImage.Load(IDB_PNG_CROSSHAIR, AfxGetResourceHandle());

		// resize bitmap to 20x20
		CBitmap bitmap_src;
		bitmap_src.Attach(pngImage.Detach());

		BITMAP bm = { 0 };
		bitmap_src.GetBitmap(&bm);
		auto size = CSize(bm.bmWidth, bm.bmHeight);

		CWindowDC screenCDC(NULL);

		auto hiSize = CSize(20, 20);

		bitmap.CreateCompatibleBitmap(&screenCDC, hiSize.cx, hiSize.cy);

		CDC srcCompatCDC;
		srcCompatCDC.CreateCompatibleDC(&screenCDC);
		CDC destCompatCDC;
		destCompatCDC.CreateCompatibleDC(&screenCDC);

		CMemDC srcDC(srcCompatCDC, CRect(CPoint(), size));
		auto oldSrcBmp = srcDC.GetDC().SelectObject(&bitmap_src);

		CMemDC destDC(destCompatCDC, CRect(CPoint(), hiSize));
		auto oldDestBmp = destDC.GetDC().SelectObject(&bitmap);

		destDC.GetDC().StretchBlt(0, 0, hiSize.cx, hiSize.cy, &srcDC.GetDC(), 0, 0, size.cx, size.cy, SRCCOPY);

		srcDC.GetDC().SelectObject(oldSrcBmp);
		destDC.GetDC().SelectObject(oldDestBmp);

		/*CBitmap bitmap_src;
		bitmap_src.Attach(pngImage.Detach());

		BITMAP bm = { 0 };
		bitmap_src.GetBitmap(&bm);
		auto size = CSize(bm.bmWidth, bm.bmHeight);
		CWindowDC wndDC(NULL);
		CDC srcDC;
		srcDC.CreateCompatibleDC(&wndDC);
		auto oldSrcBmp = srcDC.SelectObject(&bitmap_src);

		CDC destDC;
		destDC.CreateCompatibleDC(&wndDC);
		bitmap.CreateCompatibleBitmap(&wndDC, 15, 15);
		auto oldDestBmp = destDC.SelectObject(&bitmap);

		destDC.StretchBlt(0, 0, 15, 15, &srcDC, 0, 0, size.cx, size.cy, SRCCOPY);
	}*/

	// load PNG from IDB_PNG_CROSSHAIR and set it as icon ton the CStatic control


	//HICON icon;
	//auto res = SUCCEEDED(::LoadIconWithScaleDown(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_PNG_CROSSHAIR), 32, 32, &icon));
	//ASSERT(res);

	/*HICON icon;
	auto res = SUCCEEDED(::LoadIconWithScaleDown(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1), 16, 16, &icon));
	ASSERT(res);

	GetDlgItem(IDC_STATIC_CROSSHAIR)->ModifyStyle(0, SS_ICON);
	((CStatic*)(GetDlgItem(IDC_STATIC_CROSSHAIR)))->SetIcon(icon);*/


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

			auto bind = CBindEditDlg(parentWindow, editableMacrosIt->first).ExecModal();
			if (!bind.has_value())
				return nullptr;

			const auto bindText = bind->ToString();
			if (bindText == editableMacrosIt->first.ToString())
				return nullptr;

			if (auto sameBindIt = macroses.find(bind.value()); sameBindIt != macroses.end())
			{
				if (MessageBox((L"Bind '" + bindText + L"' already exists, do you want to replace it?").c_str(),
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
	Macros::Action action;
	{
		auto bind = CBindEditDlg(this).ExecModal();
		if (!bind.has_value())
			return;
		action = std::move(bind.value());
	}

	const auto it = m_configuration->macrosByBind.find(action);
	const bool actionExists = it != m_configuration->macrosByBind.end();

	Macros editableMacros;
	if (actionExists)
	{
		auto res = MessageBox((L"Do you want to edit action '" + action.ToString() + L"'?").c_str(), L"Action already has macros", MB_OKCANCEL | MB_ICONWARNING);
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
		auto item = m_listMacroses.InsertItem(m_listMacroses.GetItemCount(), action.ToString().c_str());
		m_listMacroses.SelectItem(item);
	}

	auto newMacros = CMacrosEditDlg(editableMacros, this).ExecModal();

	if (!actionExists)
		m_listMacroses.DeleteItem(m_listMacroses.GetItemCount() - 1);

	if (!newMacros.has_value())
		return;

	AddNewMacros(std::move(action), std::move(newMacros.value()));
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
}

void CGameSettingsDlg::OnBnClickedCheckEnabled()
{
	m_configuration->enabled = m_enabled.GetCheck() == BST_CHECKED;
}

void CGameSettingsDlg::OnCbnEditchangeComboExeName()
{
	CString text;
	m_exeName.GetWindowText(text);
	m_configuration->exeName = text;
}

void CGameSettingsDlg::OnCbnSelendokComboExeName()
{
	CString text;
	m_exeName.GetLBText(m_exeName.GetCurSel(), text);
	m_configuration->exeName = text;
}

void CGameSettingsDlg::AddNewMacros(TabConfiguration::Keybind keybind, Macros&& newMacros)
{
	auto& macros = m_configuration->macrosByBind;
	auto it = macros.try_emplace(std::move(keybind), std::move(newMacros));
	ASSERT(it.second);

	auto item = (int)std::distance(macros.begin(), it.first);

	item = m_listMacroses.InsertItem(item, it.first->first.ToString().c_str());
	std::wstring actions;
	for (const auto& action : it.first->second.actions) {
		if (!actions.empty())
			actions += L" → ";

		if (action.delay != 0)
			actions += L"(" + std::to_wstring(action.delay) + L"ms) ";

		actions += action.ToString();
	}
	m_listMacroses.SetItemText(item, Columns::eActions, actions.c_str());

	std::wostringstream str;
	str << it.first->second.randomizeDelays;
	m_listMacroses.SetItemText(item, Columns::eRandomizeDelay, str.str().c_str());

	m_listMacroses.SelectItem(item);
}

void CGameSettingsDlg::ChangeCrosshairColors(COLORREF newColor)
{
	std::list<CBitmap*> crosshairs;
	for (auto& crosshair : m_crosshairs)
	{
        ChangeBitmapColor(crosshair, newColor);
		crosshairs.emplace_back(&crosshair);
    }

	m_comboCrosshairs.SetBitmapsList(crosshairs, CSize(16, 16));
}

void CGameSettingsDlg::OnLvnItemchangedListMacroses(NMHDR* pNMHDR, LRESULT* pResult)
{
	GetDlgItem(IDC_BUTTON_REMOVE)->EnableWindow(m_listMacroses.GetSelectedCount() > 0);
	*pResult = 0;
}

void CGameSettingsDlg::OnCbnSetfocusComboExeName()
{
	const auto curCursor = ::SetCursor(LoadCursor(NULL, IDC_WAIT));
	EXT_DEFER(::SetCursor(curCursor));

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

	CString currentText;
	m_exeName.GetWindowText(currentText);

	m_exeName.ResetContent();
	for (const auto& name : runningProcessesNames)
	{
		m_exeName.AddString(name.c_str());
	}
	m_exeName.SetWindowText(currentText);

	m_exeName.ShowDropDown();
}

void CGameSettingsDlg::OnBnClickedCheckUse()
{
	bool use = m_checkboxShowCrosshair.GetCheck() == BST_CHECKED;

	m_comboboxCrosshairSize.EnableWindow(use);
	m_colorPickerCrosshairColor.EnableWindow(use);
	m_comboCrosshairs.EnableWindow(use);
}

void CGameSettingsDlg::OnCbnSelendokComboCrosshairSelection()
{
	// TODO: Add your control notification handler code here
}

void CGameSettingsDlg::OnCbnSelendokComboCrosshairSize()
{
	// TODO: Add your control notification handler code here
}

void CGameSettingsDlg::OnBnClickedMfccolorbuttonCrosshairColor()
{
	ChangeCrosshairColors(m_colorPickerCrosshairColor.GetColor());
}
