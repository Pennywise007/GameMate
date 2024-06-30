#include "pch.h"
#include "framework.h"
#include "afxdialogex.h"
#include "resource.h"

#include "UI/Dlg/MainDlg.h"
#include "UI/Tab/ActiveProcessToolkitTab.h"
#include "UI/Tab/ActionsExecutorTab.h"
#include "UI/Dlg/InputSimulatorInfoDlg.h"
#include "UI/Dlg/InputEditorDlg.h"

#include "InputManager.h"

#include <core/events.h>
#include <core/Worker.h>
#include "core/Settings.h"

#include <ext/core.h>
#include <ext/core/check.h>
#include <ext/constexpr/map.h>

#include <Controls/Layout/Layout.h>
#include <Controls/TrayHelper/TrayHelper.h>
#include <Controls/Tooltip/ToolTip.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace {

constexpr ext::constexpr_map kComboboxIndexesToInputModes=  {
	std::pair{0, InputManager::InputSimulator::Razer},
	std::pair{1, InputManager::InputSimulator::LogitechGHubNew},
	std::pair{2, InputManager::InputSimulator::Logitech},
	std::pair{3, InputManager::InputSimulator::DD},
	std::pair{4, InputManager::InputSimulator::MouClassInputInjection},
	std::pair{5, InputManager::InputSimulator::SendInput},
};
static_assert(!kComboboxIndexesToInputModes.contain_duplicate_keys());
static_assert(!kComboboxIndexesToInputModes.contain_duplicate_values());

constexpr ext::constexpr_map kDriverNames =
{
	std::pair{ InputManager::InputSimulator::Razer, L"Razer driver" },
	std::pair{ InputManager::InputSimulator::LogitechGHubNew, L"Logitech G Hub" },
	std::pair{ InputManager::InputSimulator::Logitech, L"Logitech old driver" },
	std::pair{ InputManager::InputSimulator::DD, L"DD driver" },
	std::pair{ InputManager::InputSimulator::MouClassInputInjection, L"Mou driver" },
	std::pair{ InputManager::InputSimulator::SendInput, L"Don't use simulator" },
};

static_assert(!kDriverNames.contain_duplicate_keys());
static_assert(!kDriverNames.contain_duplicate_values());

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

	DDX_Control(pDX, IDC_TABCONTROL_MODES, m_tabControlModes());
	DDX_Control(pDX, IDC_COMBO_INPUT_DRIVER, m_inputSimulator);
	DDX_Control(pDX, IDC_MFCBUTTON_INPUT_DRIVER_INFO, m_buttonInputSimulatorInfo);
	DDX_Control(pDX, IDC_CHECK_TIMER, m_buttonShowTimer);
	DDX_Control(pDX, IDC_MFCBUTTON_TIMER_HOTKEY, m_buttonShowTimerHotkey);
}

BEGIN_MESSAGE_MAP(CMainDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_WM_SYSCOMMAND()
	ON_CBN_SELCHANGE(IDC_COMBO_INPUT_DRIVER, &CMainDlg::OnCbnSelchangeComboInputDriver)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TABCONTROL_MODES, &CMainDlg::OnTcnSelchangeTabcontrolGames)
	ON_BN_CLICKED(IDC_MFCBUTTON_INPUT_DRIVER_INFO, &CMainDlg::OnBnClickedMfcbuttonInputSimulatorInfo)
	ON_BN_CLICKED(IDC_MFCBUTTON_TIMER_HOTKEY, &CMainDlg::OnBnClickedMfcbuttonTimerHotkey)
	ON_BN_CLICKED(IDC_CHECK_TIMER, &CMainDlg::OnBnClickedCheckTimer)
END_MESSAGE_MAP()

BOOL CMainDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	auto& settings = ext::get_singleton<Settings>();
	if (settings.tracesEnabled)
		ext::get_tracer().Enable();

	m_buttonInputSimulatorInfo.SetImageOffset(7);

	m_tabControlModes.SetDrawSelectedAsWindow();
	m_tabControlModes.AddTab(L"Actions executor",
							 std::make_shared<CActionsExecutorTab>(&m_tabControlModes()),
							 CActionsExecutorTab::IDD);
	m_tabControlModes.AddTab(L"Active process toolkit",
							 std::make_shared<CActiveProcessToolkitTab>(&m_tabControlModes()),
							 CActiveProcessToolkitTab::IDD);
	m_tabControlModes.SetCurSel(int(settings.selectedMode));
	m_tabControlModes.AutoResizeTabsToFitFullControlWidth();
	updateDriverInfoButton();

	CTrayHelper::Instance().addTrayIcon(
		m_hIcon, L"Game mate",
		[]()
		{
			HMENU hMenu = ::GetSubMenu(LoadMenu(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MENU_TRAY)), 0);
			ENSURE(hMenu);
								 
			if (ext::get_singleton<Settings>().process_toolkit.enabled)
				ENSURE(::DeleteMenu(hMenu, ID_MENU_ENABLE_PROCESS_TOOLKIT, MF_BYCOMMAND));
			else
				ENSURE(::DeleteMenu(hMenu, ID_MENU_DISABLE_PROCESS_TOOLKIT, MF_BYCOMMAND));

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
			case ID_MENU_ENABLE_PROCESS_TOOLKIT:
			case ID_MENU_DISABLE_PROCESS_TOOLKIT:
				{
					bool& enabled = ext::get_singleton<Settings>().process_toolkit.enabled;
					enabled = !enabled;
					ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eProcessToolkit);
				}
				break;
			case ID_MENU_ENABLE_TRACES:
				ext::get_singleton<Settings>().tracesEnabled = true;
				ext::get_tracer().Enable();
				ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eGeneralSettings);
				break;
			case ID_MENU_DISABLE_TRACES:
				ext::get_singleton<Settings>().tracesEnabled = false;
				ext::get_tracer().Reset();
				ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eGeneralSettings);
				break;
			case ID_MENU_CLOSE:
				EndDialog(IDCANCEL);
				break;
			default:
				EXT_ASSERT(!"Unknown command!") << commandId;
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
		auto inputMode = settings.inputSimulator;
		auto err = InputManager::SetInputSimulator(inputMode);
		if (err.has_value())
		{
			EXT_ASSERT(inputMode != InputManager::InputSimulator::Auto);

			inputMode = InputManager::InputSimulator::Auto;

			EXT_EXPECT(!InputManager::SetInputSimulator(inputMode).has_value());
			EXT_EXPECT(inputMode != InputManager::InputSimulator::Auto);
			EXT_EXPECT(kDriverNames.contains_key(inputMode));
			EXT_EXPECT(settings.inputSimulator != inputMode);

			MessageBox((L"Fail reason: " + std::wstring(err.value()) +
					   L". Switching mode to '" + kDriverNames.get_value(inputMode) + L"'").c_str(),
					   L"Previously selected input simulator is not available",
					   MB_ICONEXCLAMATION);
		}

		EXT_EXPECT(kDriverNames.contains_key(inputMode));
		
		if (settings.inputSimulator != inputMode)
		{
			settings.inputSimulator = inputMode;
			ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eInputSimulator);
		}

		for (auto&& [i, mode] : kComboboxIndexesToInputModes)
		{
			m_inputSimulator.InsertString(i, kDriverNames.get_value(mode));
		}
		m_inputSimulator.SetCurSel(kComboboxIndexesToInputModes.get_key(inputMode));
	}
	catch (...)
	{
		MessageBox((L"Try to remove config file and restart app. Err:\n" + ext::ManageExceptionText(L"")).c_str(), L"Failed to init input simulator", MB_OK);
	}

	m_buttonShowTimerHotkey.SetBitmap(IDB_PNG_SETTINGS, Alignment::CenterCenter);
	m_timerDlg.Create(CTimerDlg::IDD, GetDesktopWindow());
	updateTimerButton();

	// Deserialize settings before starting worker to avoid any mouse lags
	EXT_UNUSED(ext::get_singleton<Settings>());
	// Starting worker
	EXT_UNUSED(ext::get_singleton<Worker>());

	// Don't allow to make our dialog smaller than it is in resource files.
	CRect rect;
	GetWindowRect(rect);
	Layout::SetWindowMinimumSize(*this, 600, rect.Height());
	
	LayoutLoader::ApplyLayoutFromResource(*this, m_lpszTemplateName);

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
	m_timerDlg.DestroyWindow();
	CDialogEx::OnDestroy();

	ext::get_singleton<Settings>().SaveSettings();
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMainDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
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
			ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eGeneralSettings);

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
	settings.selectedMode = Settings::ProgramMode(m_tabControlModes.GetCurSel());

	ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eGeneralSettings);

	*pResult = 0;
}

void CMainDlg::OnCbnSelchangeComboInputDriver()
{
	auto curSel = m_inputSimulator.GetCurSel();
	EXT_ASSERT(curSel != -1);

	auto selecetedInputSimulator = kComboboxIndexesToInputModes.get_value(curSel);

	auto err = InputManager::SetInputSimulator(selecetedInputSimulator);
	if (err.has_value())
	{
		auto newInputMode = InputManager::InputSimulator::Auto;
		EXT_DUMP_IF(InputManager::SetInputSimulator(newInputMode).has_value());

		MessageBox((L"Fail reason: " + std::wstring(err.value()) +
				   L". Switching mode to '" + kDriverNames.get_value(newInputMode) + L"'").c_str(),
				   (L"Can't use input simulator " + std::wstring(kDriverNames.get_value(selecetedInputSimulator))).c_str(),
				   MB_ICONEXCLAMATION);

		selecetedInputSimulator = newInputMode;
		m_inputSimulator.SetCurSel(kComboboxIndexesToInputModes.get_key(selecetedInputSimulator));
	}

	if (ext::get_singleton<Settings>().inputSimulator == selecetedInputSimulator)
		return;

	if (selecetedInputSimulator == InputManager::InputSimulator::SendInput)
	{
		if (MessageBox(L"Be aware that some game guards might detect input from standard windows input and may take some actions against it. "
					   L"It is recommended to install some input driver to avoid such detections. Do you want to see extra information?",
					   L"You will use standard windows API input", MB_YESNO) == IDYES)
		{
			OnBnClickedMfcbuttonInputSimulatorInfo();
		}
	}

	ext::get_singleton<Settings>().inputSimulator = selecetedInputSimulator;
	ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eInputSimulator);

	updateDriverInfoButton();
}

void CMainDlg::OnBnClickedMfcbuttonInputSimulatorInfo()
{
	CInputSimulatorInfoDlg(this).DoModal();
}

void CMainDlg::OnBnClickedMfcbuttonTimerHotkey()
{
	auto& currentBind = ext::get_singleton<Settings>().timer.showTimerBind;
	auto bind = CInputEditorDlg::EditBind(this, currentBind);
	if (!bind.has_value() || currentBind.ToString() == bind->ToString())
		return;

	currentBind = bind.value();
	updateTimerButton();
	ext::send_event(&ISettingsChanged::OnSettingsChanged, ISettingsChanged::ChangedType::eTimer);
}

void CMainDlg::OnBnClickedCheckTimer()
{
	m_buttonShowTimer.SetCheck(m_buttonShowTimer.GetCheck() == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED);
	ext::send_event_async(&ITimerNotifications::OnShowHideTimer);
}

void CMainDlg::OnShowHideTimer()
{
	m_buttonShowTimer.SetCheck(m_buttonShowTimer.GetCheck() == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED);
}

void CMainDlg::updateDriverInfoButton()
{
	CString warning;
	switch (ext::get_singleton<Settings>().inputSimulator)
	{
	case InputManager::InputSimulator::SendInput:
		warning = L"When you don't use any of the simulators all program (key/mouse) input can be detected by anti-cheat programs.\n";
		break;
	case InputManager::InputSimulator::Razer:
		break;
	default:
	{
		int IntArr[3];
		SystemParametersInfoA(SPI_GETMOUSE, 0, &IntArr, 0);
		int speed;
		SystemParametersInfoA(SPI_GETMOUSESPEED, 0, &speed, 0);

		bool enchancePointerPrecisionIsOff = IntArr[2] == 0;
		bool normalSpeed = speed == 10;
		if (!enchancePointerPrecisionIsOff || !normalSpeed)
		{
			warning = L"Selected input method has some problem with SetMousePos, if you need to set mouse pos:\n"
				L"- Disable 'Enchance pointer precision'\n"
				L"- Set default mouse speed(10)\n";
		}
		break;
	}
	}

	const auto iconSize = 17;
	const auto iconId = warning.IsEmpty() ? IDI_INFORMATION : IDI_WARNING;

	HICON icon;
	auto res = SUCCEEDED(::LoadIconWithScaleDown(NULL, iconId, iconSize, iconSize, &icon));
	EXT_ASSERT(res);

	m_buttonInputSimulatorInfo.SetIcon(icon, RightCenter, iconSize, iconSize);

	warning += L"Press to see more information.";
	controls::SetTooltip(m_buttonInputSimulatorInfo, warning);
}

void CMainDlg::updateTimerButton()
{
	CString buttonText;
	buttonText.Format(L"Show timer (%s)", ext::get_singleton<Settings>().timer.showTimerBind.ToString().c_str());
	m_buttonShowTimer.SetWindowTextW(buttonText);
}
