#pragma once

#define OEMRESOURCE

#include "..\RainLibrary3\RainLibraries.h"

#include "ImageWnd.h"
#include "Settings.h"
#include "resource.h"

#include <crtdbg.h>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace Monochrome8 {
	namespace PixivBot {
		extern struct addrinfo *p_saddrinfo_www; //address for www.pixiv.net, not i1-i4

		int start();
	}
}