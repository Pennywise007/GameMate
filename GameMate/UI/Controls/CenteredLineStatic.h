#pragma once

#include <afxwin.h>

class CCenteredLineStatic : public CWnd
{
public:
    CCenteredLineStatic() = default;

protected:
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);

    DECLARE_MESSAGE_MAP()
};

