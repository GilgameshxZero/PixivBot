#include "SendHandler.h"

namespace PixivBot
{
	namespace SendHandler
	{
		void OnReject (UnivParam *uparam)
		{
			if (uparam->bfsq->size () == 0 || uparam->bfsq->front ().second->size () == 0) //images are still loading
				return;

			Rain::RainCout << "rejected " << uparam->bfsq->front ().second->front () << "\n";

			//delete cached image
			DeleteFile ((Settings::cache_dir + uparam->bfsq->front ().second->front ()).c_str ());

			//remove entry from queue
			uparam->munivparam.lock ();
			uparam->bfsq->front ().second->erase (uparam->bfsq->front().second->begin ());
			if (uparam->bfsq->front ().second->size () == 0)
			{
				uparam->awaiting->erase (uparam->bfsq->front ().first);
				uparam->inqueue->erase (uparam->bfsq->front ().first);
				uparam->processed->insert (uparam->bfsq->front ().first);
				delete uparam->bfsq->front ().second;
				uparam->bfsq->pop ();
			}
			uparam->munivparam.unlock ();

			//set new image
			SendMessage (uparam->imagewnd->hwnd, RAIN_IMAGECHANGE, 0, 0);
		}

		void OnAccept (UnivParam *uparam)
		{
			if (uparam->bfsq->size () == 0 || uparam->bfsq->front ().second->size () == 0) //images are still loading
				return;

			Rain::RainCout << "accepted " << uparam->bfsq->front ().second->front () << "\n";

			//move image to accepted folder
			MoveFile (((std::string)(Settings::cache_dir + uparam->bfsq->front ().second->front ())).c_str (), ((std::string)(Settings::accept_dir + uparam->bfsq->front ().second->front ())).c_str ());

			int code = uparam->bfsq->front ().first;

			//remove entry from queue
			uparam->munivparam.lock ();
			uparam->bfsq->front ().second->erase (uparam->bfsq->front ().second->begin ());
			if (uparam->bfsq->front ().second->size () == 0)
			{
				uparam->awaiting->erase (uparam->bfsq->front ().first);
				uparam->inqueue->erase (uparam->bfsq->front ().first);
				uparam->processed->insert (uparam->bfsq->front ().first);
				delete uparam->bfsq->front ().second;
				uparam->bfsq->pop ();
			}
			uparam->munivparam.unlock ();

			//don't send RAIN_IMAGECHANGE yet, before loading recs; recs function will do it

			//load recommended images into cache and queue, but do it asynchronously to make the process fast
			MRSIParam *mrsiparam = new MRSIParam ();
			mrsiparam->code = code;
			mrsiparam->uparam = uparam;
			Rain::SimpleCreateThread (MRSRThread, reinterpret_cast<LPVOID>(mrsiparam));
		}
	}
}