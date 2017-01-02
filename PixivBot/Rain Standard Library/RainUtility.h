/*
Standard
*/

#pragma once

#include "RainUtilityFile.h"
#include "RainWindowsLAM.h"

#include <iomanip> //EncodeURL and DecodeURL
#include <sstream>
#include <string>

namespace Rain
{
	template <typename T>
	std::string TToStr (T x)
	{
		std::stringstream ss;
		ss << x;
		return ss.str ();
	}

	template <typename T>
	T StrToT (std::string s)
	{
		T r;
		std::stringstream ss (s);
		ss >> r;
		return r;
	}

	char IntToBase64 (int x);
	int Base64ToInt (char c);
	void EncodeBase64 (const std::string &str, std::string &rtrn);
	void DecodeBase64 (const std::string &str, std::string &rtrn);

	std::string EncodeURL (const std::string &value);
	std::string DecodeURL (const std::string &value);

	//length of x, interpreted as a base10 string
	int IntLogLen (int x);

	HANDLE SimpleCreateThread (LPTHREAD_START_ROUTINE threadfunc, LPVOID threadparam);

	//dumps memory leaks to a file if on debug mode; application must CloseHandle the return HANDLE, unless it's debug mode and the return is NULL
	HANDLE LogMemoryLeaks (std::string out_file);
}