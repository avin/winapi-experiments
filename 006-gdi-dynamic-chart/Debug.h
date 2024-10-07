#pragma once

#include <cwchar>

void DebugOutputImpl(const wchar_t* format, ...) {
  wchar_t msg[255] = {0};

  va_list args;
  va_start(args, format);
  vswprintf(msg, sizeof(msg) / sizeof(wchar_t), format, args);
  va_end(args);

  OutputDebugStringW(msg);
}

#ifdef _DEBUG
#define DebugOutput(format, ...) \
DebugOutputImpl(format, __VA_ARGS__)
#else
#define DebugOutput(format, ...) \
do {} while (0)  // Пустой макрос, который не делает ничего
#endif
