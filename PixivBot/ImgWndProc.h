#pragma once

#include "SendHandler.h"
#include "Settings.h"
#include "UnivParam.h"

#include "RainLibraries.h"

#include <iostream>

#define RAIN_IMAGECHANGE	WM_RAINAVAILABLE
#define RAIN_CLOSEIMGWND	WM_RAINAVAILABLE + 1 //post when window is to be closed; WPARAM is exit code

namespace PixivBot
{
	namespace ImgWndProc
	{
		LRESULT OnClose (HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
		LRESULT OnKeyDown (HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
		LRESULT OnKeyUp (HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
		LRESULT OnPaint (HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
		LRESULT OnSize (HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

		//curimgfile in uparam is updated
		LRESULT OnImageChange (HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
	}
}