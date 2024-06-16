#include <pch.h>
#include <afxtoolbarimages.h>

#include "resource.h"

#include "Crosshairs.h"

#include <Controls/Utils/WindowClassRegistration.h>

#include <ext/core/check.h>
#include <ext/std/filesystem.h>

namespace process_toolkit::crosshair {

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
        // we have 3 sizes for every crosshair type
        auto crosshairResourceId = kFirstResourceId + 3 * (int)crosshair.type + (int)crosshair.size;

        CPngImage pngImage;
        EXT_CHECK(pngImage.Load(crosshairResourceId, AfxGetResourceHandle()))
            << "Failed to load image with type " << (int)crosshair.type << " and size " << (int)crosshair.size;
        EXT_CHECK(bitmap.Attach(pngImage.Detach())) << "Failed to load resource";
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

AttachableCrosshairWindow* g_crosshairWindow = nullptr;

void CALLBACK WinPosChangedProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
    g_crosshairWindow->OnWindowPosChanged(hwnd);
}

BEGIN_MESSAGE_MAP(TransparentWindowWithBitmap, CWnd)
    ON_WM_PAINT()
END_MESSAGE_MAP()

TransparentWindowWithBitmap::TransparentWindowWithBitmap()
    : m_className(typeid(*this).name())
{
    HINSTANCE instance = AfxGetInstanceHandle();

    WNDCLASSEX wndClass;
    if (!::GetClassInfoEx(instance, m_className, &wndClass))
    {
        memset(&wndClass, 0, sizeof(WNDCLASSEX));
        wndClass.cbSize = sizeof(WNDCLASSEX);
        wndClass.style = 0;
        wndClass.lpfnWndProc = ::DefMDIChildProc;
        wndClass.hInstance = instance;
        wndClass.lpszClassName = m_className;

        static WindowClassRegistrationLock registrator(wndClass);
    }
}

TransparentWindowWithBitmap::~TransparentWindowWithBitmap()
{
    if (::IsWindow(m_hWnd))
        DestroyWindow();
}

void TransparentWindowWithBitmap::Init(CBitmap&& bitmap, UINT exFlags)
{
    m_bitmap.DeleteObject();
    m_bitmap.Attach(bitmap.Detach());

    BITMAP bm;
    ENSURE(m_bitmap.GetBitmap(&bm) != 0);

    if (!IsWindow(*this))
    {
        CreateEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED | exFlags,
            m_className, NULL, WS_POPUP, CRect(0, 0, bm.bmWidth, bm.bmHeight), NULL, NULL);
    }

    // Create a compatible DC and select the bitmap into it
    CDC memDC;
    memDC.CreateCompatibleDC(NULL);
    CBitmap* pOldBitmap = memDC.SelectObject(&m_bitmap);

    // Create a BLENDFUNCTION structure for alpha blending
    BLENDFUNCTION blend = { 0 };
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 255; // Use 255 for per-pixel alpha
    blend.AlphaFormat = AC_SRC_ALPHA;

    POINT pptDst = {}, pptSrc = {};
    SIZE size = { bm.bmWidth, bm.bmHeight };

    CPaintDC dc(this);
    UpdateLayeredWindow(&dc, &pptDst, &size, &memDC, &pptSrc, RGB(0, 0, 0), &blend, ULW_ALPHA);

    memDC.SelectObject(pOldBitmap);
}

AttachableCrosshairWindow::AttachableCrosshairWindow()
{
    EXT_ASSERT(!g_crosshairWindow) << "Attachable crosshair window should be only 1 on the whole app";
    g_crosshairWindow = this;
}

AttachableCrosshairWindow::~AttachableCrosshairWindow()
{
    RemoveCrosshairWindow();
}

void AttachableCrosshairWindow::AttachCrosshairToWindow(const Settings& settings, HWND hWndOfActiveWindow)
{
    CBitmap crosshair;
    try
    {
        LoadCrosshair(settings, crosshair);
    }
    catch (...)
    {
        MessageBox(ext::ManageExceptionText(L"").c_str(), L"Failed to load crosshair", MB_ICONERROR);
        return;
    }

    TransparentWindowWithBitmap::Init(std::move(crosshair), WS_EX_TRANSPARENT);

    m_attachedWindowHwnd = hWndOfActiveWindow;
    OnWindowPosChanged(m_attachedWindowHwnd);

    DWORD pid = 0;
    GetWindowThreadProcessId(hWndOfActiveWindow, &pid);
    m_windowPosChangedHook = SetWinEventHook(EVENT_OBJECT_LOCATIONCHANGE, EVENT_OBJECT_LOCATIONCHANGE, NULL, WinPosChangedProc, pid, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
}

void AttachableCrosshairWindow::RemoveCrosshairWindow()
{
    m_attachedWindowHwnd = nullptr;
    if (m_windowPosChangedHook)
    {
        UnhookWinEvent(m_windowPosChangedHook);
        m_windowPosChangedHook = nullptr;
    }

    if (IsWindow(*this))
        ShowWindow(SW_HIDE);
}

void AttachableCrosshairWindow::OnWindowPosChanged(HWND hwnd)
{
    if (hwnd != m_attachedWindowHwnd)
        return;

    CRect rect;
    TransparentWindowWithBitmap::GetWindowRect(rect);

    CRect activeWindowRect;
    ::GetWindowRect(m_attachedWindowHwnd, activeWindowRect);

    CPoint topLeft = activeWindowRect.CenterPoint();
    topLeft.Offset(-rect.Width() / 2, -rect.Height() / 2);

    SetWindowPos(
           nullptr,
           topLeft.x,
           topLeft.y,
           0,
           0,
           SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOREDRAW);
}

BEGIN_MESSAGE_MAP(CursorReplacingWindow, TransparentWindowWithBitmap)
    ON_WM_SETCURSOR()
END_MESSAGE_MAP()

void CursorReplacingWindow::Create(CBitmap&& cursorImage)
{
    TransparentWindowWithBitmap::Init(std::move(cursorImage));
}

CRect CursorReplacingWindow::GetWindowRect() const
{
    CRect rect;
    TransparentWindowWithBitmap::GetWindowRect(rect);
    return rect;
}

BOOL CursorReplacingWindow::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    // removing default cursor
    ::SetCursor(NULL);
    return TRUE;
}

} // namespace process_toolkit::crosshair
