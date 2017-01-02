#pragma once

#include "Settings.h"

#include "RainLibraries.h"

#include <mutex>

namespace PixivBot
{
	namespace RequestManager
	{
		extern int cachethread;
		extern std::mutex mcthread;

		void IncCacheThread ();
		void DecCacheThread ();
	}
}