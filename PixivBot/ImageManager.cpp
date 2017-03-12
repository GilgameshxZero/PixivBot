#include "ImageManager.h"

namespace PixivBot
{
	namespace ImageManager
	{
		std::queue<std::pair<int, std::vector<std::string> *> > img_queue;
		std::unordered_set<int> img_processed, in_img_queue, img_requesting;

		void QueueRemCurImg ()
		{
			ImageManager::img_queue.front ().second->erase (ImageManager::img_queue.front ().second->begin ());
			if (ImageManager::img_queue.front ().second->size () == 0)
			{
				ImageManager::img_requesting.erase (ImageManager::img_queue.front ().first);
				ImageManager::in_img_queue.erase (ImageManager::img_queue.front ().first);
				ImageManager::img_processed.insert (ImageManager::img_queue.front ().first);
				delete ImageManager::img_queue.front ().second;
				ImageManager::img_queue.pop ();
			}
		}

		DWORD WINAPI CacheInitImages (LPVOID param)
		{
			std::vector<int> &img_queue_init = *reinterpret_cast<std::vector<int> *>(param);

			for (unsigned int a = 0; a < img_queue_init.size (); a++)
			{
				//cache images
				if (ImageManager::img_processed.find (img_queue_init[a]) == ImageManager::img_processed.end () && ImageManager::in_img_queue.find (img_queue_init[a]) == ImageManager::in_img_queue.end ())
				{
					ImageManager::in_img_queue.insert (img_queue_init[a]);
					RequestManager::ThreadCacheSubmission (img_queue_init[a]);
				}
				else
					ImageManager::img_requesting.erase (img_queue_init[a]);

				SendMessage (ImageWnd::image_wnd.hwnd, RAIN_IMAGECHANGE, 0, 0);
			}

			return 0;
		}
	}
}