#pragma once

#include <functional>

#include <Controls/Tables/List/ListGroupCtrl/ListGroupCtrl.h>
#include <Controls/Button/IconButton/IconButton.h>

#include "UI/Controls/CenteredLineStatic.h"

class CTableView : public CDialogEx
{
	DECLARE_DYNAMIC(CTableView)

public:
	CTableView(CWnd* parent);

public:
	enum { IDD = IDD_VIEW_TABLE };

	void Init(
		const wchar_t* title,
		const wchar_t* addButtonToolTip,
		const wchar_t* deleteButtonToolTip,
		std::function<void()>&& onAddClicked,
		std::function<void()>&& onRemoveClicked);

	[[nodiscard]] CListGroupCtrl& GetList();

protected:
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnBnClickedButtonRemove();
	afx_msg void OnLvnItemchangedTable(NMHDR* pNMHDR, LRESULT* pResult);

protected:
	CListGroupCtrl m_table;
	CIconButton m_buttonAdd;
	CIconButton m_buttonRemove;
	CCenteredLineStatic m_staticTitle;

	std::function<void()> m_onAddClicked;
	std::function<void()> m_onRemoveClicked;
};
