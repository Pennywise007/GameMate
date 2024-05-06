#pragma once
#include "afxdialogex.h"

class CInputSimulatorInfoDlg: public CDialogEx
{
	DECLARE_DYNAMIC(CInputSimulatorInfoDlg)

public:
	CInputSimulatorInfoDlg(CWnd* pParent = nullptr);   // standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_INPUT_SIMULATOR };
#endif

protected:
	DECLARE_MESSAGE_MAP()
};
