/*
Standard
*/

#pragma once

#include "RainWindow.h"
#include "RainWSA2Include.h"

#include <chrono>
#include <string>
#include <unordered_map>

namespace Rain
{
	typedef int (*WSARecvPMFunc) (void *);
	typedef void (*WSARecvInitFunc) (void *);
	typedef void (*WSARecvExitFunc) (void *);

	class WSARecvParam
	{
		public:
			WSARecvParam ();
			WSARecvParam (SOCKET *sock, std::string *message, int buflen, void *funcparam, WSARecvPMFunc OnProcessMessage, WSARecvInitFunc OnRecvInit, WSARecvExitFunc OnRecvEnd);

			SOCKET *sock;
			std::string *message;
			int buflen;
			void *funcparam;
			WSARecvPMFunc OnProcessMessage; //return nonzero to terminate recv
			WSARecvInitFunc OnRecvInit;
			WSARecvExitFunc OnRecvEnd;
	};

	//send raw text over a socket
	int SendText (SOCKET &sock, const char *cstrtext, std::size_t len);
	int SendText (SOCKET &sock, std::string strtext);

	int SendHeader (SOCKET &sock, std::unordered_map<std::string, std::string> *headers);

	//calls a function whenever a message is received; 
	DWORD WINAPI RecvThread (LPVOID lpParameter); //don't use this; use CreateRecvThread
	HANDLE CreateRecvThread (
		WSARecvParam *recvparam, //if NULL: returns pointer to RecvParam that must be freed when the thread ends
								 //if not NULL: ignores the the next 6 parameters, and uses this as the param for RecvThread
		SOCKET *connection = NULL,
		std::string *message = NULL, //where the message is stored each time OnProcessMessage is called
		int buflen = NULL, //the buffer size of the receive function
		void *funcparam = NULL, //additional parameter to pass to the functions OnProcessMessage and OnRecvEnd
		WSARecvPMFunc OnProcessMessage = NULL,
		WSARecvInitFunc OnRecvInit = NULL, //called at the beginning of thread
		WSARecvExitFunc OnRecvEnd = NULL, //called when the other side shuts down send
		DWORD dwCreationFlags = 0,
		SIZE_T dwStackSize = 0,
		LPDWORD lpThreadId = NULL,
		LPSECURITY_ATTRIBUTES lpThreadAttributes = NULL);

	//create a message queue/window which will respond to messages sent to it
	//RainWindow * which is returned must be freed
	RainWindow *CreateSendHandler (std::unordered_map<UINT, RainWindow::MSGFC> *msgm);

	//congregate messages from a socket until it closes or timeout
	int RecvUntilTimeout (SOCKET &socket, std::string &message, int timeout_ms = 10000, int buffer_len = 131072);
}