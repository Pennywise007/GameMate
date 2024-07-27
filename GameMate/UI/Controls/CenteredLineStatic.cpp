#include "pch.h"

#include "CenteredLineStatic.h"

#include <ext/scope/defer.h>

BEGIN_MESSAGE_MAP(CCenteredLineStatic, CStatic)
    ON_WM_PAINT()
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

void CCenteredLineStatic::OnPaint()
{
    CRect rect;
    GetClientRect(&rect);

    CPaintDC dcPaint(this);
    CMemDC memDC(dcPaint, rect);
    CDC& dc = memDC.GetDC();

    dc.FillSolidRect(rect, ::GetSysColor(COLOR_3DFACE));

    // Get text length
    dc.SetBkMode(TRANSPARENT);
    CFont* pOldFont = dc.SelectObject(GetFont());
    EXT_DEFER(dc.SelectObject(pOldFont));

    CString text;
    GetWindowText(text);

    CSize textSize = dc.GetTextExtent(text);

    // Positioning text
    int xCenter = rect.Width() / 2;
    int yCenter = rect.Height() / 2;

    int textLeft = xCenter - (textSize.cx / 2);
    int textTop = yCenter - (textSize.cy / 2);

    int lineY = yCenter;

    // Draw lines
    dc.MoveTo(rect.left, lineY);
    dc.LineTo(textLeft - 5, lineY);

    dc.MoveTo(textLeft + textSize.cx + 5, lineY);
    dc.LineTo(rect.right, lineY);

    // Draw text
    dc.TextOut(textLeft, textTop, text);
}

void CCenteredLineStatic::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);
    RedrawWindow();
}

BOOL CCenteredLineStatic::OnEraseBkgnd(CDC* pDC)
{
    return TRUE;
}
