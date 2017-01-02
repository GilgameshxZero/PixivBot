#pragma once

#include "RainLibraries.h"

#define RAIN_IMAGECHANGE	WM_RAINAVAILABLE
#define RAIN_CLOSEIMGWND	WM_RAINAVAILABLE + 1 //post when window is to be closed; WPARAM is exit code