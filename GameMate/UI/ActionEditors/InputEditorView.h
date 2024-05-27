#pragma once

#include "afxext.h"

#include "core/Settings.h"

#include "../ActionsEditor.h"

class CInputEditorBaseView : public ActionsEditor
{
protected:
	CInputEditorBaseView();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_VIEW_EDIT_INPUT };
#endif

protected: // ActionsEditor
	void SetAction(const Action& action) override;
	Action GetAction() override;

protected:
	DECLARE_MESSAGE_MAP()

	virtual void OnInitialUpdate() override;
	void DoDataExchange(CDataExchange* pDX) override;
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);

protected:
	virtual std::wstring GetActionText() const = 0;
	virtual void OnVkCodeAction(WORD vkCode, bool down) = 0;

protected:
	int m_keyPressedSubscriptionId = -1;
	Action m_currentAction;

protected:
	CEdit m_editAction;
};

class CActionEditorView : public CInputEditorBaseView
{
protected:
	DECLARE_DYNCREATE(CActionEditorView)
	CActionEditorView() = default;

protected: // ActionsEditor
	bool CanClose() const override;

private: // CInputEditorBaseView	
	void OnInitialUpdate() override;
	std::wstring GetActionText() const override;
	void OnVkCodeAction(WORD vkCode, bool down) override;
};

class CBindEditorView : public CInputEditorBaseView
{
protected:
	DECLARE_DYNCREATE(CBindEditorView)
	CBindEditorView() = default;

public: // ActionsEditor
	bool CanClose() const override;
	void SetAction(const Action& action) override;
	Action GetAction() override;

	void SetBind(Bind bind);
	Bind GetBind() const;

private: // CInputEditorBaseView
	void OnInitialUpdate() override;
	std::wstring GetActionText() const override;
	void OnVkCodeAction(WORD vkCode, bool down) override;

private:
	Bind m_currentBind;
};
