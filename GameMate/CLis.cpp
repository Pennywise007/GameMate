#include "pch.h"
#include "CLis.h"

BEGIN_MESSAGE_MAP(CLi, CListCtrl)
    ON_WM_SIZE()
    //ON_WM_MEASUREITEM_REFLECT()
END_MESSAGE_MAP()


void CLi::OnSize(UINT nType, int cx, int cy)
{
    CListCtrl::OnSize(nType, cx, cy);

    int controlWidth = cx;
    controlWidth -= GetColumnWidth(0);

    if (controlWidth < 0)
        controlWidth = 0;
    SetColumnWidth(1, controlWidth - 30);

    RedrawWindow();
    RedrawItems(0, GetItemCount());
    GetHeaderCtrl()->RedrawWindow();

    int h;
    int w;
    GetItemSpacing(TRUE, &h, &w);
    TRACE(L"w %d h %d\n", w, h);
}

void CLi::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
    int asd = 0;
    ++asd;
    // TODO: Add your message handler code here and/or call default

    //CListCtrl::OnMeasureItem(lpMeasureItemStruct);
}
