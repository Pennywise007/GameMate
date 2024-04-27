#include <pch.h>
#include <afxtoolbarimages.h>

#include "resource.h"

#include "Crosshairs.h"

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

CrosshairWindow* g_crosshairWindow = nullptr;

void CALLBACK WinPosChangedProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
    g_crosshairWindow->OnWindowPosChanged(hwnd);
}

BEGIN_MESSAGE_MAP(CrosshairWindow, CWnd)
    ON_WM_PAINT()
    ON_WM_NCDESTROY()
END_MESSAGE_MAP()

CrosshairWindow::CrosshairWindow()
{
    g_crosshairWindow = this;

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

CrosshairWindow::~CrosshairWindow()
{
    DestroyWindow();
}

void CrosshairWindow::AttachCrosshair(HWND hWndOfActiveWindow, const Settings& crosshair)
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

    m_attachedWindowHwnd = hWndOfActiveWindow;

    OnWindowPosChanged(m_attachedWindowHwnd);
    RedrawWindow();

    DWORD pid = 0;
    GetWindowThreadProcessId(hWndOfActiveWindow, &pid);
    m_windowPosChangedHook = SetWinEventHook(EVENT_OBJECT_LOCATIONCHANGE, EVENT_OBJECT_LOCATIONCHANGE, NULL, WinPosChangedProc, pid, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
}

void CrosshairWindow::HideWindow()
{
    if (m_windowPosChangedHook)
    {
        UnhookWinEvent(m_windowPosChangedHook);
        m_windowPosChangedHook = nullptr;
    }
    ShowWindow(SW_HIDE);
}

void CrosshairWindow::OnWindowPosChanged(HWND hwnd)
{
    if (hwnd != m_attachedWindowHwnd)
        return;

    BITMAP bm;
    ENSURE(m_crosshair.GetBitmap(&bm) != 0);

    CRect activeWindowRect;
    ::GetWindowRect(m_attachedWindowHwnd, activeWindowRect);

    const CRect currentRect(CPoint(activeWindowRect.CenterPoint().x - bm.bmWidth / 2,
                                   activeWindowRect.CenterPoint().y - bm.bmHeight / 2),
                            CSize(bm.bmWidth, bm.bmHeight));

    EXT_TRACE() << L"Window pos changed";

    SetWindowPos(
           nullptr,
           currentRect.left,
           currentRect.top,
           currentRect.Width(),
           currentRect.Height(),
           SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOOWNERZORDER | SWP_NOREDRAW);
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

    if (m_windowPosChangedHook)
    {
        UnhookWinEvent(m_windowPosChangedHook);
        m_windowPosChangedHook = nullptr;
    }
    g_crosshairWindow = nullptr;
}

} // namespace crosshair
