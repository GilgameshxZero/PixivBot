#pragma once

#include <string>

namespace PixivBot
{
	namespace Settings
	{
		extern std::string http_req_header[2][3], //headers for requests, of 6 different types: (r18 illust, r18 recommender, r18 image)(safe illust, safe recommender, safe image)
			accept_dir, //directory to move accepted images
			cache_dir, //directory to store cached images
			pixiv_tt[2]; //account related information, for r18/safe requests
		extern bool safe_mode; //true if safe mode enabled, false if r18 mode; also determines the headers to use in requests
		extern int recs_on_accept, //number of recommendation images to add to bfs when user accepts an image
			max_req_threads; //0 for unlimited, or limit on number of concurrent threads for requests
	}
}