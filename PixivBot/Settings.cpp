#include "Settings.h"

namespace Monochrome8 {
	namespace PixivBot {
		std::string http_req_header[2][3],
			accept_dir,
			cache_dir,
			pixiv_tt[2];
		bool safe_mode;
		int recs_on_accept,
			max_req_threads;
	}
}