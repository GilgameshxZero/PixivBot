#include "RainWSA2SendRecv.h"

namespace Rain
{
	WSARecvParam::WSARecvParam ()
	{
	}

	WSARecvParam::WSARecvParam (SOCKET *sock, std::string *message, int buflen, void *funcparam, WSARecvPMFunc OnProcessMessage, WSARecvInitFunc OnRecvInit, WSARecvExitFunc OnRecvEnd)
	{
		this->sock = sock;
		this->message = message;
		this->buflen = buflen;
		this->funcparam = funcparam;
		this->OnProcessMessage = OnProcessMessage;
		this->OnRecvInit = OnRecvInit;
		this->OnRecvEnd = OnRecvEnd;
	}

	DWORD WINAPI RecvThread (LPVOID lpParameter) {
		WSARecvParam *recvparam = reinterpret_cast<WSARecvParam *>(lpParameter);
		char *buffer = new char[recvparam->buflen];
		int ret;

		if (recvparam->OnRecvInit != NULL)
			recvparam->OnRecvInit (recvparam->funcparam);

		//receive data until the server closes the connection
		do
		{
			ret = recv (*recvparam->sock, buffer, recvparam->buflen, 0);
			if (ret > 0) //received ret bytes
			{
				*recvparam->message = std::string (buffer, ret);
				if (recvparam->OnProcessMessage != NULL)
					if (recvparam->OnProcessMessage (recvparam->funcparam))
						break;
			}
			else if (ret == 0)
				break; //connection closed
			else //failure
			{
				ret = WSAGetLastError ();
				break;
			}
		} while (ret > 0);

		delete[] buffer;
		if (recvparam->OnRecvEnd != NULL)
			recvparam->OnRecvEnd (recvparam->funcparam);

		return ret;
	}

	int SendText (SOCKET &sock, const char *cstrtext, std::size_t len)
	{
		std::size_t sent = 0;
		int ret;

		while (sent < len)
		{
			ret = send (sock, cstrtext + sent, static_cast<int>(len - sent), 0);
			if (ret == SOCKET_ERROR)
			{
				ret = WSAGetLastError ();
				return ret;
			}

			sent += ret;
		}

		return 0;
	}

	int SendText (SOCKET &sock, std::string strtext)
	{
		return SendText (sock, strtext.c_str (), strtext.length ());
	}

	int SendHeader (SOCKET &sock, std::unordered_map<std::string, std::string> *headers)
	{
		std::string message;
		for (std::unordered_map<std::string, std::string>::iterator it = headers->begin (); it != headers->end (); it++)
			message += it->first + ": " + it->second + "\n";
		message += "\n";
		return Rain::SendText (sock, message.c_str (), message.length ());
	}

	HANDLE CreateRecvThread (
		WSARecvParam *recvparam, //if NULL: returns pointer to RecvParam that must be freed when the thread ends
								 //if not NULL: ignores the the next 6 parameters, and uses this as the param for RecvThread
		SOCKET *connection,
		std::string *message, //where the message is stored each time OnProcessMessage is called
		int buflen, //the buffer size of the receive function
		void *funcparam, //additional parameter to pass to the functions OnProcessMessage and OnRecvEnd
		WSARecvPMFunc OnProcessMessage,
		WSARecvInitFunc OnRecvInit, //called when thread starts
		WSARecvExitFunc OnRecvEnd, //called when the other side shuts down send
		DWORD dwCreationFlags,
		SIZE_T dwStackSize,
		LPDWORD lpThreadId,
		LPSECURITY_ATTRIBUTES lpThreadAttributes)
	{
		if (recvparam == NULL)
			recvparam = new WSARecvParam (connection, message, buflen, funcparam, OnProcessMessage, OnRecvInit, OnRecvEnd);

		return CreateThread (lpThreadAttributes, dwStackSize, RecvThread, reinterpret_cast<LPVOID>(recvparam), dwCreationFlags, lpThreadId);
	}

	RainWindow *CreateSendHandler (std::unordered_map<UINT, RainWindow::MSGFC> *msgm)
	{
		RainWindow *rw = new RainWindow ();
		rw->Create (msgm, NULL, NULL, 0, 0, GetModuleHandle (NULL), NULL, NULL, NULL, "", NULL, NULL, "", WS_POPUP, 0, 0, 0, 0, NULL, NULL, RainWindow::NULLCLASSNAME);

		return rw;
	}
}