
#pragma once

#include <Controls/TabControl/TabControl.h>

class CMainDlg : public CDialogEx
{
// Construction
public:
	CMainDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MAIN_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

// Implementation
protected:
	DECLARE_MESSAGE_MAP()

	// Generated message map functions
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnPaint();
	afx_msg void OnDestroy();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnTcnSelchangeTabcontrolGames(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCbnSelchangeComboInputDriver();
	afx_msg void OnBnClickedMfcbuttonInputSimulatorInfo();

private:
	void UpdateDriverInfoButton();

private:
	HICON m_hIcon;
	CComboBox m_inputSimulator;
	CTabControl m_tabControlGames;
	CMFCButton m_buttonInputDriverInfo;
};
