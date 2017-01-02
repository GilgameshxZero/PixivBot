#pragma once

#include "ImgWndProcDef.h"
#include "MRSIParam.h"
#include "Settings.h"
#include "Start.h"
#include "UnivParam.h"

#include "RainLibraries.h"

#include <iostream>
#include <string>

namespace PixivBot
{
	//get image with pixiv code `code` and store it in file; create list of files of images in loc vector
	int MarkRequestStoreImage (int code, UnivParam *uparam);

	//helper functions for MarkRequestStoreImage
	int OnMRSIMessage (void *param);
	void OnMRSIExit (void *param);

	//request for multiple image submissions; uses MRSIParam and its message handler
	int MarkParseMangaSubmission (int code, UnivParam *uparam);
	void OnMPMSExit (void *param);

	//request for the mode=manga_big image of the multiple image submissions
	int MarkParseMangaBigImage (int code, int index, std::vector<std::string> *namevec, UnivParam *uparam);
	void OnMPMBIExit (void *param); //start MarkRequestStoreImage for parsed image link

	//request for and save single image from link; uses MRSIParam and its message handler
	int MarkDownloadSingleImage (std::string link, std::string referer, std::vector<std::string> *namevec, UnivParam *uparam);
	void OnMDSIExit (void *param);

	//helper functions to save code
	void OnMRSIInit (void *param);
	void PrepMRSIParams (MRSIParam *mrsiparam, Rain::WSARecvParam *recvparam, UnivParam *uparam);
	void FreeAndCloseMRSI (MRSIParam *mrsiparam);
	DWORD WINAPI MRSRThread (LPVOID param);

	//cache appropriate recommendations to image `code`
	int MarkRequestStoreRecommendations (int code, UnivParam *uparam);
	void OnMRSRExit (void *param);
}