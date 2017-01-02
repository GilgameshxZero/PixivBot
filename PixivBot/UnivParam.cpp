#include "UnivParam.h"

namespace PixivBot
{
	void IncCacheThread (UnivParam *uparam)
	{
		uparam->mcthread.lock ();
		uparam->munivparam.lock ();
		(*uparam->cachethread)++;

		Rain::RainCout << "increased request threads to " << *uparam->cachethread << std::endl;

		uparam->munivparam.unlock ();

		if (Settings::max_req_threads == 0 || (*uparam->cachethread) != Settings::max_req_threads)
			uparam->mcthread.unlock ();
	}
	void DecCacheThread (UnivParam *uparam)
	{
		uparam->munivparam.lock ();
		(*uparam->cachethread)--;

		Rain::RainCout << "decreased request threads to " << *uparam->cachethread << std::endl;

		if (!Settings::max_req_threads == 0 && (*uparam->cachethread) == Settings::max_req_threads - 1)
			uparam->mcthread.unlock ();

		uparam->munivparam.unlock ();
	}
}