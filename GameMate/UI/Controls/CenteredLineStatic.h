#pragma once

#include <afxwin.h>

class CCenteredLineStatic : public CWnd
{
public:
    CCenteredLineStatic() = default;

protected:
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnSize(UINT nType, int cx, int cy);

    DECLARE_MESSAGE_MAP()
};

