#pragma once
#include <Windows.h>
#include <string>


std::string utf16ToUtf8(LPCWSTR lpszSrc);
std::wstring utf8ToUtf16(const std::string& utf8);
