#include "debug_output.h"

#define WIN32_LEAN_AND_MEAN				// NOTE: Ok to use here because it's specific to this project, just like in settings_file_manager.h
#include <Windows.h>

#include <cstdint>
#include <cstdlib>																									// This is included because we need access to _itoa.
#include <string>

DebugOutput& DebugOutput::operator<<(const char* input) {
#ifndef _DEBUG
	return *this;
#endif
	OutputDebugStringA(input);
	return *this;
}

DebugOutput& DebugOutput::operator<<(char* input) {
#ifndef _DEBUG
	return *this;
#endif
	OutputDebugStringA(input);
	return *this;
}

DebugOutput& DebugOutput::operator<<(char input) {
#ifndef _DEBUG
	return *this;
#endif
	char buffer[] = { input, '\0' };																				// List initialization of basic types is apparently just as efficient as doing it per hand, contrary to custom types and such. Use equal sign because that makes it compatible with C.
	OutputDebugStringA(buffer);
	return *this;
}

DebugOutput& DebugOutput::operator<<(std::string& input) {
#ifndef _DEBUG
	return *this;
#endif
	OutputDebugStringA(input.c_str());
	return *this;
}

DebugOutput& DebugOutput::operator<<(int32_t input) {
#ifndef _DEBUG
	return *this;
#endif
	char buffer[12];
	_itoa(input, buffer, 10);
	OutputDebugStringA(buffer);
	return *this;
}

DebugOutput& DebugOutput::operator<<(uint32_t input) {
#ifndef _DEBUG
	return *this;
#endif
	char buffer[11];
	_itoa(input, buffer, 10);
	OutputDebugStringA(buffer);
	return *this;
}