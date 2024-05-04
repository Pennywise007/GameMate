
#include "pch.h"
#include "framework.h"
#include "GameMate.h"
#include "MainDlg.h"
#include "afxdialogex.h"

#include "AddingTabDlg.h"
#include "GameSettingsDlg.h"
#include "InputManager.h"

#include <core/Worker.h>

#include <ext/core.h>
#include <ext/core/check.h>
#include <ext/constexpr/map.h>

#include <Controls/Layout/Layout.h>
#include <Controls/TrayHelper/TrayHelper.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace {

constexpr ext::constexpr_map kComboboxIndexesToInputModes=  {
	std::pair{0, InputManager::InputMode::Razer},
	std::pair{1, InputManager::InputMode::Logitech},
	std::pair{2, InputManager::InputMode::DD},
	std::pair{3, InputManager::InputMode::MouClassInputInjection},
	std::pair{4, InputManager::InputMode::SendInput},
};

constexpr ext::constexpr_map kDriverNames =
{
	std::pair{ InputManager::InputMode::Razer, L"Razer driver" },
	std::pair{ InputManager::InputMode::Logitech, L"Logitech driver" },
	std::pair{ InputManager::InputMode::DD, L"DD driver" },
	std::pair{ InputManager::InputMode::MouClassInputInjection, L"Mou driver" },
	std::pair{ InputManager::InputMode::SendInput, L"Windows default input" },
};

static_assert(kComboboxIndexesToInputModes.size() == kDriverNames.size());

} // namespace

CMainDlg::CMainDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MAIN_DIALOG, pParent)
{
	ext::core::Init();

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMainDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_TABCONTROL_GAMES, m_tabControlGames());
	DDX_Control(pDX, IDC_CHECK_PROGRAM_WORKING, m_checkProgramWorking);
	DDX_Control(pDX, IDC_COMBO_INPUT_DRIVER, m_inputDriver);
	DDX_Control(pDX, IDC_MFCBUTTON_INPUT_DRIVER_INFO, m_buttonInputDriverInfo);
}

BEGIN_MESSAGE_MAP(CMainDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_WM_SYSCOMMAND()
	ON_BN_CLICKED(IDC_BUTTON_ADD_TAB, &CMainDlg::OnBnClickedButtonAddTab)
	ON_BN_CLICKED(IDC_BUTTON_RENAME_TAB, &CMainDlg::OnBnClickedButtonRenameTab)
	ON_BN_CLICKED(IDC_BUTTON_DELETE_TAB, &CMainDlg::OnBnClickedButtonDeleteTab)
	ON_BN_CLICKED(IDC_CHECK_PROGRAM_WORKING, &CMainDlg::OnBnClickedCheckProgramWorking)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TABCONTROL_GAMES, &CMainDlg::OnTcnSelchangeTabcontrolGames)
	ON_CBN_SELCHANGE(IDC_COMBO_INPUT_DRIVER, &CMainDlg::OnCbnSelchangeComboInputDriver)
	ON_BN_CLICKED(IDC_MFCBUTTON_INPUT_DRIVER_INFO, &CMainDlg::OnBnClickedMfcbuttonInputDriverInfo)
END_MESSAGE_MAP()

BOOL CMainDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	auto& settings = ext::get_singleton<Settings>();
	for (const auto& tab : settings.tabs)
	{
		AddTab(tab);
	}
	m_tabControlGames.SetCurSel(settings.activeTab);

	if (settings.tracesEnabled)
		ext::get_tracer().Enable();

	OnGamesTabChanged();

	CTrayHelper::Instance().addTrayIcon(
		m_hIcon, L"Game mate",
		[]()
		{
			HMENU hMenu = ::GetSubMenu(LoadMenu(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MENU_TRAY)), 0);
			ENSURE(hMenu);
								 
			if (ext::get_singleton<Settings>().programWorking)
				ENSURE(::DeleteMenu(hMenu, ID_MENU_ENABLE_PROGRAM, MF_BYCOMMAND));
			else
				ENSURE(::DeleteMenu(hMenu, ID_MENU_DISABLE_PROGRAM, MF_BYCOMMAND));

			if (ext::get_tracer().CanTrace(ext::ITracer::Level::eDebug))
				ENSURE(::DeleteMenu(hMenu, ID_MENU_ENABLE_TRACES, MF_BYCOMMAND));
			else
				ENSURE(::DeleteMenu(hMenu, ID_MENU_DISABLE_TRACES, MF_BYCOMMAND));

			return hMenu;
		},
		nullptr,
		[this](UINT commandId)
		{
			switch (commandId)
			{
			case ID_MENU_OPEN:
				ShowWindow(SW_RESTORE);
				SetForegroundWindow();
				break;
			case ID_MENU_ENABLE_PROGRAM:
			case ID_MENU_DISABLE_PROGRAM:
				OnBnClickedCheckProgramWorking();
				break;
			case ID_MENU_ENABLE_TRACES:
				ext::get_singleton<Settings>().tracesEnabled = true;
				ext::get_tracer().Enable();
				ext::send_event(&ISettingsChanged::OnSettingsChanged);
				break;
			case ID_MENU_DISABLE_TRACES:
				ext::get_singleton<Settings>().tracesEnabled = false;
				ext::get_tracer().Reset();
				ext::send_event(&ISettingsChanged::OnSettingsChanged);
				break;
			case ID_MENU_CLOSE:
				EndDialog(IDCANCEL);
				break;
			default:
				EXT_ASSERT(!"�� ��������� ����� ����!");
				break;
			}
		},
		[this]()
		{
			ShowWindow(SW_RESTORE);
			SetForegroundWindow();
		});

	try
	{
		InputManager::InputMode inputMode = InputManager::InputMode(std::max<int>(settings.driverInputMode, 0));
		auto err = InputManager::InitializeInputMode(inputMode);
		if (err != InputManager::Error::Success)
		{
			inputMode = InputManager::InputMode::Auto;

			EXT_EXPECT(InputManager::InitializeInputMode(inputMode) == InputManager::Error::Success);
			EXT_EXPECT(inputMode != InputManager::InputMode::Auto);
			EXT_EXPECT(kDriverNames.contains_key(inputMode));
			EXT_EXPECT(InputManager::InputMode(settings.driverInputMode) != inputMode);

			MessageBox((L"Fail reason: " + std::wstring(InputManager::kErrorCodes.get_value(err)) +
					   L". Program will use " + kDriverNames.get_value(inputMode)).c_str(),
					   L"Previously selected input driver is not available",
					   MB_ICONEXCLAMATION);
		}

		EXT_EXPECT(kDriverNames.contains_key(inputMode));
		
		if (settings.driverInputMode != int(inputMode))
		{
			settings.driverInputMode = int(inputMode);
			ext::send_event(&ISettingsChanged::OnSettingsChanged);
		}

		m_inputDriver.InsertString(0, kDriverNames.get_value(kComboboxIndexesToInputModes.get_value(0)));
		m_inputDriver.InsertString(1, kDriverNames.get_value(kComboboxIndexesToInputModes.get_value(1)));
		m_inputDriver.InsertString(2, kDriverNames.get_value(kComboboxIndexesToInputModes.get_value(2)));
		m_inputDriver.InsertString(3, kDriverNames.get_value(kComboboxIndexesToInputModes.get_value(3)));
		m_inputDriver.InsertString(4, kDriverNames.get_value(kComboboxIndexesToInputModes.get_value(4)));

		m_inputDriver.SetCurSel(kComboboxIndexesToInputModes.get_key(inputMode));
	}
	catch (...)
	{
		MessageBox((L"Try to remove config file and restart app. Err:\n" + ext::ManageExceptionText(L"")).c_str(), L"Failed to init input driver", MB_OK);
	}

	UpdateProgramWorkingButton();

	// Don't allow to make our dialog smaller than it is in resource files.
	CRect rect;
	GetWindowRect(rect);
	Layout::SetWindowMinimumSize(*this, rect.Width(), rect.Height());

	// Starting worker
	//EXT_UNUSED(ext::get_singleton<Worker>());

	return TRUE;
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.
void CMainDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

void CMainDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	ext::get_singleton<Settings>().SaveSettings();
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMainDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMainDlg::OnBnClickedButtonAddTab()
{
	const auto newTab = CAddingTabDlg(this).ExecModal();
	if (newTab == nullptr)
		return;

	ext::get_singleton<Settings>().tabs.push_back(newTab);
	m_tabControlGames.SetCurSel(AddTab(newTab));

	OnGamesTabChanged();

	ext::send_event(&ISettingsChanged::OnSettingsChanged);
}

void CMainDlg::OnBnClickedButtonRenameTab()
{
	auto curSel = m_tabControlGames.GetCurSel();
	if (curSel == -1)
	{
		EXT_ASSERT(false);
		return;
	}

	auto& tabs = ext::get_singleton<Settings>().tabs;
	EXT_ASSERT(curSel < (int)tabs.size());

	auto tab = std::next(tabs.begin(), curSel)->get();
	const auto edditedTab = CAddingTabDlg(this, tab).ExecModal();
	if (edditedTab == nullptr)
		return;
	tab->tabName = edditedTab->tabName;

	TCITEMW item;
	item.mask = TCIF_TEXT;
	item.pszText = tab->tabName.data();
	item.cchTextMax = int(tab->tabName.size());
	m_tabControlGames.SetItem(curSel, &item);
	m_tabControlGames().Invalidate();

	ext::send_event(&ISettingsChanged::OnSettingsChanged);
}

void CMainDlg::OnBnClickedButtonDeleteTab()
{
	auto curSel = m_tabControlGames.GetCurSel();
	if (m_tabControlGames.DeleteTab(m_tabControlGames.GetCurSel()) != TRUE)
	{
		EXT_ASSERT(false);
		return;
	}

	auto& tabs = ext::get_singleton<Settings>().tabs;
	EXT_ASSERT(curSel < (int)tabs.size());
	tabs.erase(std::next(tabs.begin(), curSel));

	if (curSel >= (int)tabs.size())
		curSel = int(tabs.size()) - 1;
	m_tabControlGames.SetCurSel(curSel);

	OnGamesTabChanged();
	
	ext::send_event(&ISettingsChanged::OnSettingsChanged);
}

int CMainDlg::AddTab(const std::shared_ptr<TabConfiguration>& tabSettings)
{
	EXT_ASSERT(tabSettings != nullptr);

	GetDlgItem(IDC_BUTTON_DELETE_TAB)->EnableWindow(TRUE);

	const auto settingsTab = std::make_shared<CGameSettingsDlg>(tabSettings, &m_tabControlGames());
	return m_tabControlGames.InsertTab(m_tabControlGames.GetItemCount(), tabSettings->tabName.c_str(), settingsTab, CGameSettingsDlg::IDD);
}

void CMainDlg::OnGamesTabChanged()
{
	bool tabsExist = m_tabControlGames.GetItemCount() != 0;
	GetDlgItem(IDC_BUTTON_DELETE_TAB)->EnableWindow(tabsExist);
	GetDlgItem(IDC_BUTTON_RENAME_TAB)->EnableWindow(tabsExist);

	GetDlgItem(IDC_STATIC_NO_TABS)->ShowWindow(tabsExist ? SW_HIDE : SW_SHOW);
}

void CMainDlg::UpdateProgramWorkingButton()
{
	auto& settings = ext::get_singleton<Settings>();

	CString buttonText = settings.programWorking ? L"Disable program" : L"Enable program";
	buttonText += L"(Shift+F9)";
	m_checkProgramWorking.SetWindowTextW(buttonText);
	m_checkProgramWorking.SetCheck(settings.programWorking ? BST_CHECKED : BST_UNCHECKED);
}

void CMainDlg::UpdateDriverInfoButton()
{
	bool showWarning = ext::get_singleton<Settings>().driverInputMode == int(InputManager::InputMode::SendInput);
	// TODO

}

BOOL CMainDlg::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message)
	{
	case WM_KEYDOWN:
	{
		switch (pMsg->wParam)
		{
		case VK_RETURN:
			// Don't close window on enter
			return TRUE;
		case VK_ESCAPE:
			ShowWindow(SW_MINIMIZE);
			return TRUE;
		}
		break;
	}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CMainDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	switch (nID)
	{
	case SC_MINIMIZE:
		// hide our app window
		ShowWindow(SW_MINIMIZE);
		ShowWindow(SW_HIDE);

		auto& settings = ext::get_singleton<Settings>();
		if (settings.showMinimizedBubble)
		{
			settings.showMinimizedBubble = false;
			ext::send_event(&ISettingsChanged::OnSettingsChanged);

			// notify user that he can open it through the tray
			CTrayHelper::Instance().showBubble(
				L"Application is minimized to tray",
				L"To restore the application, double-click the icon or select the corresponding menu item.",
				NIIF_INFO,
				[this]()
				{
					ShowWindow(SW_RESTORE);
					SetForegroundWindow();
				});
		}
		break;
	}

	CDialogEx::OnSysCommand(nID, lParam);
}

void CMainDlg::OnTcnSelchangeTabcontrolGames(NMHDR* pNMHDR, LRESULT* pResult)
{
	auto& settings = ext::get_singleton<Settings>();
	settings.activeTab = m_tabControlGames.GetCurSel();

	ext::send_event(&ISettingsChanged::OnSettingsChanged);

	*pResult = 0;
}

void CMainDlg::OnSettingsChanged()
{
	UpdateProgramWorkingButton();
}

void CMainDlg::OnBnClickedCheckProgramWorking()
{
	auto& settings = ext::get_singleton<Settings>();
	settings.programWorking = !settings.programWorking;
	ext::send_event(&ISettingsChanged::OnSettingsChanged);
}

void CMainDlg::OnCbnSelchangeComboInputDriver()
{
	auto curSel = m_inputDriver.GetCurSel();
	EXT_ASSERT(curSel != -1);

	auto selecetedInputMode = kComboboxIndexesToInputModes.get_value(curSel);

	auto err = InputManager::InitializeInputMode(selecetedInputMode);
	if (err != InputManager::Error::Success)
	{
		auto newInputMode = InputManager::InputMode::Auto;
		EXT_DUMP_IF(InputManager::InitializeInputMode(newInputMode) != InputManager::Error::Success);

		MessageBox((L"Fail reason: " + std::wstring(InputManager::kErrorCodes.get_value(err)) +
				   L". Program will use " + kDriverNames.get_value(newInputMode)).c_str(),
				   (L"Can't use driver " + std::wstring(kDriverNames.get_value(selecetedInputMode))).c_str(),
				   MB_ICONEXCLAMATION);

		selecetedInputMode = newInputMode;
		m_inputDriver.SetCurSel(kComboboxIndexesToInputModes.get_key(selecetedInputMode));
	}

	if (ext::get_singleton<Settings>().driverInputMode == int(selecetedInputMode))
		return;

	if (selecetedInputMode == InputManager::InputMode::SendInput)
	{
		if (MessageBox(L"Be aware that some game guards might detect input from standart windows input and may take some actions against it. "
					   L"It is recommended to install some input driver to avoid such detections. Do you want to see extra information?",
					   L"You will use standart windows input", MB_YESNO) == IDYES)
		{
			OnBnClickedMfcbuttonInputDriverInfo();
		}
	}

	ext::get_singleton<Settings>().driverInputMode = int(selecetedInputMode);
	ext::send_event(&ISettingsChanged::OnSettingsChanged);
}

void CMainDlg::OnBnClickedMfcbuttonInputDriverInfo()
{
	// TODO
}
