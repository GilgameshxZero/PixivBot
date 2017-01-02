#pragma once

#include "Settings.h"

#include "RainLibraries.h"

#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_set>
#include <vector>

namespace PixivBot
{
	class UnivParam
	{
		public:
			Rain::RainWindow *imagewnd;
			std::queue< std::pair<int, std::vector<std::string> *> > *bfsq;
			std::string lastload;
			std::unordered_set<int> *processed, *inqueue, *awaiting;
			int *cachethread;
			bool *imgwndready; //image window has finished drawing and is ready for commands?
			std::vector<int> *tmpbsfq;

			std::mutex munivparam,
				mcthread;
	};

	void IncCacheThread (UnivParam *uparam);
	void DecCacheThread (UnivParam *uparam);
}