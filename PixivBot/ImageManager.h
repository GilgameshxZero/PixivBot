#pragma once

#include "ImageWnd.h"

#include "RainWindowsLAM.h"

#include <queue>
#include <string>
#include <unordered_set>
#include <vector>

namespace PixivBot
{
	namespace ImageManager
	{
		extern std::queue<std::pair<int, std::vector<std::string> *> > img_queue; //(image code, image filepaths vector pointer); all images in the queue are cached; recommendations are added to the queue and cached if the image is accepted by the user
		extern std::unordered_set<int> img_processed, in_img_queue, img_requesting;

		void QueueRemCurImg ();
		DWORD WINAPI CacheInitImages (LPVOID param);
	}
}