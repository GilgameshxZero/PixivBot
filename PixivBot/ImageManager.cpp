#include "ImageManager.h"

namespace PixivBot
{
	namespace ImageManager
	{
		std::queue<std::pair<int, std::vector<std::string> *> > image_queue;
		std::unordered_set<int> processed, inqueue, awaiting;
		std::vector<int> tmpbsfq;
	}
}