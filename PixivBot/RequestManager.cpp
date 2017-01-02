#include "RequestManager.h"

namespace PixivBot
{
	namespace RequestManager
	{
		int req_thread_count;
		std::mutex m_req_thread;

		void IncReqThread ()
		{
			RequestManager::m_req_thread.lock ();
			RequestManager::req_thread_count++;

			Rain::RainCout << "increased request threads to " << RequestManager::req_thread_count << std::endl;
			
			if (Settings::max_req_threads == 0 || RequestManager::req_thread_count != Settings::max_req_threads)
				RequestManager::m_req_thread.unlock ();
		}

		void DecReqThread ()
		{
			RequestManager::req_thread_count--;

			Rain::RainCout << "decreased request threads to " << RequestManager::req_thread_count << std::endl;

			if (!Settings::max_req_threads == 0 && RequestManager::req_thread_count == Settings::max_req_threads - 1)
				RequestManager::m_req_thread.unlock ();
		}

		void BlockForThreads ()
		{
			RequestManager::m_req_thread.lock ();
			RequestManager::m_req_thread.unlock ();
		}

		int StoreFullReqMessage (void *param)
		{
			std::vector<void *> &funcparam = *reinterpret_cast<std::vector<void *> *>(param);
			Rain::WSARecvParam &recvparam = *reinterpret_cast<Rain::WSARecvParam *>(funcparam[0]);
			std::string &full_message = *reinterpret_cast<std::string *>(funcparam[1]);
			full_message += *recvparam.message;
			return 0; //continue receiving
		}
	}
}