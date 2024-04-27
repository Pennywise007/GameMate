
#include "pch.h"
#include "framework.h"
#include "GameMate.h"
#include "MainDlg.h"
#include "afxdialogex.h"

#include "AddingTabDlg.h"
#include "GameSettingsDlg.h"

#include <core/Worker.h>

#include <ext/core.h>
#include <ext/core/check.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CMainDlg::CMainDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MAIN_DIALOG, pParent)
{
	ext::core::Init();
	ext::get_tracer().Enable(); // TODO

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMainDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_TABCONTROL_GAMES, m_tabControlGames());
}

BEGIN_MESSAGE_MAP(CMainDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_ADD_TAB, &CMainDlg::OnBnClickedButtonAddTab)
	ON_BN_CLICKED(IDC_BUTTON_RENAME_TAB, &CMainDlg::OnBnClickedButtonRenameTab)
	ON_BN_CLICKED(IDC_BUTTON_DELETE_TAB, &CMainDlg::OnBnClickedButtonDeleteTab)
	ON_WM_SYSCOMMAND()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TABCONTROL_GAMES, &CMainDlg::OnTcnSelchangeTabcontrolGames)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL CMainDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	const auto& settings = ext::get_singleton<Settings>();
	for (const auto& tab : settings.tabs)
	{
		AddTab(tab);
	}
	m_tabControlGames.SetCurSel(settings.activeTab);

	OnGamesTabChanged();

	m_trayHelper.addTrayIcon(m_hIcon, L"Game mate",
							 []()
							 {
								 return ::GetSubMenu(LoadMenu(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MENU_TRAY)), 0);
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
								 case ID_MENU_CLOSE:
									 EndDialog(IDCANCEL);
									 break;
								 default:
									 EXT_ASSERT(!"Не известный пункт меню!");
									 break;
								 }
							 },
							 [this]()
							 {
								 ShowWindow(SW_RESTORE);
								 SetForegroundWindow();
							 });

	// Starting worker
	EXT_UNUSED(ext::get_singleton<Worker>());

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

	ext::send_event(&ISettingsChanged::OnSettingsChangedByUser);
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
	item.cchTextMax = tab->tabName.size();
	m_tabControlGames.SetItem(curSel, &item);
	m_tabControlGames().Invalidate();

	ext::send_event(&ISettingsChanged::OnSettingsChangedByUser);
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
	
	ext::send_event(&ISettingsChanged::OnSettingsChangedByUser);
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
	GetDlgItem(IDC_BUTTON_DELETE_TAB)->EnableWindow(m_tabControlGames.GetItemCount() != 0);
	GetDlgItem(IDC_BUTTON_RENAME_TAB)->EnableWindow(m_tabControlGames.GetItemCount() != 0);
	// TODO ADD BUNNER THAT USER NEED ADD SETTINGS
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
		// notify user that he can open it through the tray
		m_trayHelper.showBubble(L"Application is minimized to tray",
								L"To restore the application, double-click the icon or select the corresponding menu item.",
								NIIF_INFO,
								[this]()
								{
									ShowWindow(SW_RESTORE);
									SetForegroundWindow();
								});
		break;
	}

	CDialogEx::OnSysCommand(nID, lParam);
}

void CMainDlg::OnTcnSelchangeTabcontrolGames(NMHDR* pNMHDR, LRESULT* pResult)
{
	auto& settings = ext::get_singleton<Settings>();
	settings.activeTab = m_tabControlGames.GetCurSel();

	ext::send_event(&ISettingsChanged::OnSettingsChangedByUser);

	*pResult = 0;
}
