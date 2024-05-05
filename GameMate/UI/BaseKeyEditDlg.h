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
	enum { IDD = IDD_DIALOG_ACTION_EDIT };
#endif

protected:
	DECLARE_MESSAGE_MAP()

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();

protected:
	virtual std::wstring GetActionText() const = 0;
	virtual void OnVkKeyAction(WORD vkKey, bool down) = 0;

protected:
	int m_keyPressedSubscriptionId = -1;
	CEdit m_editAction;
};

class CMacrosActionEditDlg : protected CBaseKeyEditDlg
{
public:
	[[nodiscard]] static std::optional<MacrosAction> EditMacros(CWnd* pParent, const std::optional<MacrosAction>& editMacros = std::nullopt);

private:
	DECLARE_DYNAMIC(CMacrosActionEditDlg)
	DECLARE_MESSAGE_MAP()

	CMacrosActionEditDlg(CWnd* pParent, std::optional<MacrosAction>& action);

private: // CBaseEditDlg
	std::wstring GetActionText() const override;
	void OnVkKeyAction(WORD vkKey, bool down) override;
	void OnOK() override;

private:
	std::optional<MacrosAction>& m_currentAction;
};

class CBindEditDlg : protected CBaseKeyEditDlg
{
public:
	[[nodiscard]] static std::optional<Bind> EditBind(CWnd* pParent, const std::optional<Bind>& editBind = std::nullopt);

private:
	DECLARE_DYNAMIC(CBindEditDlg)
	DECLARE_MESSAGE_MAP()

	CBindEditDlg(CWnd* pParent, std::optional<Bind>& action);

private: // CBaseEditDlg	
	std::wstring GetActionText() const override;
	void OnVkKeyAction(WORD vkKey, bool down) override;
	void OnOK() override;

private:
	std::optional<Bind>& m_currentBind;
};
