#pragma once

#include "afxdialogex.h"

#include <optional>

#include "core/Settings.h"

class CBaseKeyEditDlg : protected CDialogEx
{
protected:
	DECLARE_DYNAMIC(CBaseKeyEditDlg)
	CBaseKeyEditDlg(CWnd* pParent);

	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_BIND_EDIT };
#endif

protected:
	DECLARE_MESSAGE_MAP()

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);

protected:
	virtual std::wstring GetActionText() const = 0;
	virtual void OnVkCodeAction(WORD vkCode, bool down) = 0;

protected:
	int m_keyPressedSubscriptionId = -1;
	CEdit m_editAction;
};

class CActionEditDlg : protected CBaseKeyEditDlg
{
public:
	[[nodiscard]] static std::optional<Action> EditAction(CWnd* pParent, const std::optional<Action>& action = std::nullopt);

private:
	DECLARE_DYNAMIC(CActionEditDlg)
	DECLARE_MESSAGE_MAP()

	CActionEditDlg(CWnd* pParent, std::optional<Action>& action);
	virtual BOOL OnInitDialog() override;

private: // CBaseKeyEditDlg
	std::wstring GetActionText() const override;
	void OnVkCodeAction(WORD vkCode, bool down) override;
	void OnOK() override;

private:
	std::optional<Action>& m_currentAction;
};

class CBindEditDlg : protected CBaseKeyEditDlg
{
public:
	[[nodiscard]] static std::optional<Bind> EditBind(CWnd* pParent, const std::optional<Bind>& editBind = std::nullopt);

private:
	DECLARE_DYNAMIC(CBindEditDlg)
	DECLARE_MESSAGE_MAP()

	CBindEditDlg(CWnd* pParent, std::optional<Bind>& action);

private: // CBaseKeyEditDlg	
	std::wstring GetActionText() const override;
	void OnVkCodeAction(WORD vkCode, bool down) override;
	void OnOK() override;

private:
	std::optional<Bind>& m_currentBind;
};
