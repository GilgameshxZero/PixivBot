#pragma once

#include "Settings.h"

#include "RainLibraries.h"

#include <mutex>

namespace PixivBot
{
	namespace RequestManager
	{
		extern int req_thread_count;
		extern std::mutex m_req_thread;

		void IncReqThread ();
		void DecReqThread ();
		void BlockForThreads ();

		//should be used as the message handler for requests; param must be a std::vector<void *> * type, with recvparam as the first element, and a std::string * as the second element, to store the full message
		int StoreFullReqMessage (void *param);
	}
}