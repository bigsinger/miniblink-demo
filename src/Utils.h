#pragma once
#include <Windows.h>
#include <string>


std::wstring utf8ToUtf16(const std::string& utf8);
std::string utf16ToUtf8(LPCWSTR lpszSrc);
std::string utf16ToAnsi(LPCWSTR lpszSrc);
