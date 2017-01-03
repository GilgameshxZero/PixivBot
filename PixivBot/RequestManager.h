#pragma once

#include "Settings.h"
#include "Start.h"

#include "RainLibraries.h"

#include <mutex>
#include <queue>
#include <thread>

namespace PixivBot
{
	namespace RequestManager
	{
		extern int req_count;
		extern std::mutex m_req_count;

		void IncReqCount ();
		void DecReqCount ();
		void BlockForThreads ();

		//used as the message handler for requests; param must be a std::vector<void *> * type, with recvparam as the first element, and a std::string * as the second element, to store the full message
		int StoreFullReqMessage (void *param);

		void JoinRemoveThreads ();
		void ThreadCacheSubmission (int code);
		void ThreadCacheRecommendations (int code);

		//request for any submissions
		void CacheSubmission (int code);
		//request for multiple image submissions
		void CacheMultipleSubmission (int code);
		//request for the mode=manga_big image of the multiple image submissions
		void CacheMangaBigImage (int code, int index, std::vector<std::string> *namevec);
		//request for single original image
		void CacheOriginalImage (std::string link, std::string referer, std::vector<std::string> *namevec);
		//cache appropriate recommendations to image `code`
		void CacheRecommendations (int code);
	}
}