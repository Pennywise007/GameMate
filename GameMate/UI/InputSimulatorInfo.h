#pragma once
#include "afxdialogex.h"

class CInputSimulatorInfo : public CDialogEx
{
	DECLARE_DYNAMIC(CInputSimulatorInfo)

public:
	CInputSimulatorInfo(CWnd* pParent = nullptr);   // standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_INPUT_SIMULATOR };
#endif

protected:
	DECLARE_MESSAGE_MAP()
};
