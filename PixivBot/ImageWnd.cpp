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
			if (ImageManager::img_queue.size () == 0 || ImageManager::img_queue.front ().second->size () == 0) //images are still loading
				return 0;

			PAINTSTRUCT ps;
			BeginPaint (hwnd, &ps);

			std::string imgpath = Settings::cache_dir + ImageManager::img_queue.front ().second->front ();
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

			if (ImageManager::img_requesting.size () == 0) //done with images
			{
				PostMessage (hwnd, RAIN_CLOSEIMGWND, 0, 0);
				return 0;
			}
			else if (ImageManager::img_queue.size () == 0 && RequestManager::req_thread_count != 0) //still caching
				return 0;
			else if (ImageManager::img_queue.size () == 0 || ImageManager::img_queue.front ().second->size () == 0) //images are still loading
				return 0;
			else if (loaded_image == (*ImageManager::img_queue.front ().second)[0]) //no changes to the current image
				return 0;

			image_wnd_ready = false;
			InvalidateRect (hwnd, NULL, TRUE);
			UpdateWindow (hwnd);

			loaded_image = (*ImageManager::img_queue.front ().second)[0];
			Rain::RainCout << "loaded " << ImageManager::img_queue.front ().second->front () << std::endl;

			return 0;
		}

		void OnReject ()
		{
			if (ImageManager::img_queue.size () == 0 || ImageManager::img_queue.front ().second->size () == 0) //images are still loading
				return;

			Rain::RainCout << "rejected " << ImageManager::img_queue.front ().second->front () << std::endl;

			//delete cached image
			DeleteFile ((Settings::cache_dir + ImageManager::img_queue.front ().second->front ()).c_str ());

			//remove entry from queue
			ImageManager::QueueRemCurImg ();

			//set new image
			SendMessage (ImageWnd::image_wnd.hwnd, RAIN_IMAGECHANGE, 0, 0);
		}

		void OnAccept ()
		{
			if (ImageManager::img_queue.size () == 0 || ImageManager::img_queue.front ().second->size () == 0) //images are still loading
				return;

			Rain::RainCout << "accepted " << ImageManager::img_queue.front ().second->front () << std::endl;

			//move image to accepted folder
			MoveFile (((std::string)(Settings::cache_dir + ImageManager::img_queue.front ().second->front ())).c_str (), ((std::string)(Settings::accept_dir + ImageManager::img_queue.front ().second->front ())).c_str ());

			MRSIParam *mrsiparam = new MRSIParam ();
			mrsiparam->code = ImageManager::img_queue.front ().first;

			//remove entry from queue
			ImageManager::QueueRemCurImg ();

			//don't send RAIN_IMAGECHANGE yet, before loading recs; recs function will do it
			//load recommended images into cache and queue, but do it asynchronously to make the process fast
			Rain::SimpleCreateThread (MRSRThread, reinterpret_cast<LPVOID>(mrsiparam));
		}
	}
}