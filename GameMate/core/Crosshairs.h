#pragma once

#include <afxwin.h>

#include <ext/error/exception.h>

#include "Settings.h"

namespace crosshair {

void LoadCrosshair(const Settings& crosshair, CBitmap& bitmap) EXT_THROWS(std::runtime_error);
void ChangeCrosshairColor(CBitmap& bitmap, COLORREF color);
void ResizeCrosshair(CBitmap& bitmap, const CSize& size);

// Layered non clickable topmost window to show crosshair in games
class CrosshairWindow : public CWnd
{
    DECLARE_MESSAGE_MAP()
public:
    CrosshairWindow();
    ~CrosshairWindow();

    void SetCrosshair(HWND hWndOfActiveWindow, const Settings& crosshair);

private:
    afx_msg void OnPaint();
    afx_msg void OnNcDestroy();

private:
    HWND m_attachedWindowHwnd = nullptr;
    CBitmap m_crosshair;
};

} // namespace crosshair
