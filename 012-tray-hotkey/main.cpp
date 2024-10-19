#include <windows.h>
#include <wchar.h>

#define ID_TRAY_APP_ICON    1001
#define WM_TRAYICON         (WM_USER + 1)
#define HOTKEY_ID           1
#define ID_TRAY_EXIT        2001 // Идентификатор пункта "Выход" в меню

HINSTANCE hInst;
NOTIFYICONDATAW nid = {0};
HHOOK hKeyboardHook;
HWND hMainWnd;

DWORD WINAPI ShowMessageBoxThread(LPVOID lpParam) {
    if (hMainWnd) {
        DWORD mainThreadId = GetWindowThreadProcessId(hMainWnd, NULL);
        DWORD currentThreadId = GetCurrentThreadId();

        // Присоединяем потоки ввода
        AttachThreadInput(mainThreadId, currentThreadId, TRUE);

        // Делаем главное окно активным и на переднем плане
        SetForegroundWindow(hMainWnd);
        SetFocus(hMainWnd); // Прямо устанавливаем фокус на главное окно

        // Отключаем связь потоков ввода
        AttachThreadInput(mainThreadId, currentThreadId, FALSE);
    }

    // Выводим MessageBox на передний план с фокусом
    MessageBoxW(NULL, L"Win + U Pressed!", L"Hotkey", MB_OK | MB_TOPMOST | MB_SETFOREGROUND);

    return 0;
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* pKeyBoard = (KBDLLHOOKSTRUCT*)lParam;

        // Проверяем, нажата ли клавиша U и удерживается ли клавиша Win
        if ((pKeyBoard->vkCode == 'U') && (GetAsyncKeyState(VK_LWIN) & 0x8000)) {
            if (wParam == WM_KEYDOWN) {
                // Выводим сообщение при нажатии комбинации Win + U через тред, чтобы не блокировать этот поток, иначе хук не заблокирует оригинальный хоткей.
                HANDLE hThread = CreateThread(NULL, 0, ShowMessageBoxThread, NULL, 0, NULL);
                if (hThread != NULL) {
                    CloseHandle(hThread); // Закрываем дескриптор потока
                }

                return 1; // Блокируем дальнейшую обработку этой комбинации
            }
        }
    }
    return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

void ShowTrayMenu(HWND hwnd) {
    // Создаем меню
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");

    // Получаем координаты курсора
    POINT pt;
    GetCursorPos(&pt);

    // Отображаем контекстное меню
    SetForegroundWindow(hwnd); // Необходимо для корректного отображения меню

    // Adjust the alignment flags based on the screen position
    UINT uFlags = TPM_RIGHTBUTTON;

    APPBARDATA appBarData = {};
    appBarData.cbSize = sizeof(APPBARDATA);

    // Запрашиваем информацию о панели задач
    if (SHAppBarMessage(ABM_GETTASKBARPOS, &appBarData)) {
        switch (appBarData.uEdge) {
            case ABE_TOP:
                uFlags |= TPM_LEFTALIGN | TPM_TOPALIGN;
                break;
            case ABE_RIGHT:
                uFlags |= TPM_RIGHTALIGN | TPM_TOPALIGN;
            case ABE_LEFT:
                uFlags |= TPM_LEFTALIGN | TPM_TOPALIGN;
                break;
            default:
                uFlags |= TPM_LEFTALIGN | TPM_BOTTOMALIGN;
                break;
        }
    }

    TrackPopupMenu(hMenu, uFlags, pt.x, pt.y, 0, hwnd, NULL);

    // Удаляем меню после использования
    DestroyMenu(hMenu);
}

void AddTrayIcon(HWND hwnd) {
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hwnd;
    nid.uID = ID_TRAY_APP_ICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcscpy_s(nid.szTip, L"Hotkey Tray App");
    Shell_NotifyIconW(NIM_ADD, &nid);
}

void RemoveTrayIcon() {
    Shell_NotifyIconW(NIM_DELETE, &nid);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            AddTrayIcon(hwnd);

        // Устанавливаем глобальный хук клавиатуры
            hKeyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
            if (hKeyboardHook == NULL) {
                MessageBoxW(hwnd, L"Failed to install keyboard hook!", L"Error", MB_ICONERROR);
                PostQuitMessage(1);
            }
            break;

        case WM_TRAYICON:
            // Проверяем, что нажата правая кнопка мыши
            if (LOWORD(lParam) == WM_RBUTTONUP) {
                ShowTrayMenu(hwnd); // Показываем меню при правом клике
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_TRAY_EXIT:
                    DestroyWindow(hwnd);
                    break;
            }
            break;

        case WM_DESTROY:
            // Убираем хук и удаляем иконку из трея
            UnhookWindowsHookEx(hKeyboardHook);
            RemoveTrayIcon();
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;

    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"TrayAppClass";

    if (!RegisterClassExW(&wc))
        return 1;

    HWND hwnd = CreateWindowExW(0, L"TrayAppClass", L"Tray Hotkey App", 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
    hMainWnd = hwnd;

    if (!hwnd)
        return 1;

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
