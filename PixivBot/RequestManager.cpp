#include "RequestManager.h"

namespace PixivBot
{
	namespace RequestManager
	{
		int cachethread;
		std::mutex mcthread;

		void IncCacheThread ()
		{
			RequestManager::mcthread.lock ();
			RequestManager::cachethread++;

			Rain::RainCout << "increased request threads to " << RequestManager::cachethread << std::endl;
			
			if (Settings::max_req_threads == 0 || RequestManager::cachethread != Settings::max_req_threads)
				RequestManager::mcthread.unlock ();
		}
		void DecCacheThread ()
		{
			RequestManager::cachethread--;

			Rain::RainCout << "decreased request threads to " << RequestManager::cachethread << std::endl;

			if (!Settings::max_req_threads == 0 && RequestManager::cachethread == Settings::max_req_threads - 1)
				RequestManager::mcthread.unlock ();
		}
	}
}