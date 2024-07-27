#include "pch.h"
#include "GameMate.h"
#include "UI/View/CTableView.h"

#include <Controls/Layout/Layout.h>
#include <Controls/Tooltip/ToolTip.h>

#include <ext/core/check.h>

IMPLEMENT_DYNAMIC(CTableView, CDialogEx)

CTableView::CTableView(CWnd* parent)
	: CDialogEx(IDD_VIEW_TABLE, parent)
{
}

void CTableView::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_TABLE, m_table);
    DDX_Control(pDX, IDC_BUTTON_ADD, m_buttonAdd);
    DDX_Control(pDX, IDC_BUTTON_REMOVE, m_buttonRemove);
    DDX_Control(pDX, IDC_STATIC_GROUP_TITLE, m_staticTitle);
}

BEGIN_MESSAGE_MAP(CTableView, CDialogEx)
    ON_BN_CLICKED(IDC_BUTTON_ADD, &CTableView::OnBnClickedButtonAdd)
    ON_BN_CLICKED(IDC_BUTTON_REMOVE, &CTableView::OnBnClickedButtonRemove)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_TABLE, &CTableView::OnLvnItemchangedTable)
END_MESSAGE_MAP()

BOOL CTableView::PreCreateWindow(CREATESTRUCT& cs)
{
    auto res = CDialogEx::PreCreateWindow(cs);
    cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
    return res;
}

CListGroupCtrl& CTableView::GetList()
{
    return m_table;
}

BOOL CTableView::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    LayoutLoader::ApplyLayoutFromResource(*this, m_lpszTemplateName);

    return TRUE;
}

void CTableView::Init(
    const wchar_t* title,
    const wchar_t* addButtonToolTip,
    const wchar_t* deleteButtonToolTip,
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
    initButton(m_buttonRemove, IDB_PNG_DELETE, deleteButtonToolTip);

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
        {
            // selection changed
            m_buttonRemove.EnableWindow(m_table.GetSelectedCount() > 0);
        }
    }
    *pResult = 0;
}
