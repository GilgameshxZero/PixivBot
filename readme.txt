[M4 - Monochrome8]
PixivBot

Facilitates browsing through pixiv images and their recommendations.
If you like an image, press the right arrow, and the program will save the image and load it's recommendations into the queue.
If you don't like it, press the left arrow and it will be discarded.
ESC ends the program.
See config_format.txt and config_sample.txt for more configuration options

changelog
	todo
		pixiv seems to be easing away recommendations - figure out a workaround to this

		limiting request threads but processing images fast may lead to instabilities in slow internet conditions, needs fix
		this doesn't happen much at all when threads aren't limited

		change all threads into std::thread
		revise RainCout to become RainCoutF
		something is a little off with thread counts
		make thread limits besides 1 and 0 work
		make the p_saddrinfo_www variable into a map
		rebase RequestManager code
		add timout as a setting in config
		find a better way to clean out std::threads in RequestManager
		some crashes in manga mode, such as with http://www.pixiv.net/member_illust.php?mode=medium&illust_id=48561810
	2.0.0
		added structure to auxiliary and configurations
		updated to RainLibrary3 and imported library into root
		updated BuildID linking to project
		re-added program icon
		adjusted namespace structure to remove namespaces for individual files and structure them around Monochrome8/PixivBot
		refactored code styles around brackets and naming
		added Main.cpp error handling
		removed sensitive information from version control

		refactor code accordingly
	1.2.1
		quick fix for odd manga submissions such as 48561810 which result in program stalling
		increased crash resistability by moving config.txt file writing forward
	1.2.0
		last update should have been a minor update instead of a "revision"
		this update removes a possible race condition in CacheInitImages, as well as adding some debugging output
		also, config.txt is added back into the github repo (personal private information should be difficult to decode from the cookie data)
	1.1.3
		code refactoring:
			removed the sendhandler window, and made OnAccept/OnReject just functions
			removed ImgWndProcDef files
			removed UnivParam and spread variables amoung RequestManager and ImageManager
			renamed variables and functions
			removed dependency on MRSIParam
			switch some threading to std::thread
			now requests have timeouts, and threads are more organized
			removed MRSIParam and MarkRequestStoreImage files, and moved them with timeout threads to RequestManager
		smoother thread creation/deletion with std::thread
		all threads now end peacefully at program exit
	1.1.2
		added to github version control
		added future directions file to list features/bugs to do
		exclude txt files from build
	1.1.1
		code refactoring: added standard rain library to remove race conditions on cout, moved memory leak code to utility
		rain library standardization
		now closes window on ESC again
		removed memory leaks
		updated detection of image request errors
	1.1.0
		accepting the last image in the queue will not end the program prematurely anymore before loading recs
		code refactoring: removed redundant settings, and moved settings to global variables in custom namespace
		added program icon: Kanan, from Love Live, from pixiv illustration 58835247
		removed config host/port options, since those are constant
		usually loads first image of a manga submission first instead of the last image first
		video entries are outputted to the console to be manually dealt with and no longer crash the program
		added memory leak monitor on debug mode
	1.0.3
		fixed bug where requests would return an empty message and crash the program; now retries request
		made cache loading a thread, so can process images while loading cache
		cache is now persistent between program executions, so loading will check if previous execution loaded image already
		OnAccept/OnReject is now faster in slow internet conditions
		fixed warnings
	1.0.2
		added capability to limit concurrent thread count
		fixed req_count related bugs
	1.0.1
		updated error handling for invalid pixiv image requests
		updated safe mode to be more consistent