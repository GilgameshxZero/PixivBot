#pragma once

#include "RainLibraries.h"

#include <string>
#include <vector>

namespace PixivBot
{
	class MRSIParam
	{
		public:
			std::string *fmess; //queue for full messages
			Rain::WSARecvParam *recvparam;
			SOCKET *conn;
			int code;
			int index;
			std::string imagename;
			std::vector<std::string> *namevec;
			std::string link, referer; //for MDSI troubleshooting
	};
}