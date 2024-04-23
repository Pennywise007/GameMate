#include <pch.h>
#include <afxtoolbarimages.h>

#include "resource.h"

#include "Crosshairs.h"

#include <Controls/DefaultWindowProc.h>
#include <Controls/Utils/WindowClassRegistration.h>

#include <ext/core/check.h>
#include <ext/std/filesystem.h>

namespace crosshair {

void LoadCrosshair(const Settings& crosshair, CBitmap& bitmap) EXT_THROWS(std::runtime_error)
{
    if (!crosshair.customCrosshairName.empty())
    {
        const auto fullFilePath = std::filesystem::get_exe_directory() / L"res" / crosshair.customCrosshairName;

        CImage image;
        EXT_CHECK(SUCCEEDED(image.Load(fullFilePath.c_str()))) << "Failed to load custom crosshair " << fullFilePath;
        EXT_CHECK(bitmap.Attach(image.Detach())) << "Failed to load custom crosshair";
    }
    else
    {
        constexpr auto kFirstResourceId = IDB_PNG_CROSSHAIR_0_16;

        auto crosshairResourceId = kFirstResourceId + 3 * (int)crosshair.type + (int)crosshair.size;

        CPngImage pngImage;
        EXT_CHECK(pngImage.Load(crosshairResourceId, AfxGetResourceHandle()))
            << "Failed to load image with type " << (int)crosshair.type << " and size " << (int)crosshair.size;
        EXT_CHECK(bitmap.Attach(pngImage.Detach())) << "Failed to load resouce";
    }

    ChangeCrosshairColor(bitmap, crosshair.color);
}

void ChangeCrosshairColor(CBitmap& bitmap, COLORREF color)
{
    BITMAP bm;
    bitmap.GetBitmap(&bm);

    std::unique_ptr<BYTE[]> pBits(new BYTE[bm.bmWidthBytes * bm.bmHeight]);
    ::ZeroMemory(pBits.get(), bm.bmWidthBytes * bm.bmHeight);
    ::GetBitmapBits(bitmap, bm.bmWidthBytes * bm.bmHeight, pBits.get());

    for (int i = 0; i < bm.bmWidth * bm.bmHeight; ++i) {
        BYTE* pPixel = pBits.get() + i * 4; // Assuming 32-bit RGBA format
        if (pPixel[3] != 0)  // Non-transparent pixel
        {
            pPixel[0] = GetBValue(color);
            pPixel[1] = GetGValue(color);
            pPixel[2] = GetRValue(color);
            // Alpha component remains unchanged (pPixel[3])
        }
    }
    ::SetBitmapBits(bitmap, bm.bmWidthBytes * bm.bmHeight, pBits.get());
}

void ResizeCrosshair(CBitmap& bitmap, const CSize& size)
{
    CBitmap srcBitmap;
    srcBitmap.Attach(bitmap.Detach());

    BITMAP bm = { 0 };
    srcBitmap.GetBitmap(&bm);
    auto srcSize = CSize(bm.bmWidth, bm.bmHeight);

    CWindowDC screenCDC(NULL);

    bitmap.CreateCompatibleBitmap(&screenCDC, size.cx, size.cy);

    CDC srcCompatCDC;
    srcCompatCDC.CreateCompatibleDC(&screenCDC);
    CDC destCompatCDC;
    destCompatCDC.CreateCompatibleDC(&screenCDC);

    CMemDC srcDC(srcCompatCDC, CRect(CPoint(), srcSize));
    auto oldSrcBmp = srcDC.GetDC().SelectObject(&srcBitmap);

    CMemDC destDC(destCompatCDC, CRect(CPoint(), size));
    auto oldDestBmp = destDC.GetDC().SelectObject(&bitmap);

    destDC.GetDC().StretchBlt(0, 0, size.cx, size.cy, &srcDC.GetDC(), 0, 0, srcSize.cx, srcSize.cy, SRCCOPY);

    srcDC.GetDC().SelectObject(oldSrcBmp);
    destDC.GetDC().SelectObject(oldDestBmp);

    ::DeleteDC(srcCompatCDC);
    ::DeleteDC(destCompatCDC);
}

BEGIN_MESSAGE_MAP(CrosshairWindow, CWnd)
    ON_WM_PAINT()
    ON_WM_NCDESTROY()
END_MESSAGE_MAP()

CrosshairWindow::~CrosshairWindow()
{
    DestroyWindow();
}

CrosshairWindow::CrosshairWindow()
{
    HINSTANCE instance = AfxGetInstanceHandle();
    const CString className(typeid(*this).name());

    WNDCLASSEX wndClass;
    if (!::GetClassInfoEx(instance, className, &wndClass))
    {
        memset(&wndClass, 0, sizeof(WNDCLASSEX));
        wndClass.cbSize = sizeof(WNDCLASSEX);
        wndClass.style = 0;
        wndClass.lpfnWndProc = ::DefMDIChildProc;
        wndClass.hInstance = instance;
        wndClass.lpszClassName = className;

        static WindowClassRegistrationLock registrator(wndClass);
    }

    CreateEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT, CString(typeid(*this).name()), NULL, WS_POPUP, CRect(), NULL, NULL);


    SetWindowLongPtr(GetSafeHwnd(), GWL_EXSTYLE, GetWindowLongPtr(GetSafeHwnd(), GWL_EXSTYLE) | WS_EX_LAYERED);
    SetLayeredWindowAttributes(RGB(255, 255, 255), 0, LWA_COLORKEY);
}

void CrosshairWindow::SetCrosshair(HWND hWndOfActiveWindow, const Settings& crosshair)
{
    try
    {
        auto old = m_crosshair.Detach();
        if (old != nullptr)
            ::DeleteObject(old);

        LoadCrosshair(crosshair, m_crosshair);
    }
    catch (...)
    {
        // TODO
    }

    BITMAP bm;
    ENSURE(m_crosshair.GetBitmap(&bm) != 0);

    CRect activeWindowRect;
    ::GetWindowRect(hWndOfActiveWindow, activeWindowRect);

    SetWindowPos(
        nullptr,
        activeWindowRect.CenterPoint().x - bm.bmWidth / 2,
        activeWindowRect.CenterPoint().y - bm.bmHeight / 2,
        bm.bmWidth,
        bm.bmHeight,
        SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOOWNERZORDER);

    if (m_attachedWindowHwnd != nullptr && ::IsWindow(m_attachedWindowHwnd))
    {
        auto wnd = CWnd::FromHandle(m_attachedWindowHwnd);
        DefaultWindowProc::RemoveCallback(*wnd, WM_WINDOWPOSCHANGED, this);
    }

    m_attachedWindowHwnd = hWndOfActiveWindow;

    auto wnd = CWnd::FromHandle(m_attachedWindowHwnd);
    DefaultWindowProc::OnWindowMessage(*wnd, WM_WINDOWPOSCHANGED, [&, width = bm.bmWidth, height = bm.bmHeight](HWND hWnd, WPARAM wParam, LPARAM lParam, auto)
    {
        const auto* lpwndpos = reinterpret_cast<WINDOWPOS*>(lParam);
        if ((lpwndpos->flags & SWP_NOMOVE) && (lpwndpos->flags & SWP_NOSIZE))
            return;

        CRect activeWindowRect;
        ::GetWindowRect(hWnd, activeWindowRect);

        SetWindowPos(
            nullptr,
            activeWindowRect.CenterPoint().x - width / 2,
            activeWindowRect.CenterPoint().y - height / 2,
            width,
            height,
            SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOOWNERZORDER | SWP_NOREDRAW);
    }, this);
}

afx_msg void CrosshairWindow::OnPaint()
{
    BITMAP bm;
    ENSURE(m_crosshair.GetBitmap(&bm) != 0);

    CPaintDC dc(this);

    std::unique_ptr<BYTE[]> pBits(new BYTE[bm.bmWidthBytes * bm.bmHeight]);
    ::ZeroMemory(pBits.get(), bm.bmWidthBytes * bm.bmHeight);
    ::GetBitmapBits(m_crosshair, bm.bmWidthBytes * bm.bmHeight, pBits.get());

    for (int i = 0; i < bm.bmWidth * bm.bmHeight; ++i) {
        BYTE* pPixel = pBits.get() + i * 4; // Assuming 32-bit RGBA format

        if (pPixel[3] != 0)  // Non-transparent pixel
            dc.SetPixel(CPoint(i / bm.bmWidth, i % bm.bmWidth), RGB(pPixel[2], pPixel[1], pPixel[0]));
    }
}

void CrosshairWindow::OnNcDestroy()
{
    CWnd::OnNcDestroy();

    if (m_attachedWindowHwnd != nullptr)
    {
        auto wnd = CWnd::FromHandle(m_attachedWindowHwnd);
        if (wnd)
            DefaultWindowProc::RemoveCallback(*wnd, WM_WINDOWPOSCHANGED, this);
    }
}

} // namespace crosshair
