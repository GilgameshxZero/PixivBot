#pragma once

#include "ImgWndProcDef.h"
#include "SendHandlerDef.h"
#include "Settings.h"
#include "UnivParam.h"

#include "RainLibraries.h"

#include <iostream>

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