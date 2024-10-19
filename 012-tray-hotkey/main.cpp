#include <windows.h>
#include <stdexcept>
#include "TrayApp.h"

// Точка входа
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    try {
        TrayApp app(hInstance);
        return app.Run();
    } catch (const std::exception& e) {
        MessageBoxA(nullptr, e.what(), "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
}
