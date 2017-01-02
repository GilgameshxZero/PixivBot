#include "Start.h"

namespace PixivBot
{
	namespace Start
	{
		struct addrinfo *p_saddrinfo_www;

		int Start ()
		{
			const std::string HOST = "www.pixiv.net",
				HOST_PORT = "80";

			WSADATA wsa_data;

			UnivParam uparam;
			Rain::RainWindow imagewnd, *sendhwnd;
			std::unordered_map<UINT, Rain::RainWindow::MSGFC> imagewndhandler;
			Gdiplus::GdiplusStartupInput gdiplusStartupInput;
			ULONG_PTR gdiplusToken;
			std::unordered_set<int> processed, inqueue, awaiting;
			std::vector<std::string> tmpprocessed;
			int cachethread = 0;
			WPARAM imgwndresult;

			MSG msg;
			BOOL bRet;

			bool imgwndready = false;

			std::vector<int> tmpbsfq;
			HANDLE mem_leak;
			std::unordered_map<UINT, Rain::RainWindow::MSGFC> sendhandler;
			std::fstream config;
			std::queue< std::pair<int, std::vector<std::string> *> > bfsq; //(image code, image filepaths pointer); all images in the queue are loaded; recommendations are added to the queue and loaded if the image is accepted by the user
			std::string tmpline;

			int error;

			mem_leak = Rain::LogMemoryLeaks ("memory_leaks.txt");

			Gdiplus::GdiplusStartup (&gdiplusToken, &gdiplusStartupInput, NULL);

			uparam.imagewnd = &imagewnd;
			uparam.bfsq = &bfsq;
			uparam.processed = &processed;
			uparam.inqueue = &inqueue;
			uparam.cachethread = &cachethread;
			uparam.imgwndready = &imgwndready;
			uparam.tmpbsfq = &tmpbsfq;
			uparam.awaiting = &awaiting;

			Rain::RedirectCerrFile ("pixivbot_error.txt");
			config.open ("config.txt", std::ios_base::in | std::ios_base::binary);

			//get bsf start points
			std::getline (config, tmpline);
			Rain::TrimBSR (tmpline);
			while (tmpline != "")
			{
				int tmp = 0;
				for (unsigned int b = 0; b < tmpline.length (); b++)
				{
					if (tmpline[b] >= '0' && tmpline[b] <= '9')
						tmp = tmp * 10 + tmpline[b] - '0';
				}
				tmpbsfq.push_back (tmp);
				awaiting.insert (tmp);
				std::getline (config, tmpline);
				Rain::TrimBSR (tmpline);
			}

			if (tmpbsfq.size () == 0)
			{
				Rain::RainCout << "no origin images specified\npress enter to quit";
				std::cin.get ();
				config.close ();
				Gdiplus::GdiplusShutdown (gdiplusToken);
				return Rain::ReportError (-1, "No origin images specified.");
			}

			//essential parameters
			std::getline (config, tmpline);
			Rain::TrimBSR (tmpline);
			if (tmpline == "r18")
				Settings::safe_mode = false;
			else if (tmpline == "safe")
				Settings::safe_mode = true;
			else //error
				return Rain::ReportError (-2, "safemode configuration parameter is neither \"r18\" or \"safe\"");

			std::getline (config, tmpline);
			Rain::TrimBSR (tmpline);
			Settings::recs_on_accept = Rain::StrToT<int> (tmpline);
			std::getline (config, Settings::cache_dir);
			Rain::TrimBSR (Settings::cache_dir);
			std::getline (config, Settings::accept_dir);
			Rain::TrimBSR (Settings::accept_dir);
			std::getline (config, tmpline);
			Settings::max_req_threads = Rain::StrToT<int> (tmpline);
			if (Settings::max_req_threads > 1) //currently we can't put limit anything other than 1 or 0
				Settings::max_req_threads = 1;
			for (int a = 0;a < 2;a++)
			{
				std::getline (config, Settings::pixiv_tt[a]);
				Rain::TrimBSR (Settings::pixiv_tt[a]);
			}
			std::getline (config, tmpline);

			error = Rain::InitWinsock (wsa_data);
			if (error) return Rain::ReportError (error);
			error = Rain::GetClientAddr (HOST, HOST_PORT, &p_saddrinfo_www);
			if (error) return Rain::ReportError (error);

			//headers for our requests
			for (int a = 0; a < 6; a++)
			{
				std::getline (config, tmpline);
				Rain::TrimBSR (tmpline);
				while (tmpline != "")
				{
					Settings::http_req_header[a / 3][a % 3] += tmpline + "\n";
					std::getline (config, tmpline);
					Rain::TrimBSR (tmpline);
				}
			}

			//processed images
			std::getline (config, tmpline);
			Rain::TrimBSR (tmpline);
			while (tmpline != "")
			{
				processed.insert (Rain::StrToT<int> (tmpline));
				std::getline (config, tmpline);
				Rain::TrimBSR (tmpline);
			}

			config.close ();

			//scan accepted directory for processed images
			Rain::GetFiles (Settings::accept_dir, tmpprocessed, "*_p*.*");
			for (unsigned int a = 0;a < tmpprocessed.size ();a++)
				processed.insert (Rain::StrToT<int> (tmpprocessed[a].substr (0, tmpprocessed[a].find ("_"))));

			//create window before cache is complete, so that we can process images while they are cached in
			sendhandler.insert (std::make_pair (RAIN_SHACCEPT, SendHandler::OnAccept));
			sendhandler.insert (std::make_pair (RAIN_SHREJECT, SendHandler::OnReject));
			sendhwnd = Rain::CreateSendHandler (&sendhandler);
			uparam.sendhwnd = sendhwnd;
			SetWindowLongPtr (sendhwnd->hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&uparam));

			imagewndhandler.insert (std::make_pair (WM_CLOSE, ImgWndProc::OnClose));
			imagewndhandler.insert (std::make_pair (WM_KEYDOWN, ImgWndProc::OnKeyDown));
			imagewndhandler.insert (std::make_pair (WM_KEYUP, ImgWndProc::OnKeyUp));
			imagewndhandler.insert (std::make_pair (WM_PAINT, ImgWndProc::OnPaint));
			imagewndhandler.insert (std::make_pair (WM_SIZE, ImgWndProc::OnSize));
			imagewndhandler.insert (std::make_pair (RAIN_IMAGECHANGE, ImgWndProc::OnImageChange));
			imagewnd.Create (&imagewndhandler, NULL, NULL, 0, 0, GetModuleHandle (NULL), NULL/**/, reinterpret_cast<HCURSOR>(LoadImage (NULL, MAKEINTRESOURCE (OCR_NORMAL), IMAGE_CURSOR, 0, 0, LR_SHARED | LR_DEFAULTSIZE)), reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1), "", NULL/**/, NULL, "PixivBot Image Window", WS_OVERLAPPEDWINDOW, 0, 0, 1000, 1000, NULL, NULL);
			SetWindowLongPtr (imagewnd.hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&uparam));
			UpdateWindow (imagewnd.hwnd);
			ShowWindow (imagewnd.hwnd, SW_SHOWMAXIMIZED);

			Rain::SimpleCreateThread (CacheImages, reinterpret_cast<LPVOID>(&uparam));

			while ((bRet = GetMessage (&msg, NULL, 0, 0)) != 0)
			{
				if (bRet == -1)
					return -1; //serious error
				else if (msg.hwnd == imagewnd.hwnd && msg.message == RAIN_CLOSEIMGWND) //image window signals close
					break;
				else
				{
					TranslateMessage (&msg);
					DispatchMessage (&msg);
				}
			}

			imgwndresult = msg.wParam;
			ShowWindow (imagewnd.hwnd, SW_HIDE);

			//write to config file the leftover queue at this point
			config.open ("config.txt", std::ios_base::out | std::ios_base::binary);
			for (auto it = awaiting.begin ();it != awaiting.end ();it++)
				config << "http://www.pixiv.net/member_illust.php?mode=medium&illust_id=" << *it << "\n";
			config << "\n"
				<< (Settings::safe_mode ? "safe" : "r18") << "\n"
				<< Settings::recs_on_accept << "\n"
				<< Settings::cache_dir << "\n"
				<< Settings::accept_dir << "\n"
				<< Settings::max_req_threads << "\n"
				<< Settings::pixiv_tt[0] << "\n"
				<< Settings::pixiv_tt[1] << "\n"
				<< "\n";
			for (int a = 0;a < 2;a++)
				for (int b = 0;b < 3;b++)
					config << Settings::http_req_header[a][b] << "\n";
			//headers contain newlines at their end
			for (std::unordered_set<int>::iterator it = processed.begin ();it != processed.end ();it++)
				config << *it << "\n";
			config << "\n";
			config.close ();

			if (imgwndresult == 0) //exited because all images were processed
				Rain::RainCout << "all images were processed\npress enter to quit";
			else if (imgwndresult == 1) //aborted program
				Rain::RainCout << "program aborted before all images were processed\nprogress is saved for next program run\npress enter to quit";
			std::cin.get ();

			imagewnd.~RainWindow ();
			delete sendhwnd;

			freeaddrinfo (p_saddrinfo_www);
			Gdiplus::GdiplusShutdown (gdiplusToken);

			if (mem_leak != NULL)
				CloseHandle (mem_leak);

			return 0;
		}
	}

	DWORD WINAPI CacheImages (LPVOID param)
	{
		UnivParam &uparam = *reinterpret_cast<UnivParam *>(param);
		for (unsigned int a = 0; a < uparam.tmpbsfq->size (); a++)
		{
			//cache images
			if (uparam.processed->find ((*uparam.tmpbsfq)[a]) == uparam.processed->end () && uparam.inqueue->find ((*uparam.tmpbsfq)[a]) == uparam.inqueue->end ())
			{
				MarkRequestStoreImage ((*uparam.tmpbsfq)[a], &uparam);
				uparam.inqueue->insert ((*uparam.tmpbsfq)[a]);
			}
			else
				uparam.awaiting->erase ((*uparam.tmpbsfq)[a]);

			SendMessage (uparam.imagewnd->hwnd, RAIN_IMAGECHANGE, 0, 0);
		}

		return 0;
	}
}