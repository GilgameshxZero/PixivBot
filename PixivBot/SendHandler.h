#pragma once

#include "ImgWndProcDef.h"
#include "MarkRequestStoreImage.h"
#include "SendHandlerDef.h"
#include "UnivParam.h"

#include "RainLibraries.h"

#include <iostream>

namespace PixivBot
{
	namespace SendHandler
	{
		LRESULT OnReject (HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
		LRESULT OnAccept (HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
	}
}