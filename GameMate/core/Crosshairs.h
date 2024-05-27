#pragma once

#include <afxwin.h>

#include <ext/error/exception.h>

#include "Settings.h"

namespace process_toolkit::crosshair {

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

    using CWnd::m_hWnd;
    using CWnd::SetWindowPos;

    void InitCrosshairWindow(const Settings& crosshair);
    void AttachCrosshairToWindow(HWND hWndOfActiveWindow);
    void RemoveCrosshairWindow();

    CRect GetWindowRect() const;

    void OnWindowPosChanged(HWND hwnd);

private:
    afx_msg void OnPaint();

private:
    HWINEVENTHOOK m_windowPosChangedHook = nullptr;
    HWND m_attachedWindowHwnd = nullptr;
    CBitmap m_crosshair;
};

} // namespace crosshair
