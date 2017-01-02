#pragma once

#define OEMRESOURCE

#include "ImgWndProc.h"
#include "MarkRequestStoreImage.h"
#include "SendHandler.h"
#include "Settings.h"
#include "UnivParam.h"
#include "resource.h"

#include "RainLibraries.h"

#include <crtdbg.h>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace PixivBot
{
	namespace Start
	{
		extern struct addrinfo *p_saddrinfo_www; //address for www.pixiv.net, not i1-i4

		int Start ();
	}

	DWORD WINAPI CacheImages (LPVOID param);
}