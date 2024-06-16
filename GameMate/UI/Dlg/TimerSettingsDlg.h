#pragma once

#include "afxdialogex.h"

#include "core/Settings.h"

#include "Controls/MFC/CMFCColorSelection.h"

class CTimerSettings : public CDialogEx
{
	DECLARE_DYNAMIC(CTimerSettings)

public:
	CTimerSettings(CWnd* pParent = nullptr);   // standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_TIMER_SETTINGS };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;
	virtual BOOL OnInitDialog() override;
	virtual void OnOK() override;
	afx_msg void OnBnClickedButtonChangeStartBind();
	afx_msg void OnBnClickedButtonChangeResetBind();

	DECLARE_MESSAGE_MAP()
private:
	CButton m_checkHideInterface;
	CButton m_checkDisplayHours;
	CMFCColorButtonEx m_textColor;
	CMFCColorButtonEx m_backgroundColor;
	CStatic m_staticStartPauseBind;
	CStatic m_staticResetBind;

private:
	Bind m_pauseBind;
	Bind m_resetBind;
};
