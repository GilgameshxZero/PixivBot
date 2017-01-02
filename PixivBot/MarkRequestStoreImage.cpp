#include "MarkRequestStoreImage.h"

namespace PixivBot
{
	int MarkRequestStoreImage (int code, UnivParam *uparam)
	{
		Rain::WSARecvParam *recvparam = new Rain::WSARecvParam ();
		MRSIParam *mrsiparam = new MRSIParam ();
		mrsiparam->conn = new SOCKET ();

		//add to the queue here so that we don't send unnecessary requests
		uparam->munivparam.lock ();
		uparam->inqueue->insert (code);
		uparam->munivparam.unlock ();

		if (Rain::CreateClientSocket (&Start::p_saddrinfo_www, *(mrsiparam->conn)))
			return -1;
		if (Rain::ConnToServ (&Start::p_saddrinfo_www, *(mrsiparam->conn)))
			return -2;

		std::string getreq;
		getreq = "GET /member_illust.php?mode=medium&illust_id=" + Rain::TToStr (code) + " HTTP/1.1\nHost: www.pixiv.net\n" + Settings::http_req_header[Settings::safe_mode][0] + "\n";

		recvparam->OnRecvEnd = OnMRSIExit;
		mrsiparam->code = code;

		PrepMRSIParams (mrsiparam, recvparam, uparam);

		Rain::SendText (*(mrsiparam->conn), getreq.c_str (), getreq.length ());
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
		MRSIParam *mrsiparam = reinterpret_cast<MRSIParam *>(param);
		std::string &fmess = *(mrsiparam->fmess);

		DecCacheThread (mrsiparam->uparam);

		if (fmess.length () == 0)
		{
			Rain::RainCout << "MarkRequestStoreImage request returned empty, retrying\n";
			MarkRequestStoreImage (mrsiparam->code, mrsiparam->uparam);
		}
		else
		{
			if ((Settings::safe_mode && fmess.find ("Access by users under age 18 is restricted.") != std::string::npos) || //check safety because recommender doesn't do that
				(fmess.find ("Artist has made their work private.") != std::string::npos) ||  //check for private works
				(fmess.find ("ugokuIllustData") != std::string::npos) || //check for gif works
				(fmess.find ("This work was deleted.") != std::string::npos)) //deleted work
			{
				if (fmess.find ("ugokuIllustData") != std::string::npos)
					Rain::RainCout << "MANUAL CONFIMATION (VIDEO): " <<  mrsiparam->code << std::endl;

				mrsiparam->uparam->munivparam.lock ();
				mrsiparam->uparam->inqueue->erase (mrsiparam->code);
				mrsiparam->uparam->processed->insert (mrsiparam->code);
				mrsiparam->uparam->awaiting->erase (mrsiparam->code);
				mrsiparam->uparam->munivparam.unlock ();

				PostMessage (mrsiparam->uparam->imagewnd->hwnd, RAIN_IMAGECHANGE, 0, 0);
				FreeAndCloseMRSI (mrsiparam);
				return;
			}

			//parse fmess for original image(s)
			std::size_t orig = fmess.find ("_illust_modal _hidden ui-modal-close-box");

			if (orig == std::string::npos)
			{
				if (fmess.find ("mode=manga") != std::string::npos) //is a manga/multiple image submission
					MarkParseMangaSubmission (mrsiparam->code, mrsiparam->uparam);
				else //we have a problem
				{
					Rain::RainCout << "MANUAL CONFIMATION (???): " << mrsiparam->code << std::endl;
					mrsiparam->uparam->munivparam.lock ();
					mrsiparam->uparam->inqueue->erase (mrsiparam->code);
					mrsiparam->uparam->processed->insert (mrsiparam->code);
					mrsiparam->uparam->awaiting->erase (mrsiparam->code);
					mrsiparam->uparam->munivparam.unlock ();

					PostMessage (mrsiparam->uparam->imagewnd->hwnd, RAIN_IMAGECHANGE, 0, 0);
					FreeAndCloseMRSI (mrsiparam);
					return;
				}
			}
			else //single image submission
			{
				//create vector in bsfq for image name storage
				mrsiparam->uparam->munivparam.lock ();
				mrsiparam->uparam->bfsq->push (std::make_pair (mrsiparam->code, new std::vector<std::string> ()));
				mrsiparam->uparam->munivparam.unlock ();

				orig = fmess.find ("data-src=\"", orig) + 10;
				MarkDownloadSingleImage (fmess.substr (orig, fmess.find ("\"", orig) - orig), "http://www.pixiv.net/member_illust.php?mode=medium&illust_id=" + Rain::TToStr (mrsiparam->code), mrsiparam->uparam->bfsq->back ().second, mrsiparam->uparam);
			}
		}

		FreeAndCloseMRSI (mrsiparam);
	}

	int MarkParseMangaSubmission (int code, UnivParam *uparam)
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

		PrepMRSIParams (mrsiparam, recvparam, uparam);

		Rain::SendText (*(mrsiparam->conn), getreq.c_str (), getreq.length ());
		return 0;
	}
	void OnMPMSExit (void *param)
	{
		MRSIParam *mrsiparam = reinterpret_cast<MRSIParam *>(param);
		std::string &fmess = *(mrsiparam->fmess);

		DecCacheThread (mrsiparam->uparam);

		if (fmess.length () == 0)
		{
			Rain::RainCout << "MarkParseMangaSubmission request returned empty, retrying\n";
			MarkParseMangaSubmission (mrsiparam->code, mrsiparam->uparam);
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

			mrsiparam->uparam->munivparam.lock ();
			//create bfsq vector
			mrsiparam->uparam->bfsq->push (std::make_pair (mrsiparam->code, new std::vector<std::string> ()));
			mrsiparam->uparam->munivparam.unlock ();

			for (int a = 0;a < images;a++)
				MarkParseMangaBigImage (mrsiparam->code, a, mrsiparam->uparam->bfsq->back ().second, mrsiparam->uparam);
		}

		FreeAndCloseMRSI (mrsiparam);
	}

	int MarkParseMangaBigImage (int code, int index, std::vector<std::string> *namevec, UnivParam *uparam)
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

		PrepMRSIParams (mrsiparam, recvparam, uparam);

		Rain::SendText (*(mrsiparam->conn), getreq.c_str (), getreq.length ());
		return 0;
	}
	void OnMPMBIExit (void *param)
	{
		MRSIParam *mrsiparam = reinterpret_cast<MRSIParam *>(param);
		std::string &fmess = *(mrsiparam->fmess);

		DecCacheThread (mrsiparam->uparam);

		if (fmess.length () == 0)
		{
			Rain::RainCout << "MarkParseMangaBigImage request returned empty, retrying\n";
			MarkParseMangaBigImage (mrsiparam->code, mrsiparam->index, mrsiparam->namevec, mrsiparam->uparam);
		}
		else
		{
			//get the original image link from message, and start MarkDownloadSingleImage for it
			std::size_t search = 0;
			int images = 0;

			search = fmess.find ("img src=\"") + 9;

			MarkDownloadSingleImage (fmess.substr (search, fmess.find ("\"", search) - search), "http://www.pixiv.net/member_illust.php?mode=manga_big&illust_id=" + Rain::TToStr (mrsiparam->code) + "&page=" + Rain::TToStr (mrsiparam->index), mrsiparam->namevec, mrsiparam->uparam);
		}

		FreeAndCloseMRSI (mrsiparam);
	}

	int MarkDownloadSingleImage (std::string link, std::string referer, std::vector<std::string> *namevec, UnivParam *uparam)
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
			Rain::RainCout << "discovered " << mrsiparam->imagename << " in cache, skipping\n";
			mrsiparam->namevec->push_back (mrsiparam->imagename);

			if (uparam->imagewnd->hwnd != NULL)
				PostMessage (uparam->imagewnd->hwnd, RAIN_IMAGECHANGE, 0, 0);

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

			PrepMRSIParams (mrsiparam, recvparam, uparam);

			Rain::SendText (*(mrsiparam->conn), getreq.c_str (), getreq.length ());
		}

		return 0;
	}
	void OnMDSIExit (void *param)
	{
		MRSIParam *mrsiparam = reinterpret_cast<MRSIParam *>(param);
		std::string &fmess = *(mrsiparam->fmess);

		DecCacheThread (mrsiparam->uparam);

		if (fmess.length () == 0)
		{
			Rain::RainCout << "MarkDownloadSingleImage request returned empty, retrying\n";

			//resend MDSI request
			MarkDownloadSingleImage (mrsiparam->link, mrsiparam->referer, mrsiparam->namevec, mrsiparam->uparam);
		}
		else
		{
			//cut from the first \n\n to the end of the string; save it in a file called mrsiparam->imagename, and put that imagename in mrsiparam->loc
			std::size_t imagestart = fmess.find ("\r\n\r\n") + 4;
			Rain::FastOutputFile (Settings::cache_dir + mrsiparam->imagename, fmess.substr (imagestart, fmess.length () - imagestart));
			mrsiparam->namevec->push_back (mrsiparam->imagename);

			Rain::RainCout << "cached " << mrsiparam->imagename << std::endl;

			//update the image window with the newly loaded image
			if (mrsiparam->uparam->imagewnd->hwnd != NULL)
				PostMessage (mrsiparam->uparam->imagewnd->hwnd, RAIN_IMAGECHANGE, 0, 0);
		}

		FreeAndCloseMRSI (mrsiparam);
	}

	int MarkRequestStoreRecommendations (int code, UnivParam *uparam)
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

		PrepMRSIParams (mrsiparam, recvparam, uparam);

		Rain::SendText (*(mrsiparam->conn), getreq.c_str (), getreq.length ());
		return 0;
	}
	void OnMRSRExit (void *param)
	{
		MRSIParam *mrsiparam = reinterpret_cast<MRSIParam *>(param);
		std::string &fmess = *(mrsiparam->fmess);

		DecCacheThread (mrsiparam->uparam);

		if (fmess.length () == 0)
		{
			Rain::RainCout << "MarkRequestStoreRecommendations request returned empty, retrying\n";
			MarkRequestStoreRecommendations (mrsiparam->code, mrsiparam->uparam);
		}
		else
		{
			//for each recommendation, check if its processed or in queue; if not, markrequeststoreimage it
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

			mrsiparam->uparam->munivparam.lock ();
			for (std::size_t a = 0;a < recs.size ();a++)
				if (mrsiparam->uparam->processed->find (recs[a]) == mrsiparam->uparam->processed->end () &&
					mrsiparam->uparam->inqueue->find (recs[a]) == mrsiparam->uparam->inqueue->end ())
					mrsiparam->uparam->awaiting->insert (recs[a]);
			mrsiparam->uparam->munivparam.unlock ();

			if (mrsiparam->uparam->imagewnd->hwnd != NULL)
				SendMessage (mrsiparam->uparam->imagewnd->hwnd, RAIN_IMAGECHANGE, 0, 0);

			for (std::size_t a = 0;a < recs.size ();a++)
				if (mrsiparam->uparam->processed->find (recs[a]) == mrsiparam->uparam->processed->end () && 
					mrsiparam->uparam->inqueue->find (recs[a]) == mrsiparam->uparam->inqueue->end ())
					MarkRequestStoreImage (recs[a], mrsiparam->uparam);
		}

		FreeAndCloseMRSI (mrsiparam);
	}

	void OnMRSIInit (void *param)
	{
		MRSIParam *mrsiparam = reinterpret_cast<MRSIParam *>(param);

		IncCacheThread (mrsiparam->uparam);
	}

	void PrepMRSIParams (MRSIParam *mrsiparam, Rain::WSARecvParam *recvparam, UnivParam *uparam)
	{
		mrsiparam->fmess = new std::string ();
		mrsiparam->recvparam = recvparam;
		mrsiparam->uparam = uparam;

		recvparam->buflen = 1024;
		recvparam->funcparam = mrsiparam;
		recvparam->message = new std::string ();
		recvparam->OnProcessMessage = OnMRSIMessage; //recieve full message
		recvparam->sock = mrsiparam->conn;
		recvparam->OnRecvInit = OnMRSIInit;

		Rain::CreateRecvThread (recvparam);

		//prevents sending of requests until threads are open
		uparam->mcthread.lock ();
		uparam->mcthread.unlock ();

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
		MarkRequestStoreRecommendations (mrsiparam.code, mrsiparam.uparam);
		delete &mrsiparam;
		return 0;
	}
}