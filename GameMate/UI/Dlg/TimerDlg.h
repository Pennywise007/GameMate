#pragma once

#include "afxdialogex.h"
#include "chrono"

#include <Controls/Button/IconButton/IconButton.h>

#include <core/events.h>

class CTimerWindow : public CWnd
{
	DECLARE_DYNAMIC(CTimerWindow)
	DECLARE_MESSAGE_MAP()

	inline static UINT kUpdateWindowTimerID = 0;
public:
	void StartTimer();
	void StopTimer();
	void ResetTimer();
	void DisplayHours(bool show);
	void SetColors(COLORREF backColor, COLORREF textColor);

private:
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnSize(UINT nType, int cx, int cy);

private:
	void recalcFont();
	CString getDisplayText() const;

private:
	bool m_timerRunning = false;
	std::chrono::milliseconds m_savedMilliseconds;
	std::chrono::high_resolution_clock::time_point m_startPoint;

private: // drawing info
	int m_displayHours = true;
	int m_logFontSize = -10;
	COLORREF m_backColor;
	COLORREF m_textColor = RGB(0, 0, 0);
};

class CTimerDlg : public CDialogEx, ext::events::ScopeSubscription<ITimerNotifications>
{
	DECLARE_DYNAMIC(CTimerDlg)

public:
	CTimerDlg(CWnd* pParent = nullptr);

	enum { IDD = IDD_DIALOG_TIMER };
protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	virtual BOOL OnInitDialog() override;
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedCheckStart();
	afx_msg void OnBnClickedButtonReset();
	afx_msg void OnBnClickedMfcbuttonTimerSettigns();

	DECLARE_MESSAGE_MAP()

private: // ITimerNotifications
	void OnShowHideTimer() override;
	void OnStartOrPauseTimer() override;
	void OnResetTimer() override;

private:
	void updateButtonText();
	void showFullInterface();
	void minimizeInterface();

private:
	CButton m_checkStart;
	CButton m_buttonReset;
	CIconButton m_buttonTimerSettings;
	CTimerWindow m_timerWindow;

private:
	CPoint m_timerOffsetFromWindow;
	CRect m_fullWindowRect;
	bool m_interfaceMinimized = false;
	bool m_childDlgOpened = false;
};
