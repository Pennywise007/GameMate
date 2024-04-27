#pragma once

#include <afxwin.h>

#include <ext/error/exception.h>

#include "Settings.h"

namespace crosshair {

void LoadCrosshair(const Settings& crosshair, CBitmap& bitmap) EXT_THROWS(std::runtime_error);
void ChangeCrosshairColor(CBitmap& bitmap, COLORREF color);
void ResizeCrosshair(CBitmap& bitmap, const CSize& size);

// Layered non clickable topmost window to show crosshair in games
class CrosshairWindow : protected CWnd
{
    DECLARE_MESSAGE_MAP()
public:
    CrosshairWindow();
    ~CrosshairWindow();

    void AttachCrosshair(HWND hWndOfActiveWindow, const Settings& crosshair);
    void HideWindow();

    void OnWindowPosChanged(HWND hwnd);

private:
    afx_msg void OnPaint();
    afx_msg void OnNcDestroy();

private:
    HWINEVENTHOOK m_windowPosChangedHook = nullptr;
    HWND m_attachedWindowHwnd = nullptr;
    CBitmap m_crosshair;
};

} // namespace crosshair
