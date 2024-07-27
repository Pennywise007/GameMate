#include "pch.h"
#include "resource.h"

#include "TableView.h"

#include <Controls/Layout/Layout.h>
#include <Controls/Tooltip/ToolTip.h>

#include <ext/core/check.h>

IMPLEMENT_DYNCREATE(CTableView, CFormView)

CTableView::CTableView()
	: CFormView(IDD_VIEW_TABLE)
{
}

void CTableView::DoDataExchange(CDataExchange* pDX)
{
    CFormView::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_TABLE, m_table);
    DDX_Control(pDX, IDC_BUTTON_ADD, m_buttonAdd);
    DDX_Control(pDX, IDC_BUTTON_REMOVE, m_buttonRemove);
    DDX_Control(pDX, IDC_STATIC_GROUP_TITLE, m_staticTitle);
}

BEGIN_MESSAGE_MAP(CTableView, CFormView)
    ON_BN_CLICKED(IDC_BUTTON_ADD, &CTableView::OnBnClickedButtonAdd)
    ON_BN_CLICKED(IDC_BUTTON_REMOVE, &CTableView::OnBnClickedButtonRemove)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_TABLE, &CTableView::OnLvnItemchangedTable)
    ON_WM_WINDOWPOSCHANGED()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

BOOL CTableView::PreCreateWindow(CREATESTRUCT& cs)
{
    auto res = CFormView::PreCreateWindow(cs);
    cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
    return res;
}

void CTableView::OnInitialUpdate()
{
    CFormView::OnInitialUpdate();

    m_buttonAdd.UseCustomBackgroundDraw(true);
    m_buttonRemove.UseCustomBackgroundDraw(true);

    LayoutLoader::ApplyLayoutFromResource(*this, m_lpszTemplateName);
}

controls::list::widgets::SubItemsEditor<CListGroupCtrl>& CTableView::GetTable()
{
    return m_table;
}

void CTableView::UpdateRemoveButtonState()
{
    m_buttonRemove.EnableWindow(m_table.GetSelectedCount() > 0);
}

void CTableView::Init(
    const wchar_t* title,
    const wchar_t* addButtonToolTip,
    std::function<void()>&& onAddClicked,
    std::function<void()>&& onRemoveClicked)
{
    m_staticTitle.SetWindowTextW(title);

    const auto initButton = [](CIconButton& button, UINT bitmapId, const wchar_t* tooltip) {
        button.SetBitmap(bitmapId, Alignment::CenterCenter);
        button.SetWindowTextW(L"");
        controls::SetTooltip(button, tooltip);
        };
    initButton(m_buttonAdd, IDB_PNG_ADD, addButtonToolTip);
    initButton(m_buttonRemove, IDB_PNG_DELETE, L"Delete selected items");

    m_onAddClicked = std::move(onAddClicked);
    m_onRemoveClicked = std::move(onRemoveClicked);
}

void CTableView::OnBnClickedButtonAdd()
{
    EXT_EXPECT(!!m_onAddClicked);
    m_onAddClicked();
}

void CTableView::OnBnClickedButtonRemove()
{
    EXT_EXPECT(!!m_onRemoveClicked);
    m_onRemoveClicked();
}

void CTableView::OnLvnItemchangedTable(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    if (pNMLV->uChanged & LVIF_STATE)
    {
        if ((pNMLV->uNewState & LVIS_SELECTED) != (pNMLV->uOldState & LVIS_SELECTED))
            // selection changed
            UpdateRemoveButtonState();
    }
    *pResult = 0;
}

void CTableView::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
    CFormView::OnWindowPosChanged(lpwndpos);
    // Fixing flickering
    RedrawWindow();
}

BOOL CTableView::OnEraseBkgnd(CDC* pDC)
{
    // Sometimes we can see artifacts on right side from the table, just draw background there
    CRect rect;
    GetClientRect(rect);

    CRect tableRect;
    m_table.GetWindowRect(tableRect);
    ScreenToClient(tableRect);
    rect.left = tableRect.right + 1;

    CRect titleRect;
    m_staticTitle.GetClientRect(titleRect);
    ScreenToClient(titleRect);
    rect.top = titleRect.bottom + 1;

    pDC->FillSolidRect(rect, ::GetSysColor(COLOR_3DFACE));

    return TRUE;
}
