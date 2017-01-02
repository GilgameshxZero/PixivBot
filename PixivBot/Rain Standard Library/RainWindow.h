/*
Standard
*/

#pragma once

#include "RainUtility.h"
#include "RainWindowsLAM.h"

#include <tchar.h>
#include <unordered_map>
#include <vector>

//first available value for custom messages
#define WM_RAINAVAILABLE	WM_APP + 100

namespace Rain
{
	//does not interfere with GWLP_USERDATA
	class RainWindow
	{
		public:
			typedef LRESULT (*MSGFC)(HWND, UINT, WPARAM, LPARAM);

			static const LPCTSTR NULLCLASSNAME;

			HWND hwnd;

			RainWindow ();
			~RainWindow ();
			int Create (
				std::unordered_map<UINT, MSGFC> *msgm, //pointer to a map which maps messages to functions to be called when the message is received
				MSGFC *intfc = NULL, //interceptor function: if the application wants to define a custom message handler, here is where to do it
				UINT      style = NULL,
				int       cbClsExtra = 0,
				int	      cbWndExtra = 0,
				HINSTANCE hInstance = GetModuleHandle (NULL),
				HICON     hIcon = NULL,
				HCURSOR   hCursor = NULL,
				HBRUSH    hbrBackground = NULL,
				LPCTSTR   lpszMenuName = _T (""),
				HICON     hIconSm = NULL,
				DWORD     dwExStyle = NULL,
				LPCTSTR   lpWindowName = _T (""),
				DWORD     dwStyle = WS_OVERLAPPEDWINDOW,
				int       x = 0,
				int       y = 0,
				int       nWidth = 0,
				int       nHeight = 0,
				HWND      hWndParent = NULL,
				HMENU     hMenu = NULL,
				LPCTSTR   lpszClassName = NULLCLASSNAME);
			LPCTSTR GetWndClassName ();

			static RainWindow *GetWndObj (HWND hwndid);

			//can be called by anything; enter message loop for all windows; returns when WM_QUIT is received
			static WPARAM EnterMessageLoop ();

		private:
			static int class_id;
			static std::unordered_map<HWND, RainWindow *> objmap;

			std::unordered_map<UINT, MSGFC> *msgm;
			MSGFC *intfc;
			LPTSTR classname;

			friend LRESULT CALLBACK RainWindowProc ( //the windowproc is controlled by this class; process messages through the map in the initializer; if intfc != NULL, this function is not used
				HWND   hwnd,
				UINT   uMsg,
				WPARAM wParam,
				LPARAM lParam
			);
	};

	LRESULT CALLBACK RainWindowProc (
		HWND   hwnd,
		UINT   uMsg,
		WPARAM wParam,
		LPARAM lParam
	);
}