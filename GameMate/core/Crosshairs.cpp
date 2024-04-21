#include <pch.h>
#include <afxtoolbarimages.h>

#include "resource.h"

#include "Crosshairs.h"

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

} // namespace crosshair
