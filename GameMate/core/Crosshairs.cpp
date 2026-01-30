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
    BITMAP bm{};
    EXT_CHECK(bitmap.GetBitmap(&bm)) << "Failed to get Bitmap";

    const int w = bm.bmWidth;
    const int h = bm.bmHeight;

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    std::vector<BYTE> pixels(w * h * 4);

    // Read bitmap
    {
        CDC memDC;
        memDC.CreateCompatibleDC(nullptr);
        CBitmap* old = memDC.SelectObject(&bitmap);

        if (!GetDIBits(memDC, bitmap, 0, h, pixels.data(), &bmi, DIB_RGB_COLORS))
        {
            memDC.SelectObject(old);
            EXT_TRACE_ERR() << L"GetDIBits failed, err=" << GetLastError();
            return;
        }
        memDC.SelectObject(old);
    }

    const BYTE r = GetRValue(color);
    const BYTE g = GetGValue(color);
    const BYTE b = GetBValue(color);

    for (size_t i = 0; i < pixels.size(); i += 4)
    {
        BYTE a = pixels[i + 3];
        if (a == 0)
            continue;

        pixels[i + 0] = (b * a + 127) / 255;
        pixels[i + 1] = (g * a + 127) / 255;
        pixels[i + 2] = (r * a + 127) / 255;
    }

    // Create new bitmap
    void* bits = nullptr;
    HBITMAP hNew = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!hNew)
    {
        EXT_TRACE_ERR() << L"CreateDIBSection failed, err=" << GetLastError();
        return;
    }

    memcpy(bits, pixels.data(), pixels.size());

    bitmap.DeleteObject();
    bitmap.Attach(hNew);
}

void ResizeCrosshair(CBitmap& bitmap, const CSize& size)
{
    // Scale bitmap using nearest-neighbor on 32-bit BGRA buffer (preserves alpha)
    CBitmap srcBitmap;
    srcBitmap.Attach(bitmap.Detach());

    BITMAP bm = { 0 };
    srcBitmap.GetBitmap(&bm);
    const int srcW = bm.bmWidth;
    const int srcH = bm.bmHeight;
    const int dstW = size.cx;
    const int dstH = size.cy;
    if (srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0)
        return;

    // Read source into 32-bit top-down buffer
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = srcW;
    bmi.bmiHeader.biHeight = -srcH; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    std::vector<BYTE> srcPixels(srcW * srcH * 4);
    CDC memDC;
    memDC.CreateCompatibleDC(nullptr);
    CBitmap* old = memDC.SelectObject(&srcBitmap);

    if (!GetDIBits(memDC, srcBitmap, 0, srcH, srcPixels.data(), &bmi, DIB_RGB_COLORS))
    {
        memDC.SelectObject(old);
        EXT_TRACE_ERR() << L"GetDIBits failed, err=" << GetLastError();
        return;
    }
    memDC.SelectObject(old);

    // Destination buffer
    std::vector<BYTE> dstPixels(dstW * dstH * 4);

    for (int y = 0; y < dstH; ++y)
    {
        int srcY = (int)((y * (long long)srcH) / dstH);
        if (srcY >= srcH)
            srcY = srcH - 1;
        for (int x = 0; x < dstW; ++x)
        {
            int srcX = (int)((x * (long long)srcW) / dstW);
            if (srcX >= srcW) srcX = srcW - 1;
            const BYTE* s = &srcPixels[(srcY * srcW + srcX) * 4];
            BYTE* d = &dstPixels[(y * dstW + x) * 4];
            // Copy BGRA; ensure premultiplied alpha correctness by recomputing premultiplied channels
            BYTE a = s[3];
            d[3] = a;
            if (a == 0)
            {
                d[0] = d[1] = d[2] = 0;
            }
            else
            {
                // Source might not be premultiplied; compute un-premultiplied RGB by scaling if needed
                // We'll assume s stores premultiplied (or near enough) and just copy
                d[0] = s[0];
                d[1] = s[1];
                d[2] = s[2];
            }
        }
    }

    // Create DIB for destination
    BITMAPINFO dbmi = {};
    dbmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    dbmi.bmiHeader.biWidth = dstW;
    dbmi.bmiHeader.biHeight = -dstH;
    dbmi.bmiHeader.biPlanes = 1;
    dbmi.bmiHeader.biBitCount = 32;
    dbmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP hNew = CreateDIBSection(nullptr, &dbmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!hNew)
    {
        EXT_TRACE_ERR() << L"CreateDIBSection failed, err=" << GetLastError();
        return;
    }

    memcpy(bits, dstPixels.data(), dstPixels.size());

    bitmap.DeleteObject();
    bitmap.Attach(hNew);
}

AttachableCrosshairWindow* g_crosshairWindow = nullptr;

void CALLBACK WinPosChangedProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
    if (idObject != OBJID_WINDOW)
        return;

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

        const static WindowClassRegistrationLock registrator(wndClass);
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

    CClientDC dc(this);
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
