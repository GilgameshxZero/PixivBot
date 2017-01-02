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
	}
}