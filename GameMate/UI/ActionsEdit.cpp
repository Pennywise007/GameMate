#include "pch.h"
#include "afxdialogex.h"
#include "resource.h"

#include "ActionsEdit.h"
#include "InputManager.h"

#include <Controls/Layout/Layout.h>

#include <ext/core/check.h>

IMPLEMENT_DYNAMIC(CActionsEditDlg, CActionsEditBase<CDialogEx>)

BEGIN_MESSAGE_MAP(CActionsEditDlg, CActionsEditBase<CDialogEx>)
END_MESSAGE_MAP()

CActionsEditDlg::CActionsEditDlg(CWnd* pParent, Actions& actions)
	: CActionsEditBase(IDD, pParent)
{
	SetActions(actions);
}

BOOL CActionsEditDlg::OnInitDialog()
{
	CActionsEditBase::OnInitDialog();

	OnInit();

	return TRUE;
}

void CActionsEditDlg::PreSubclassWindow()
{
	// Changing dlg to view
	LONG lStyle = GetWindowLong(m_hWnd, GWL_STYLE);
	lStyle &= ~WS_CHILD;
	lStyle |= WS_POPUP;
	SetWindowLong(m_hWnd, GWL_STYLE, lStyle);

	CActionsEditBase::PreSubclassWindow();
}

std::optional<Actions> CActionsEditDlg::ExecModal(CWnd* pParent, const Actions& currentActions)
{
	Actions actions = currentActions;
	if (CActionsEditDlg(pParent, actions).DoModal() != IDOK)
		return std::nullopt;

	return actions;
}

IMPLEMENT_DYNCREATE(CActionsEditView, CActionsEditBase<CFormView>)

BEGIN_MESSAGE_MAP(CActionsEditView, CActionsEditBase<CFormView>)
END_MESSAGE_MAP()

CActionsEditView::CActionsEditView()
	: CActionsEditBase(IDD)
{
}

void CActionsEditView::Init(Actions& actions, OnSettingsChangedCallback callback)
{
	SetActions(actions);
	m_onSettingsChangedCallback = std::move(callback);

	OnInit();

	// We need to hide OK button and remove extra offset from the sides
	GetDlgItem(IDOK)->ShowWindow(SW_HIDE);

	auto parent = GetParent();
	ASSERT(parent);

	CRect rect;
	parent->GetClientRect(rect);
	rect.InflateRect(5, 5);
	MoveWindow(rect);

	Layout::AnchorWindow(*this, *parent, { AnchorSide::eLeft }, AnchorSide::eLeft, 100);
	Layout::AnchorWindow(*this, *parent, { AnchorSide::eTop }, AnchorSide::eTop, 100);
	Layout::AnchorWindow(*this, *parent, { AnchorSide::eRight }, AnchorSide::eRight, 100);
	Layout::AnchorWindow(*this, *parent, { AnchorSide::eBottom }, AnchorSide::eBottom, 100);

	// disable scrols
	CSize sizeTotal(0, 0);
	SetScrollSizes(MM_TEXT, sizeTotal);

	LayoutLoader::ApplyLayoutFromResource(*this, m_lpszTemplateName);
}

BOOL CActionsEditView::PreCreateWindow(CREATESTRUCT& cs)
{
	auto res = CActionsEditBase::PreCreateWindow(cs);

	cs.style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU | WS_POPUP | DS_MODALFRAME);
	cs.style |= WS_CHILDWINDOW;

	cs.dwExStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);

	return res;
}

void CActionsEditView::OnSettingsChanged()
{
	CActionsEditBase::OnSettingsChanged();

	if (m_onSettingsChangedCallback)
		m_onSettingsChangedCallback();
}
