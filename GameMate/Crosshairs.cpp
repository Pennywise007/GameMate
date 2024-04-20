#include <pch.h>
#include "Crosshairs.h"

#include <ext/core/check.h>

namespace crosshair {

CSize GetBitmapSize(const Size& size)
{
    switch (size)
    {
    case Size::eSmall:
        return CSize(8, 8);
    default:
        EXT_ASSERT(false) << "Unknown size " << (int)size;
        [[fallthrough]];
    case Size::eMedium:
        return CSize(16, 16);
    case Size::eLarge:
        return CSize(32, 32);
    }
}

void LoadCrosshair(const Settings& crosshair, CBitmap& bitmap) EXT_THROWS(std::runtime_error)
{
    Size size = crosshair.size;
}


} // namespace crosshair
