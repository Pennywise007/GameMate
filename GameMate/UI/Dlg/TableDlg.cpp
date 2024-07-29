#include "pch.h"
#include "resource.h"

#include "TableDlg.h"

#include <Controls/Layout/Layout.h>
#include <Controls/Tooltip/ToolTip.h>

#include <ext/core/check.h>

IMPLEMENT_DYNAMIC(CTableDlg, CDialogEx)

CTableDlg::CTableDlg(CWnd* pParent)
	: CDialogEx(IDD_DIALOG_TABLE, pParent)
{
}

void CTableDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_TABLE, m_table);
    DDX_Control(pDX, IDC_BUTTON_ADD, m_buttonAdd);
    DDX_Control(pDX, IDC_BUTTON_REMOVE, m_buttonRemove);
    DDX_Control(pDX, IDC_STATIC_GROUP_TITLE, m_staticTitle);
}

BEGIN_MESSAGE_MAP(CTableDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BUTTON_ADD, &CTableDlg::OnBnClickedButtonAdd)
    ON_BN_CLICKED(IDC_BUTTON_REMOVE, &CTableDlg::OnBnClickedButtonRemove)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_TABLE, &CTableDlg::OnLvnItemchangedTable)
    ON_WM_WINDOWPOSCHANGED()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

BOOL CTableDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    LayoutLoader::ApplyLayoutFromResource(*this, m_lpszTemplateName);

    return TRUE;
}

controls::list::widgets::SubItemsEditor<CListGroupCtrl>& CTableDlg::GetTable()
{
    return m_table;
}

void CTableDlg::UpdateRemoveButtonState()
{
    m_buttonRemove.EnableWindow(m_table.GetSelectedCount() > 0);
}

void CTableDlg::Init(
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

void CTableDlg::OnBnClickedButtonAdd()
{
    EXT_EXPECT(!!m_onAddClicked);
    m_onAddClicked();
}

void CTableDlg::OnBnClickedButtonRemove()
{
    EXT_EXPECT(!!m_onRemoveClicked);
    m_onRemoveClicked();
}

void CTableDlg::OnLvnItemchangedTable(NMHDR* pNMHDR, LRESULT* pResult)
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

void CTableDlg::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
    CDialogEx::OnWindowPosChanged(lpwndpos);
    // Fixing flickering
    RedrawWindow();
}

BOOL CTableDlg::OnEraseBkgnd(CDC* pDC)
{
    // Sometimes we can see artifacts on the right and left side from the table, just draw background there
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
    rect.left = 0;
    rect.right = tableRect.left - 1;

    return TRUE;
}

