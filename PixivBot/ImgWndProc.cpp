#include "ImgWndProc.h"

namespace PixivBot
{
	namespace ImgWndProc
	{
		LRESULT OnClose (HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
		{
			PostMessage (hwnd, RAIN_CLOSEIMGWND, 1, 0);
			return 0;
		}
		LRESULT OnKeyDown (HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
		{
			return 0;
		}
		LRESULT OnKeyUp (HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
		{
			UnivParam *uparam = reinterpret_cast<UnivParam *>(GetWindowLongPtr (hwnd, GWLP_USERDATA));
			if (!*uparam->imgwndready)
				return 0;
			if (wparam == VK_LEFT)
				SendHandler::OnReject (uparam);
			if (wparam == VK_RIGHT)
				SendHandler::OnAccept (uparam);
			if (wparam == VK_ESCAPE)
				PostMessage (hwnd, RAIN_CLOSEIMGWND, 1, 0);
			return 0;
		}
		LRESULT OnPaint (HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
		{
			UnivParam *uparam = reinterpret_cast<UnivParam *>(GetWindowLongPtr (hwnd, GWLP_USERDATA));

			if (uparam->bfsq->size () == 0 || uparam->bfsq->front ().second->size () == 0) //images are still loading
				return 0;

			PAINTSTRUCT ps;
			BeginPaint (hwnd, &ps);

			std::string imgpath = Settings::cache_dir + uparam->bfsq->front ().second->front ();
			WCHAR *wimgpath;
			wimgpath = new WCHAR[MAX_PATH];
			MultiByteToWideChar (CP_UTF8, 0, imgpath.c_str (), -1, wimgpath, MAX_PATH);
			Gdiplus::Image *img = new Gdiplus::Image (wimgpath);

			RECT crect;
			GetClientRect (hwnd, &crect);
			double resizeratio = min ((double)crect.right / img->GetWidth (), (double)crect.bottom / img->GetHeight ());
			Gdiplus::Rect imgbound (static_cast<INT>((crect.right - resizeratio * img->GetWidth ()) / 2), 
				static_cast<INT>((crect.bottom - resizeratio * img->GetHeight ()) / 2),
				static_cast<INT>(resizeratio * img->GetWidth ()),
				static_cast<INT>(resizeratio * img->GetHeight ()));
			Gdiplus::Graphics *graphics = new Gdiplus::Graphics (ps.hdc);
			graphics->DrawImage (img, imgbound);

			delete graphics;
			delete img;
			delete[] wimgpath;
			EndPaint (hwnd, &ps);

			uparam->munivparam.lock ();
			*uparam->imgwndready = true;
			uparam->munivparam.unlock ();

			return 0;
		}
		LRESULT OnSize (HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
		{
			//redraw the window
			InvalidateRect (hwnd, NULL, TRUE);
			UpdateWindow (hwnd);
			return 0;
		}
		LRESULT OnImageChange (HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
		{
			UnivParam *uparam = reinterpret_cast<UnivParam *>(GetWindowLongPtr (hwnd, GWLP_USERDATA));

			if (uparam->awaiting->size () == 0) //done with images
			{
				PostMessage (hwnd, RAIN_CLOSEIMGWND, 0, 0);
				return 0;
			}
			else if (uparam->bfsq->size () == 0 && *uparam->cachethread != 0) //still loading in cache threads
				return 0;
			else if (uparam->bfsq->size () == 0 || uparam->bfsq->front ().second->size () == 0) //images are still loading
				return 0;
			else if (uparam->lastload == (*uparam->bfsq->front ().second)[0]) //no changes to the current image
				return 0;

			uparam->munivparam.lock ();
			*uparam->imgwndready = false;
			uparam->munivparam.unlock ();
			InvalidateRect (hwnd, NULL, TRUE);
			UpdateWindow (hwnd);

			uparam->munivparam.lock ();
			uparam->lastload = (*uparam->bfsq->front ().second)[0];
			uparam->munivparam.unlock ();
			Rain::RainCout << "loaded " << uparam->bfsq->front ().second->front () << "\n";

			return 0;
		}
	}
}