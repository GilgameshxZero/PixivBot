#include "ImageWnd.h"

namespace PixivBot
{
	namespace ImageWnd
	{
		Rain::RainWindow image_wnd;
		bool image_wnd_ready; //whether window has finished drawing

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
			if (!image_wnd_ready)
				return 0;
			if (wparam == VK_LEFT)
				ImageWnd::OnReject ();
			if (wparam == VK_RIGHT)
				ImageWnd::OnAccept ();
			if (wparam == VK_ESCAPE)
				PostMessage (hwnd, RAIN_CLOSEIMGWND, 1, 0);
			return 0;
		}
		LRESULT OnPaint (HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
		{
			if (ImageManager::image_queue.size () == 0 || ImageManager::image_queue.front ().second->size () == 0) //images are still loading
				return 0;

			PAINTSTRUCT ps;
			BeginPaint (hwnd, &ps);

			std::string imgpath = Settings::cache_dir + ImageManager::image_queue.front ().second->front ();
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

			image_wnd_ready = true;

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
			static std::string loaded_image;

			if (ImageManager::awaiting.size () == 0) //done with images
			{
				PostMessage (hwnd, RAIN_CLOSEIMGWND, 0, 0);
				return 0;
			}
			else if (ImageManager::image_queue.size () == 0 && RequestManager::cachethread != 0) //still loading in cache threads
				return 0;
			else if (ImageManager::image_queue.size () == 0 || ImageManager::image_queue.front ().second->size () == 0) //images are still loading
				return 0;
			else if (loaded_image == (*ImageManager::image_queue.front ().second)[0]) //no changes to the current image
				return 0;

			image_wnd_ready = false;
			InvalidateRect (hwnd, NULL, TRUE);
			UpdateWindow (hwnd);

			loaded_image = (*ImageManager::image_queue.front ().second)[0];
			Rain::RainCout << "loaded " << ImageManager::image_queue.front ().second->front () << "\n";

			return 0;
		}

		void OnReject ()
		{
			if (ImageManager::image_queue.size () == 0 || ImageManager::image_queue.front ().second->size () == 0) //images are still loading
				return;

			Rain::RainCout << "rejected " << ImageManager::image_queue.front ().second->front () << "\n";

			//delete cached image
			DeleteFile ((Settings::cache_dir + ImageManager::image_queue.front ().second->front ()).c_str ());

			//remove entry from queue
			ImageManager::image_queue.front ().second->erase (ImageManager::image_queue.front ().second->begin ());
			if (ImageManager::image_queue.front ().second->size () == 0)
			{
				ImageManager::awaiting.erase (ImageManager::image_queue.front ().first);
				ImageManager::inqueue.erase (ImageManager::image_queue.front ().first);
				ImageManager::processed.insert (ImageManager::image_queue.front ().first);
				delete ImageManager::image_queue.front ().second;
				ImageManager::image_queue.pop ();
			}

			//set new image
			SendMessage (ImageWnd::image_wnd.hwnd, RAIN_IMAGECHANGE, 0, 0);
		}

		void OnAccept ()
		{
			if (ImageManager::image_queue.size () == 0 || ImageManager::image_queue.front ().second->size () == 0) //images are still loading
				return;

			Rain::RainCout << "accepted " << ImageManager::image_queue.front ().second->front () << "\n";

			//move image to accepted folder
			MoveFile (((std::string)(Settings::cache_dir + ImageManager::image_queue.front ().second->front ())).c_str (), ((std::string)(Settings::accept_dir + ImageManager::image_queue.front ().second->front ())).c_str ());

			int code = ImageManager::image_queue.front ().first;

			//remove entry from queue
			ImageManager::image_queue.front ().second->erase (ImageManager::image_queue.front ().second->begin ());
			if (ImageManager::image_queue.front ().second->size () == 0)
			{
				ImageManager::awaiting.erase (ImageManager::image_queue.front ().first);
				ImageManager::inqueue.erase (ImageManager::image_queue.front ().first);
				ImageManager::processed.insert (ImageManager::image_queue.front ().first);
				delete ImageManager::image_queue.front ().second;
				ImageManager::image_queue.pop ();
			}

			//don't send RAIN_IMAGECHANGE yet, before loading recs; recs function will do it

			//load recommended images into cache and queue, but do it asynchronously to make the process fast
			MRSIParam *mrsiparam = new MRSIParam ();
			mrsiparam->code = code;
			Rain::SimpleCreateThread (MRSRThread, reinterpret_cast<LPVOID>(mrsiparam));
		}
	}
}