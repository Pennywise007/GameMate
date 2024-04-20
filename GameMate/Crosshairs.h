#pragma once

#include <afxwin.h>

#include <ext/error/exception.h>

#include "Settings.h"

namespace crosshair {

void LoadCrosshair(const Settings& crosshair, CBitmap& bitmap) EXT_THROWS(std::runtime_error);

} // namespace crosshair
