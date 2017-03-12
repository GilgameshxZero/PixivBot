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
			std::vector<int> img_queue_init; //initial image queue to cache, from config file

			std::unordered_map<UINT, Rain::RainWindow::MSGFC> imagewndhandler;
			Gdiplus::GdiplusStartupInput gdiplusStartupInput;
			ULONG_PTR gdiplusToken;
			std::vector<std::string> tmpimg_processed;
			WPARAM imgwndresult;

			MSG msg;
			BOOL bRet;

			HANDLE mem_leak;
			std::fstream config;
			std::string tmpline;

			int error;

			mem_leak = Rain::LogMemoryLeaks ("memory_leaks.txt");

			Rain::RedirectCerrFile ("pixivbot_error.txt");
			Rain::RainCout << "Reading configuration file..." << std::endl;
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
				img_queue_init.push_back (tmp);
				ImageManager::img_requesting.insert (tmp);
				std::getline (config, tmpline);
				Rain::TrimBSR (tmpline);
			}

			if (img_queue_init.size () == 0)
			{
				Rain::RainCout << "no origin images specified\npress enter to quit";
				std::cin.get ();
				config.close ();
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

			Rain::RainCout << "Initializing winsock and fetching information about pixiv.net..." << std::endl;
			error = Rain::InitWinsock (wsa_data);
			if (error) return Rain::ReportError (error);
			error = Rain::GetClientAddr (HOST, HOST_PORT, &p_saddrinfo_www);
			if (error) return Rain::ReportError (error);
			Rain::RainCout << "Done." << std::endl;

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

			//img_processed images
			std::getline (config, tmpline);
			Rain::TrimBSR (tmpline);
			while (tmpline != "")
			{
				ImageManager::img_processed.insert (Rain::StrToT<int> (tmpline));
				std::getline (config, tmpline);
				Rain::TrimBSR (tmpline);
			}

			config.close ();
			Rain::RainCout << "Finished reading configuration file. Scanning accepted directory..." << std::endl;

			//scan accepted directory for img_processed images
			Rain::GetFiles (Settings::accept_dir, tmpimg_processed, "*_p*.*");
			for (unsigned int a = 0;a < tmpimg_processed.size ();a++)
				ImageManager::img_processed.insert (Rain::StrToT<int> (tmpimg_processed[a].substr (0, tmpimg_processed[a].find ("_"))));

			Rain::RainCout << "Done. Creating image window..." << std::endl;

			//create window so that we can process images while they are cached in
			Gdiplus::GdiplusStartup (&gdiplusToken, &gdiplusStartupInput, NULL);

			imagewndhandler.insert (std::make_pair (WM_CLOSE, ImageWnd::OnClose));
			imagewndhandler.insert (std::make_pair (WM_KEYDOWN, ImageWnd::OnKeyDown));
			imagewndhandler.insert (std::make_pair (WM_KEYUP, ImageWnd::OnKeyUp));
			imagewndhandler.insert (std::make_pair (WM_PAINT, ImageWnd::OnPaint));
			imagewndhandler.insert (std::make_pair (WM_SIZE, ImageWnd::OnSize));
			imagewndhandler.insert (std::make_pair (RAIN_IMAGECHANGE, ImageWnd::OnImageChange));
			ImageWnd::image_wnd.Create (&imagewndhandler, NULL, NULL, 0, 0, GetModuleHandle (NULL), NULL/**/, reinterpret_cast<HCURSOR>(LoadImage (NULL, MAKEINTRESOURCE (OCR_NORMAL), IMAGE_CURSOR, 0, 0, LR_SHARED | LR_DEFAULTSIZE)), reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1), "", NULL/**/, NULL, "PixivBot Image Window", WS_OVERLAPPEDWINDOW, 0, 0, 1000, 1000, NULL, NULL);
			UpdateWindow (ImageWnd::image_wnd.hwnd);
			ShowWindow (ImageWnd::image_wnd.hwnd, SW_SHOWMAXIMIZED);

			Rain::SimpleCreateThread (ImageManager::CacheInitImages, &img_queue_init);

			while ((bRet = GetMessage (&msg, NULL, 0, 0)) != 0)
			{
				if (bRet == -1)
					return -1; //serious error
				else if (msg.hwnd == ImageWnd::image_wnd.hwnd && msg.message == RAIN_CLOSEIMGWND) //image window signals close
					break;
				else
				{
					TranslateMessage (&msg);
					DispatchMessage (&msg);
				}
			}

			RequestManager::JoinRemoveThreads ();

			imgwndresult = msg.wParam;
			ShowWindow (ImageWnd::image_wnd.hwnd, SW_HIDE);

			//write to config file the leftover queue at this point
			config.open ("config.txt", std::ios_base::out | std::ios_base::binary);
			for (std::unordered_set<int>::iterator it = ImageManager::img_requesting.begin ();it != ImageManager::img_requesting.end ();it++)
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
			for (std::unordered_set<int>::iterator it = ImageManager::img_processed.begin ();it != ImageManager::img_processed.end ();it++)
				config << *it << "\n";
			config << "\n";
			config.close ();

			if (imgwndresult == 0) //exited because all images were img_processed
				Rain::RainCout << "all images were processed\npress enter to quit";
			else if (imgwndresult == 1) //aborted program
				Rain::RainCout << "program aborted before all images were processed\nprogress is saved for next program run\npress enter to quit";
			std::cin.get ();

			ImageWnd::image_wnd.~RainWindow ();

			freeaddrinfo (p_saddrinfo_www);
			Gdiplus::GdiplusShutdown (gdiplusToken);

			if (mem_leak != NULL)
				CloseHandle (mem_leak);

			return 0;
		}
	}
}