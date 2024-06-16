#pragma once

#include "afxdialogex.h"

#include <optional>

#include <Controls/Edit/SpinEdit/SpinEdit.h>

class CEditValueDlg : private CDialogEx
{
	DECLARE_DYNAMIC(CEditValueDlg)

	CEditValueDlg(CWnd* pParent, const CString& title, int& val, bool allowNegative = false);

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_EDIT_NUMBER };
#endif
public:
	static std::optional<int> EditValue(
		CWnd* pParent, const CString& title, int number, bool allowNegative);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

protected:
	int& m_value;
	const bool m_allowNegative;
	const CString m_title;

protected:
	CSpinEdit m_editNumber;
};
