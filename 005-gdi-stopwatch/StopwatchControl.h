#pragma once

#include <windows.h>

#define WM_GET_TIMER_STATUS (WM_USER + 1)
#define WM_GET_TIMER_TIME (WM_USER + 2)

#define WM_COMMAND_TIMER_START (WM_USER + 101)
#define WM_COMMAND_TIMER_STOP (WM_USER + 102)
#define WM_COMMAND_TIMER_RESET (WM_USER + 103)

// Функция для регистрации класса окна таймера
void RegisterStopwatchControl(HINSTANCE hInstance);
