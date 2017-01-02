#include "MarkRequestStoreImage.h"

namespace PixivBot
{
	int MarkRequestStoreImage (int code)
	{
		Rain::WSARecvParam *recvparam = new Rain::WSARecvParam ();

		recvparam->funcparam = Rain::PackPointers (recvparam, new std::string (), new int (code));
		recvparam->OnRecvEnd = OnMRSIExit;

		recvparam->OnProcessMessage = RequestManager::StoreFullReqMessage;

		recvparam->buflen = 1024;
		recvparam->message = new std::string ();
		recvparam->sock = new SOCKET ();
		recvparam->OnRecvInit = OnMRSIInit;

		//add to the queue here so that we don't send unnecessary requests; queue checking is not done here
		ImageManager::in_img_queue.insert (code);

		if (Rain::CreateClientSocket (&Start::p_saddrinfo_www, *recvparam->sock))
			return -1;
		if (Rain::ConnToServ (&Start::p_saddrinfo_www, *recvparam->sock))
			return -2;

		Rain::CreateRecvThread (recvparam);

		RequestManager::BlockForThreads ();
		Rain::RainCout << "sending request" << std::endl;
		Rain::SendText (*recvparam->sock, "GET /member_illust.php?mode=medium&illust_id=" + Rain::TToStr (code) + " HTTP/1.1\nHost: www.pixiv.net\n" + Settings::http_req_header[Settings::safe_mode][0] + "\n");

		return 0;
	}
	int OnMRSIMessage (void *param)
	{
		MRSIParam *mrsiparam = reinterpret_cast<MRSIParam *>(param);
		*(mrsiparam->fmess) += *(mrsiparam->recvparam->message);
		return 0; //continue receiving
	}
	void OnMRSIExit (void *param)
	{
		std::vector<void *> &funcparam = *reinterpret_cast<std::vector<void *> *>(param);
		Rain::WSARecvParam &recvparam = *reinterpret_cast<Rain::WSARecvParam *>(funcparam[0]);
		std::string &full_message = *reinterpret_cast<std::string *>(funcparam[1]);
		int &code = *reinterpret_cast<int *>(funcparam[2]);

		RequestManager::DecReqThread ();

		if (full_message.length () == 0)
		{
			Rain::RainCout << "MarkRequestStoreImage request returned empty, retrying" << std::endl;
			MarkRequestStoreImage (code);
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
						MarkParseMangaSubmission (code);
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
					MarkDownloadSingleImage (full_message.substr (orig, full_message.find ("\"", orig) - orig), "http://www.pixiv.net/member_illust.php?mode=medium&illust_id=" + Rain::TToStr (code), ImageManager::img_queue.back ().second);
				}
			}
		}

		closesocket (*recvparam.sock);
		delete recvparam.sock;
		delete recvparam.message;
		Rain::FreePtrVectorPtr (&funcparam);
	}

	int MarkParseMangaSubmission (int code)
	{
		Rain::WSARecvParam *recvparam = new Rain::WSARecvParam ();
		MRSIParam *mrsiparam = new MRSIParam ();
		mrsiparam->conn = new SOCKET ();

		if (Rain::CreateClientSocket (&Start::p_saddrinfo_www, *(mrsiparam->conn)))
			return -1;
		if (Rain::ConnToServ (&Start::p_saddrinfo_www, *(mrsiparam->conn)))
			return -2;

		std::string getreq;
		getreq = "GET /member_illust.php?mode=manga&illust_id=" + Rain::TToStr (code) + " HTTP/1.1\nHost: www.pixiv.net\n" + Settings::http_req_header[Settings::safe_mode][0] + "\n";

		mrsiparam->code = code;
		recvparam->OnRecvEnd = OnMPMSExit;

		PrepMRSIParams (mrsiparam, recvparam);

		Rain::SendText (*(mrsiparam->conn), getreq.c_str (), getreq.length ());
		return 0;
	}
	void OnMPMSExit (void *param)
	{
		MRSIParam *mrsiparam = reinterpret_cast<MRSIParam *>(param);
		std::string &fmess = *(mrsiparam->fmess);

		RequestManager::DecReqThread ();

		if (fmess.length () == 0)
		{
			Rain::RainCout << "MarkParseMangaSubmission request returned empty, retrying" << std::endl;
			MarkParseMangaSubmission (mrsiparam->code);
		}
		else
		{
			//see how many images in this submission, and start MarkParseMangaBigImage for each
			std::size_t search = 0;
			int images = 0;

			search = fmess.find ("full-size-container _ui-tooltip");
			while (search != std::string::npos)
			{
				images++;
				search = fmess.find ("full-size-container _ui-tooltip", search + 1);
			}

			//create bfsq vector
			ImageManager::img_queue.push (std::make_pair (mrsiparam->code, new std::vector<std::string> ()));

			for (int a = 0;a < images;a++)
				MarkParseMangaBigImage (mrsiparam->code, a, ImageManager::img_queue.back ().second);
		}

		FreeAndCloseMRSI (mrsiparam);
	}

	int MarkParseMangaBigImage (int code, int index, std::vector<std::string> *namevec)
	{
		Rain::WSARecvParam *recvparam = new Rain::WSARecvParam ();
		MRSIParam *mrsiparam = new MRSIParam ();
		mrsiparam->conn = new SOCKET ();

		if (Rain::CreateClientSocket (&Start::p_saddrinfo_www, *(mrsiparam->conn)))
			return -1;
		if (Rain::ConnToServ (&Start::p_saddrinfo_www, *(mrsiparam->conn)))
			return -2;

		std::string getreq;
		getreq = "GET /member_illust.php?mode=manga_big&illust_id=" + Rain::TToStr (code) + "&page=" + Rain::TToStr (index) + " HTTP/1.1\nHost: www.pixiv.net\n" + Settings::http_req_header[Settings::safe_mode][0] + "\n";
		
		mrsiparam->code = code;
		mrsiparam->index = index;
		mrsiparam->namevec = namevec;
		recvparam->OnRecvEnd = OnMPMBIExit;

		PrepMRSIParams (mrsiparam, recvparam);

		Rain::SendText (*(mrsiparam->conn), getreq.c_str (), getreq.length ());
		return 0;
	}
	void OnMPMBIExit (void *param)
	{
		MRSIParam *mrsiparam = reinterpret_cast<MRSIParam *>(param);
		std::string &fmess = *(mrsiparam->fmess);

		RequestManager::DecReqThread ();

		if (fmess.length () == 0)
		{
			Rain::RainCout << "MarkParseMangaBigImage request returned empty, retrying" << std::endl;
			MarkParseMangaBigImage (mrsiparam->code, mrsiparam->index, mrsiparam->namevec);
		}
		else
		{
			//get the original image link from message, and start MarkDownloadSingleImage for it
			std::size_t search = 0;
			int images = 0;

			search = fmess.find ("img src=\"") + 9;

			MarkDownloadSingleImage (fmess.substr (search, fmess.find ("\"", search) - search), "http://www.pixiv.net/member_illust.php?mode=manga_big&illust_id=" + Rain::TToStr (mrsiparam->code) + "&page=" + Rain::TToStr (mrsiparam->index), mrsiparam->namevec);
		}

		FreeAndCloseMRSI (mrsiparam);
	}

	int MarkDownloadSingleImage (std::string link, std::string referer, std::vector<std::string> *namevec)
	{
		Rain::WSARecvParam *recvparam = new Rain::WSARecvParam ();
		MRSIParam *mrsiparam = new MRSIParam ();
		mrsiparam->conn = new SOCKET ();

		std::string rellink, host;
		std::size_t domainstart = link.find ("//") + 2;
		std::size_t relstart = link.find ("/", domainstart);
		std::size_t namestart = link.find_last_of ("/") + 1;
		host = link.substr (domainstart, relstart - domainstart);
		rellink = link.substr (relstart, link.length () - relstart);
		mrsiparam->imagename = link.substr (namestart, link.size () - namestart);
		mrsiparam->namevec = namevec;
		mrsiparam->link = link;
		mrsiparam->referer = referer;
		recvparam->OnRecvEnd = OnMDSIExit;

		if (Rain::FileExists (Settings::cache_dir + mrsiparam->imagename))
		{
			Rain::RainCout << "discovered " << mrsiparam->imagename << " in cache, skipping" << std::endl;
			mrsiparam->namevec->push_back (mrsiparam->imagename);

			if (ImageWnd::image_wnd.hwnd != NULL)
				PostMessage (ImageWnd::image_wnd.hwnd, RAIN_IMAGECHANGE, 0, 0);

			delete mrsiparam->conn;
			delete mrsiparam;
			delete recvparam;
		}
		else
		{
			//get client address for this subdomain
			struct addrinfo *saddr;
			if (Rain::GetClientAddr (host, "80", &saddr))
				return -1;
			if (Rain::CreateClientSocket (&saddr, *(mrsiparam->conn)))
				return -2;
			if (Rain::ConnToServ (&saddr, *(mrsiparam->conn)))
				return -3;
			freeaddrinfo (saddr);

			std::string getreq;
			getreq = "GET " + rellink + " HTTP/1.1\nHost: " + host + "\nReferer: " + referer + "\n" + Settings::http_req_header[Settings::safe_mode][2] + "\n";

			PrepMRSIParams (mrsiparam, recvparam);

			Rain::SendText (*(mrsiparam->conn), getreq.c_str (), getreq.length ());
		}

		return 0;
	}
	void OnMDSIExit (void *param)
	{
		MRSIParam *mrsiparam = reinterpret_cast<MRSIParam *>(param);
		std::string &fmess = *(mrsiparam->fmess);

		RequestManager::DecReqThread ();

		if (fmess.length () == 0)
		{
			Rain::RainCout << "MarkDownloadSingleImage request returned empty, retrying" << std::endl;

			//resend MDSI request
			MarkDownloadSingleImage (mrsiparam->link, mrsiparam->referer, mrsiparam->namevec);
		}
		else
		{
			//cut from the first \n\n to the end of the string; save it in a file called mrsiparam->imagename, and put that imagename in mrsiparam->loc
			std::size_t imagestart = fmess.find ("\r\n\r\n") + 4;
			Rain::FastOutputFile (Settings::cache_dir + mrsiparam->imagename, fmess.substr (imagestart, fmess.length () - imagestart));
			mrsiparam->namevec->push_back (mrsiparam->imagename);

			Rain::RainCout << "cached " << mrsiparam->imagename << std::endl;

			//update the image window with the newly loaded image
			if (ImageWnd::image_wnd.hwnd != NULL)
				PostMessage (ImageWnd::image_wnd.hwnd, RAIN_IMAGECHANGE, 0, 0);
		}

		FreeAndCloseMRSI (mrsiparam);
	}

	int MarkRequestStoreRecommendations (int code)
	{
		Rain::WSARecvParam *recvparam = new Rain::WSARecvParam ();
		MRSIParam *mrsiparam = new MRSIParam ();
		mrsiparam->conn = new SOCKET ();

		if (Rain::CreateClientSocket (&Start::p_saddrinfo_www, *(mrsiparam->conn)))
			return -1;
		if (Rain::ConnToServ (&Start::p_saddrinfo_www, *(mrsiparam->conn)))
			return -2;

		std::string getreq;
		getreq = "GET /rpc/recommender.php?type=illust&sample_illusts=" + Rain::TToStr (code) + "&num_recommendations=" + Rain::TToStr (Settings::recs_on_accept) + "&tt=" + Settings::pixiv_tt[Settings::safe_mode] + " HTTP/1.1\nHost: www.pixiv.net\nReferer: http://www.pixiv.net/member_illust.php?mode=medium&illust_id=" + Rain::TToStr (code) + "\n" + Settings::http_req_header[Settings::safe_mode][1] + "\n";

		mrsiparam->code = code;
		recvparam->OnRecvEnd = OnMRSRExit;

		PrepMRSIParams (mrsiparam, recvparam);

		Rain::SendText (*(mrsiparam->conn), getreq.c_str (), getreq.length ());
		return 0;
	}
	void OnMRSRExit (void *param)
	{
		MRSIParam *mrsiparam = reinterpret_cast<MRSIParam *>(param);
		std::string &fmess = *(mrsiparam->fmess);

		RequestManager::DecReqThread ();

		if (fmess.length () == 0)
		{
			Rain::RainCout << "MarkRequestStoreRecommendations request returned empty, retrying" << std::endl;
			MarkRequestStoreRecommendations (mrsiparam->code);
		}
		else
		{
			//for each recommendation, check if its img_processed or in queue; if not, markrequeststoreimage it
			std::size_t recstart = fmess.find ("\":[") + 3;
			std::vector<int> recs (Settings::recs_on_accept, 0);
			int rec_counter = 0;
			for (std::size_t a = recstart; rec_counter < Settings::recs_on_accept;a++)
			{
				if (fmess[a] >= '0' && fmess[a] <= '9')
					recs[rec_counter] = recs[rec_counter] * 10 + fmess[a] - '0';
				else
					rec_counter++;
			}

			for (std::size_t a = 0;a < recs.size ();a++)
				if (ImageManager::img_processed.find (recs[a]) == ImageManager::img_processed.end () &&
					ImageManager::in_img_queue.find (recs[a]) == ImageManager::in_img_queue.end ())
						ImageManager::img_requesting.insert (recs[a]);

			if (ImageWnd::image_wnd.hwnd != NULL)
				SendMessage (ImageWnd::image_wnd.hwnd, RAIN_IMAGECHANGE, 0, 0);

			for (std::size_t a = 0;a < recs.size ();a++)
				if (ImageManager::img_processed.find (recs[a]) == ImageManager::img_processed.end () &&
					ImageManager::in_img_queue.find (recs[a]) == ImageManager::in_img_queue.end ())
					MarkRequestStoreImage (recs[a]);
		}

		FreeAndCloseMRSI (mrsiparam);
	}

	void OnMRSIInit (void *param)
	{
		MRSIParam *mrsiparam = reinterpret_cast<MRSIParam *>(param);

		RequestManager::IncReqThread ();
	}

	void PrepMRSIParams (MRSIParam *mrsiparam, Rain::WSARecvParam *recvparam)
	{
		mrsiparam->fmess = new std::string ();
		mrsiparam->recvparam = recvparam;

		recvparam->buflen = 1024;
		recvparam->funcparam = mrsiparam;
		recvparam->message = new std::string ();
		recvparam->OnProcessMessage = OnMRSIMessage; //recieve full message
		recvparam->sock = mrsiparam->conn;
		recvparam->OnRecvInit = OnMRSIInit;

		Rain::CreateRecvThread (recvparam);

		//prevents sending of requests until threads are open
		RequestManager::m_req_thread.lock ();
		RequestManager::m_req_thread.unlock ();

		Rain::RainCout << "sending request" << std::endl;
	}

	//free pointers and close sockets
	void FreeAndCloseMRSI (MRSIParam *mrsiparam)
	{
		closesocket (*(mrsiparam->conn));
		delete mrsiparam->recvparam->message;
		delete mrsiparam->recvparam;
		delete mrsiparam->fmess;
		delete mrsiparam->conn;
		delete mrsiparam;
	}

	DWORD WINAPI MRSRThread (LPVOID param)
	{
		MRSIParam &mrsiparam = *reinterpret_cast<MRSIParam *>(param);
		MarkRequestStoreRecommendations (mrsiparam.code);
		delete &mrsiparam;
		return 0;
	}
}