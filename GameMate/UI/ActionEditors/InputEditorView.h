#pragma once

#include "afxext.h"

#include "core/Settings.h"

#include "UI/ActionEditors/InputEditor.h"

class CInputEditorBaseView : public InputEditor
{
protected:
	CInputEditorBaseView();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_VIEW_EDIT_INPUT };
#endif

protected: // InputEditor
	void PostInit(const std::shared_ptr<IBaseInput>& input) override;
	std::shared_ptr<IBaseInput> TryFinishDialog() override;

protected:
	DECLARE_MESSAGE_MAP()

	void DoDataExchange(CDataExchange* pDX) override;
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);

protected:
	virtual const wchar_t* GetDescription() const = 0;
	virtual std::wstring GetActionEditText() const = 0;

protected:
	int m_keyPressedSubscriptionId = -1;
	std::shared_ptr<IBaseInput> m_editableInput;

protected:
	CEdit m_editAction;
};

class CKeyEditorView : public CInputEditorBaseView
{
protected:
	DECLARE_DYNCREATE(CKeyEditorView)
	CKeyEditorView() = default;

public: // InputEditor
	std::shared_ptr<IBaseInput> TryFinishDialog() override;

private: // CInputEditorBaseView
	const wchar_t* GetDescription() const override;
	std::wstring GetActionEditText() const override;
};

class CBindEditorView : public CInputEditorBaseView
{
protected:
	DECLARE_DYNCREATE(CBindEditorView)
	CBindEditorView() = default;

public: // InputEditor
	std::shared_ptr<IBaseInput> TryFinishDialog() override;

private: // CInputEditorBaseView
	const wchar_t* GetDescription() const override;
	std::wstring GetActionEditText() const override;
};

class CActionEditorView : public CInputEditorBaseView
{
protected:
	DECLARE_DYNCREATE(CActionEditorView)
	CActionEditorView() = default;

protected: // InputEditor
	std::shared_ptr<IBaseInput> TryFinishDialog() override;

private: // CInputEditorBaseView	
	const wchar_t* GetDescription() const override;
	std::wstring GetActionEditText() const override;
};
