#include "afxdialogex.h"
#include "pch.h"
#include "resource.h"
#include "string"

#include "EditValueDlg.h"

IMPLEMENT_DYNAMIC(CEditValueDlg, CDialogEx)

CEditValueDlg::CEditValueDlg(CWnd* pParent, const CString& title, int& val, bool allowNegative)
	: CDialogEx(IDD_DIALOG_EDIT_NUMBER, pParent)
	, m_title(title)
	, m_allowNegative(allowNegative)
	, m_value(val)
{
}

std::optional<int> CEditValueDlg::EditValue(CWnd* pParent, const CString& title, int number, bool allowNegative)
{
	int value = number;
	if (CEditValueDlg(pParent, title, value, allowNegative).DoModal() == IDCANCEL)
		return std::nullopt;

	return value;
}

void CEditValueDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT, m_editNumber);
}

BEGIN_MESSAGE_MAP(CEditValueDlg, CDialogEx)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL CEditValueDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetWindowText(m_title);

	m_editNumber.SetUseOnlyNumbers();
	m_editNumber.UsePositiveDigitsOnly(!m_allowNegative);
	m_editNumber.SetWindowText(std::to_wstring(m_value).c_str());

	return TRUE;
}

void CEditValueDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	CString text;
	m_editNumber.GetWindowText(text);
	m_value = std::stol(text.GetString());
}
