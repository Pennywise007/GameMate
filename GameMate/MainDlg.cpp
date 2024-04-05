
#include "pch.h"
#include "framework.h"
#include "GameMate.h"
#include "MainDlg.h"
#include "afxdialogex.h"

#include "AddingTabDlg.h"
#include "GameSettingsDlg.h"

#include <ext/core/check.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CMainDlg::CMainDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MAIN_DIALOG, pParent)
{
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
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_ADD_TAB, &CMainDlg::OnBnClickedButtonAddTab)
	ON_BN_CLICKED(IDC_BUTTON_DELETE_TAB, &CMainDlg::OnBnClickedButtonDeleteTab)
END_MESSAGE_MAP()

BOOL CMainDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	const auto& settings = ext::get_service<Settings>();
	for (const auto& tab : settings.tabs)
	{
		AddTab(tab);
	}
	m_tabControlGames.SetCurSel(settings.activeTab);

	OnGamesTabChanged();

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

	ext::get_service<Settings>().tabs.push_back(newTab);
	m_tabControlGames.SetCurSel(AddTab(newTab));
}

void CMainDlg::OnBnClickedButtonDeleteTab()
{
	auto curSel = m_tabControlGames.GetCurSel();
	if (m_tabControlGames.DeleteTab(m_tabControlGames.GetCurSel()) != TRUE)
	{
		EXT_ASSERT(false);
		return;
	}

	auto& tabs = ext::get_service<Settings>().tabs;
	EXT_ASSERT(curSel < (int)tabs.size());
	tabs.erase(std::next(tabs.begin(), curSel));

	if (curSel >= (int)tabs.size())
		curSel = int(tabs.size()) - 1;
	m_tabControlGames.SetCurSel(curSel);

	OnGamesTabChanged();
}

int CMainDlg::AddTab(const std::shared_ptr<TabConfiguration>& tabSettings)
{
	EXT_ASSERT(tabSettings != nullptr);

	GetDlgItem(IDC_BUTTON_DELETE_TAB)->EnableWindow(TRUE);

	const auto settingsTab = std::make_shared<CGameSettingsDlg>(tabSettings, &m_tabControlGames());
	return m_tabControlGames.InsertTab(m_tabControlGames.GetItemCount(), tabSettings->tabName.c_str(), settingsTab, CGameSettingsDlg::IDD);
}

void CMainDlg::OnDestroy()
{
	ext::get_service<Settings>().activeTab = m_tabControlGames.GetCurSel();
	CDialogEx::OnDestroy();
}

void CMainDlg::OnGamesTabChanged()
{
	GetDlgItem(IDC_BUTTON_DELETE_TAB)->EnableWindow(m_tabControlGames.GetItemCount() != 0);
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
