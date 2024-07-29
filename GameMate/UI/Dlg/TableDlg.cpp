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
    ON_WM_ERASEBKGND()
    ON_WM_WINDOWPOSCHANGED()
END_MESSAGE_MAP()

BOOL CTableDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    m_buttonAdd.UseCustomBackgroundDraw(true);
    m_buttonRemove.UseCustomBackgroundDraw(true);

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

BOOL CTableDlg::OnEraseBkgnd(CDC* pDC)
{
    // Fill COLOR_3DFACE everywhere except child controls client areas
    CRect rect;
    GetClientRect(rect);

    CRgn clientRgn;
    clientRgn.CreateRectRgnIndirect(rect);

    for (CWnd* pChild = GetWindow(GW_CHILD); pChild != nullptr; pChild = pChild->GetNextWindow())
    {
        if (pChild->IsWindowVisible())
        {
            CRect ctrlRect;
            pChild->GetWindowRect(ctrlRect);
            ScreenToClient(ctrlRect);

            CRgn ctrlRgn;
            ctrlRgn.CreateRectRgnIndirect(ctrlRect);
            clientRgn.CombineRgn(&clientRgn, &ctrlRgn, RGN_DIFF);
            ctrlRgn.DeleteObject();
        }
    }

    CBrush brush(::GetSysColor(COLOR_3DFACE));
    pDC->FillRgn(&clientRgn, &brush);
    clientRgn.DeleteObject();

    return TRUE;
}

void CTableDlg::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
    CDialogEx::OnWindowPosChanged(lpwndpos);
    // Sometimes we see artifacts on controls because of custom OnEraseBackground, redraw window to fix it
    RedrawWindow();
}

void CTableDlg::OnOK()
{
    // CDialogEx::OnOK();
}

void CTableDlg::OnCancel()
{
    // CDialogEx::OnCancel();
}
