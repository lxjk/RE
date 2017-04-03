#pragma once

#include <string>

inline std::string trim(std::string& str, bool trimStart = true, bool trimEnd = true)
{
	if(trimStart)
		str.erase(0, str.find_first_not_of(" \t\n\r"));       //prefixing spaces
	if(trimEnd)
		str.erase(str.find_last_not_of(" \t\n\r") + 1);         //surfixing spaces
	return str;
}
