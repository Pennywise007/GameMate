#pragma once

#include <afxwin.h>

#include <ext/error/exception.h>

#include "Settings.h"

namespace process_toolkit::crosshair {

void LoadCrosshair(const Settings& crosshair, CBitmap& bitmap) EXT_THROWS(std::runtime_error);
void ChangeCrosshairColor(CBitmap& bitmap, COLORREF color);
void ResizeCrosshair(CBitmap& bitmap, const CSize& size);

// Transparent window with drawable bitmap
class TransparentWindowWithBitmap : protected CWnd
{
    DECLARE_MESSAGE_MAP()
public:
    TransparentWindowWithBitmap();
    virtual ~TransparentWindowWithBitmap();

    void Create(CBitmap&& bitmap, UINT extraExFlags);

private:
    afx_msg void OnPaint();

private:
    CBitmap m_crosshair;
    const CString m_className;
};

// Layered non clickable topmost window to show cross hair in games
class AttachableCrosshairWindow : protected TransparentWindowWithBitmap
{
public:
    AttachableCrosshairWindow();
    ~AttachableCrosshairWindow();

    void AttachCrosshairToWindow(const Settings& settings, HWND hWndOfActiveWindow);
    void RemoveCrosshairWindow();

    void OnWindowPosChanged(HWND hwnd);
private:
    HWINEVENTHOOK m_windowPosChangedHook = nullptr;
    HWND m_attachedWindowHwnd = nullptr;
};

// Transparent window with cursor image which suppose to replace crosshair
class CursorReplacingWindow : protected TransparentWindowWithBitmap
{
    DECLARE_MESSAGE_MAP()
public:
    CursorReplacingWindow() = default;

    void Create(CBitmap&& cursorImage);
    
    using TransparentWindowWithBitmap::DestroyWindow;
    using TransparentWindowWithBitmap::m_hWnd;

    CRect GetWindowRect() const;
    using TransparentWindowWithBitmap::SetWindowPos;

private:
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
};

} // namespace crosshair
