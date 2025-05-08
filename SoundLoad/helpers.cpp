#include "pch.hpp"
#include "helpers.hpp"

std::string StrFromWStr(const std::wstring src)
{
	const int sz = WideCharToMultiByte(CP_UTF8, 0, src.c_str(), -1, nullptr, 0, nullptr, nullptr);

	std::string str(sz - 1, 0);
	WideCharToMultiByte(CP_UTF8, 0, src.c_str(), -1, str.data(), sz, nullptr, nullptr);

	return str;
}

std::wstring WStrFromStr(const std::string src)
{
	const int sz = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, nullptr, 0);

	std::wstring wstr(sz - 1, 0);
	MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, wstr.data(), sz);

	return wstr;
}