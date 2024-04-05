#include "pch.h"
#include "GameMate.h"
#include "afxdialogex.h"
#include "GameSettingsDlg.h"
#include "EditMacrosDlg.h"

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
	DDX_Control(pDX, IDC_EDIT_GAME_NAME, m_exeName);
}

BEGIN_MESSAGE_MAP(CGameSettingsDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CGameSettingsDlg::OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_EDIT, &CGameSettingsDlg::OnBnClickedButtonEdit)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, &CGameSettingsDlg::OnBnClickedButtonRemove)
	ON_BN_CLICKED(IDC_CHECK_ENABLED, &CGameSettingsDlg::OnBnClickedCheckEnabled)
	ON_EN_CHANGE(IDC_EDIT_GAME_NAME, &CGameSettingsDlg::OnEnChangeEditGameName)
END_MESSAGE_MAP()

BOOL CGameSettingsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_enabled.SetCheck(m_configuration->enabled);
	m_exeName.SetWindowTextW(m_configuration->exeName.c_str());

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
	CEditMacrosDlg dlg({}, this);
	dlg.DoModal();
	// TODO: Add your control notification handler code here
}

void CGameSettingsDlg::OnBnClickedButtonEdit()
{
	// TODO: Add your control notification handler code here
}

void CGameSettingsDlg::OnBnClickedButtonRemove()
{
	// TODO: Add your control notification handler code here
}

void CGameSettingsDlg::OnBnClickedCheckEnabled()
{
	m_configuration->enabled = m_enabled.GetCheck();
}

void CGameSettingsDlg::OnEnChangeEditGameName()
{
	CString name;
	m_exeName.GetWindowTextW(name);
	m_configuration->exeName = name;
}
