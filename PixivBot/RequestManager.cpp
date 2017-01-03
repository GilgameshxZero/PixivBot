#include "RequestManager.h"

namespace PixivBot
{
	namespace RequestManager
	{
		int req_count;
		std::mutex m_req_count;
		std::queue<std::thread> req_threads;

		void IncReqCount ()
		{
			RequestManager::m_req_count.lock ();
			RequestManager::req_count++;

			Rain::RainCout << "increased request threads to " << RequestManager::req_count << std::endl;
			
			if (Settings::max_req_threads == 0 || RequestManager::req_count != Settings::max_req_threads)
				RequestManager::m_req_count.unlock ();
		}

		void DecReqCount ()
		{
			RequestManager::req_count--;

			Rain::RainCout << "decreased request threads to " << RequestManager::req_count << std::endl;

			if (!Settings::max_req_threads == 0 && RequestManager::req_count == Settings::max_req_threads - 1)
				RequestManager::m_req_count.unlock ();
		}

		void BlockForThreads ()
		{
			RequestManager::m_req_count.lock ();
			RequestManager::m_req_count.unlock ();
		}

		int StoreFullReqMessage (void *param)
		{
			std::vector<void *> &funcparam = *reinterpret_cast<std::vector<void *> *>(param);
			Rain::WSARecvParam &recvparam = *reinterpret_cast<Rain::WSARecvParam *>(funcparam[0]);
			std::string &full_message = *reinterpret_cast<std::string *>(funcparam[1]);
			full_message += *recvparam.message;
			return 0; //continue receiving
		}

		void JoinRemoveThreads ()
		{
			while (!req_threads.empty ())
			{
				req_threads.front ().join ();
				req_threads.pop ();
			}
		}
		void ThreadCacheSubmission (int code)
		{
			RequestManager::req_threads.push (std::thread (CacheSubmission, code));
		}
		void ThreadCacheRecommendations (int code)
		{
			RequestManager::req_threads.push (std::thread (CacheRecommendations, code));
		}

		void CacheSubmission (int code)
		{
			ImageManager::in_img_queue.insert (code);

			SOCKET socket;
			std::string full_message;
			int recv_return;

			if (Rain::CreateClientSocket (&Start::p_saddrinfo_www, socket) || Rain::ConnToServ (&Start::p_saddrinfo_www, socket))
				return;

			RequestManager::IncReqCount ();
			Rain::RainCout << "sending request" << std::endl;
			Rain::SendText (socket, "GET /member_illust.php?mode=medium&illust_id=" + Rain::TToStr (code) + " HTTP/1.1\nHost: www.pixiv.net\n" + Settings::http_req_header[Settings::safe_mode][0] + "\n");
			recv_return = Rain::RecvUntilTimeout (socket, full_message);
			closesocket (socket);
			RequestManager::DecReqCount ();

			//process message
			if (recv_return != 0 || full_message.length () == 0)
			{
				Rain::RainCout << "MarkRequestStoreImage request returned empty, retrying" << std::endl;
				RequestManager::ThreadCacheSubmission (code);
			}
			else
			{
				if ((Settings::safe_mode && full_message.find ("Access by users under age 18 is restricted.") != std::string::npos) || //check safety because recommender doesn't do that
					(full_message.find ("Artist has made their work private.") != std::string::npos) ||  //check for private works
					(full_message.find ("ugokuIllustData") != std::string::npos) || //check for gif works
					(full_message.find ("This work was deleted.") != std::string::npos)) //deleted work
				{
					if (full_message.find ("ugokuIllustData") != std::string::npos)
						Rain::RainCout << "MANUAL CONFIMATION (VIDEO): " << code << std::endl;

					ImageManager::in_img_queue.erase (code);
					ImageManager::img_processed.insert (code);
					ImageManager::img_requesting.erase (code);

					PostMessage (ImageWnd::image_wnd.hwnd, RAIN_IMAGECHANGE, 0, 0);
				}
				else
				{
					//parse fmess for original image(s)
					std::size_t orig = full_message.find ("_illust_modal _hidden ui-modal-close-box");

					if (orig == std::string::npos)
					{
						if (full_message.find ("mode=manga") != std::string::npos) //is a manga/multiple image submission
							RequestManager::req_threads.push (std::thread (CacheMultipleSubmission, code));
						else //we have a problem
						{
							Rain::RainCout << "MANUAL CONFIMATION (???): " << code << std::endl;
							ImageManager::in_img_queue.erase (code);
							ImageManager::img_processed.insert (code);
							ImageManager::img_requesting.erase (code);

							PostMessage (ImageWnd::image_wnd.hwnd, RAIN_IMAGECHANGE, 0, 0);
						}
					}
					else //single image submission
					{
						//create vector in bsfq for image name storage
						ImageManager::img_queue.push (std::make_pair (code, new std::vector<std::string> ()));

						orig = full_message.find ("data-src=\"", orig) + 10;
						RequestManager::req_threads.push (std::thread (CacheOriginalImage, full_message.substr (orig, full_message.find ("\"", orig) - orig), "http://www.pixiv.net/member_illust.php?mode=medium&illust_id=" + Rain::TToStr (code), ImageManager::img_queue.back ().second));
					}
				}
			}
		}

		void CacheMultipleSubmission (int code)
		{
			SOCKET socket;
			std::string full_message;
			int recv_return;

			if (Rain::CreateClientSocket (&Start::p_saddrinfo_www, socket) || Rain::ConnToServ (&Start::p_saddrinfo_www, socket))
				return;

			RequestManager::IncReqCount ();
			Rain::RainCout << "sending request" << std::endl;
			Rain::SendText (socket, "GET /member_illust.php?mode=manga&illust_id=" + Rain::TToStr (code) + " HTTP/1.1\nHost: www.pixiv.net\n" + Settings::http_req_header[Settings::safe_mode][0] + "\n");
			recv_return = Rain::RecvUntilTimeout (socket, full_message);
			closesocket (socket);
			RequestManager::DecReqCount ();

			if (recv_return != 0 || full_message.length () == 0)
			{
				Rain::RainCout << "MarkParseMangaSubmission request returned empty, retrying" << std::endl;
				RequestManager::req_threads.push (std::thread (CacheMultipleSubmission, code));
			}
			else
			{
				//see how many images in this submission, and start MarkParseMangaBigImage for each
				std::size_t search = 0;
				int images = 0;

				search = full_message.find ("full-size-container _ui-tooltip");
				while (search != std::string::npos)
				{
					images++;
					search = full_message.find ("full-size-container _ui-tooltip", search + 1);
				}

				//create bfsq vector
				ImageManager::img_queue.push (std::make_pair (code, new std::vector<std::string> ()));

				for (int a = 0; a < images; a++)
					RequestManager::req_threads.push (std::thread (CacheMangaBigImage, code, a, ImageManager::img_queue.back ().second));
			}
		}

		void CacheMangaBigImage (int code, int index, std::vector<std::string> *namevec)
		{
			SOCKET socket;
			std::string full_message;
			int recv_return;

			if (Rain::CreateClientSocket (&Start::p_saddrinfo_www, socket) || Rain::ConnToServ (&Start::p_saddrinfo_www, socket))
				return;

			RequestManager::IncReqCount ();
			Rain::RainCout << "sending request" << std::endl;
			Rain::SendText (socket, "GET /member_illust.php?mode=manga_big&illust_id=" + Rain::TToStr (code) + "&page=" + Rain::TToStr (index) + " HTTP/1.1\nHost: www.pixiv.net\n" + Settings::http_req_header[Settings::safe_mode][0] + "\n");
			recv_return = Rain::RecvUntilTimeout (socket, full_message);
			closesocket (socket);
			RequestManager::DecReqCount ();

			if (recv_return != 0 || full_message.length () == 0)
			{
				Rain::RainCout << "MarkParseMangaBigImage request returned empty, retrying" << std::endl;
				RequestManager::req_threads.push (std::thread (CacheMangaBigImage, code, index, namevec));
			}
			else
			{
				//get the original image link from message, and start MarkDownloadSingleImage for it
				std::size_t search = 0;
				int images = 0;

				search = full_message.find ("img src=\"") + 9;

				RequestManager::req_threads.push (std::thread (CacheOriginalImage, full_message.substr (search, full_message.find ("\"", search) - search), "http://www.pixiv.net/member_illust.php?mode=manga_big&illust_id=" + Rain::TToStr (code) + "&page=" + Rain::TToStr (index), namevec));
			}
		}

		void CacheOriginalImage (std::string link, std::string referer, std::vector<std::string> *namevec)
		{
			std::string rellink, host, imagename;
			std::size_t domainstart = link.find ("//") + 2;
			std::size_t relstart = link.find ("/", domainstart);
			std::size_t namestart = link.find_last_of ("/") + 1;

			host = link.substr (domainstart, relstart - domainstart);
			rellink = link.substr (relstart, link.length () - relstart);
			imagename = link.substr (namestart, link.size () - namestart);

			if (Rain::FileExists (Settings::cache_dir + imagename))
			{
				Rain::RainCout << "discovered " << imagename << " in cache, skipping" << std::endl;
				namevec->push_back (imagename);

				if (ImageWnd::image_wnd.hwnd != NULL)
					PostMessage (ImageWnd::image_wnd.hwnd, RAIN_IMAGECHANGE, 0, 0);
			}
			else
			{
				SOCKET socket;
				std::string full_message;
				int recv_return;
				struct addrinfo *saddr;

				if (Rain::GetClientAddr (host, "80", &saddr) || 
					Rain::CreateClientSocket (&saddr, socket) || Rain::ConnToServ (&saddr, socket))
					return;
				freeaddrinfo (saddr);

				RequestManager::IncReqCount ();
				Rain::RainCout << "sending request" << std::endl;
				Rain::SendText (socket, "GET " + rellink + " HTTP/1.1\nHost: " + host + "\nReferer: " + referer + "\n" + Settings::http_req_header[Settings::safe_mode][2] + "\n");
				recv_return = Rain::RecvUntilTimeout (socket, full_message);
				closesocket (socket);
				RequestManager::DecReqCount ();

				if (recv_return != 0 ||full_message.length () == 0)
				{
					Rain::RainCout << "MarkDownloadSingleImage request returned empty, retrying" << std::endl;
					RequestManager::req_threads.push (std::thread (CacheOriginalImage, link, referer, namevec));
				}
				else
				{
					//cut from the first \n\n to the end of the string; save it in a file called mrsiparam->imagename, and put that imagename in mrsiparam->loc
					std::size_t imagestart = full_message.find ("\r\n\r\n") + 4;
					Rain::FastOutputFile (Settings::cache_dir + imagename, full_message.substr (imagestart, full_message.length () - imagestart));
					namevec->push_back (imagename);

					Rain::RainCout << "cached " << imagename << std::endl;

					//update the image window with the newly loaded image
					if (ImageWnd::image_wnd.hwnd != NULL)
						PostMessage (ImageWnd::image_wnd.hwnd, RAIN_IMAGECHANGE, 0, 0);
				}
			}
		}

		void CacheRecommendations (int code)
		{
			SOCKET socket;
			std::string full_message;
			int recv_return;

			if (Rain::CreateClientSocket (&Start::p_saddrinfo_www, socket) || Rain::ConnToServ (&Start::p_saddrinfo_www, socket))
				return;

			RequestManager::IncReqCount ();
			Rain::RainCout << "sending request" << std::endl;
			Rain::SendText (socket, "GET /rpc/recommender.php?type=illust&sample_illusts=" + Rain::TToStr (code) + "&num_recommendations=" + Rain::TToStr (Settings::recs_on_accept) + "&tt=" + Settings::pixiv_tt[Settings::safe_mode] + " HTTP/1.1\nHost: www.pixiv.net\nReferer: http://www.pixiv.net/member_illust.php?mode=medium&illust_id=" + Rain::TToStr (code) + "\n" + Settings::http_req_header[Settings::safe_mode][1] + "\n");
			recv_return = Rain::RecvUntilTimeout (socket, full_message);
			closesocket (socket);
			RequestManager::DecReqCount ();

			if (recv_return != 0 || full_message.length () == 0)
			{
				Rain::RainCout << "MarkRequestStoreRecommendations request returned empty, retrying" << std::endl;
				RequestManager::req_threads.push (std::thread (CacheRecommendations, code));
			}
			else
			{
				//for each recommendation, check if its img_processed or inqueue; if not, markrequeststoreimage it
				std::size_t recstart = full_message.find ("\":[") + 3;
				std::vector<int> recs (Settings::recs_on_accept, 0);
				int rec_counter = 0;
				for (std::size_t a = recstart; rec_counter < Settings::recs_on_accept; a++)
				{
					if (full_message[a] >= '0' && full_message[a] <= '9')
						recs[rec_counter] = recs[rec_counter] * 10 + full_message[a] - '0';
					else
						rec_counter++;
				}

				for (std::size_t a = 0; a < recs.size (); a++)
					if (ImageManager::img_processed.find (recs[a]) == ImageManager::img_processed.end () &&
						ImageManager::in_img_queue.find (recs[a]) == ImageManager::in_img_queue.end ())
						ImageManager::img_requesting.insert (recs[a]);

				if (ImageWnd::image_wnd.hwnd != NULL)
					SendMessage (ImageWnd::image_wnd.hwnd, RAIN_IMAGECHANGE, 0, 0);

				for (std::size_t a = 0; a < recs.size (); a++)
					if (ImageManager::img_processed.find (recs[a]) == ImageManager::img_processed.end () &&
						ImageManager::in_img_queue.find (recs[a]) == ImageManager::in_img_queue.end ())
						RequestManager::ThreadCacheSubmission (recs[a]);
			}
		}
	}
}