#pragma once

#include <afxwin.h>

#include <ext/error/exception.h>

#include "Settings.h"

namespace crosshair {

void LoadCrosshair(const Settings& crosshair, CBitmap& bitmap) EXT_THROWS(std::runtime_error);
void ChangeCrosshairColor(CBitmap& bitmap, COLORREF color);
void ResizeCrosshair(CBitmap& bitmap, const CSize& size);

} // namespace crosshair
