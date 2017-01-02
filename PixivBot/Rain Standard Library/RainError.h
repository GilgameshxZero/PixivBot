/*
Standard
*/

#pragma once

#include <fstream>
#include <iostream>
#include <string>

namespace Rain
{
	int ReportError (int code, std::string desc = "");
	std::streambuf *RedirectCerrFile (std::string filename);
}