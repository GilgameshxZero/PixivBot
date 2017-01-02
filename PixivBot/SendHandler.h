#pragma once

#include "ImgWndProcDef.h"
#include "MarkRequestStoreImage.h"
#include "UnivParam.h"

#include "RainLibraries.h"

#include <iostream>

namespace PixivBot
{
	namespace SendHandler
	{
		void OnReject (UnivParam *uparam);
		void OnAccept (UnivParam *uparam);
	}
}