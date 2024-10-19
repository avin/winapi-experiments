#include "TrayApp.h"

TrayApp* TrayApp::s_instance = nullptr;

// Конструктор
TrayApp::TrayApp(HINSTANCE hInstance) : m_hInst(hInstance), m_hMainWnd(nullptr), m_hKeyboardHook(nullptr) {
    s_instance = this;

    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = TrayApp::WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"TrayAppClass";

    if (!RegisterClassExW(&wc)) {
        throw std::runtime_error("Failed to register window class");
    }

    m_hMainWnd = CreateWindowExW(0, L"TrayAppClass", L"Tray Hotkey App", 0, 0, 0, 0, 0, NULL, NULL, hInstance, this);
    if (!m_hMainWnd) {
        throw std::runtime_error("Failed to create window");
    }
}

// Деструктор
TrayApp::~TrayApp() {
    s_instance = nullptr;
    RemoveTrayIcon();
    RemoveKeyboardHook();
}

// Основной цикл программы
int TrayApp::Run() {
    AddTrayIcon();
    InstallKeyboardHook();

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
}

// Добавление иконки в трей
void TrayApp::AddTrayIcon() {
    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = m_hMainWnd;
    m_nid.uID = 1001;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = WM_APP + 1;
    m_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcscpy_s(m_nid.szTip, L"Hotkey Tray App");
    Shell_NotifyIconW(NIM_ADD, &m_nid);
}

// Удаление иконки из трея
void TrayApp::RemoveTrayIcon() {
    Shell_NotifyIconW(NIM_DELETE, &m_nid);
}

// Показ контекстного меню при клике правой кнопкой мыши
void TrayApp::ShowTrayMenu() {
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, 2001, L"Exit");

    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(m_hMainWnd);

    // Флаги выравнивания на основе позиции экрана.
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

    TrackPopupMenu(hMenu, uFlags, pt.x, pt.y, 0, m_hMainWnd, NULL);
    
    DestroyMenu(hMenu);
}

// Установка глобального хука клавиатуры
void TrayApp::InstallKeyboardHook() {
    HINSTANCE hModule = GetModuleHandle(NULL);
    m_hKeyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, TrayApp::KeyboardProc, hModule, 0);
    if (!m_hKeyboardHook) {
        throw std::runtime_error("Failed to install keyboard hook");
    }
}

// Удаление хука клавиатуры
void TrayApp::RemoveKeyboardHook() {
    if (m_hKeyboardHook) {
        UnhookWindowsHookEx(m_hKeyboardHook);
        m_hKeyboardHook = nullptr;
    }
}

// Обработка горячей клавиши Win+U
void TrayApp::OnHotkey() {
    // Создаем новый поток для показа MessageBox
    HANDLE hThread = CreateThread(NULL, 0, ShowMessageBoxThread, this, 0, NULL);
    if (hThread) {
        CloseHandle(hThread);
    } else {
        MessageBoxW(NULL, L"Failed to create thread for MessageBox", L"Error", MB_OK | MB_ICONERROR);
    }
}

// Функция потока для показа MessageBox
DWORD WINAPI TrayApp::ShowMessageBoxThread(LPVOID lpParam) {
    TrayApp* pThis = static_cast<TrayApp*>(lpParam);
    if (pThis && pThis->m_hMainWnd) {
        DWORD mainThreadId = GetWindowThreadProcessId(pThis->m_hMainWnd, NULL);
        DWORD currentThreadId = GetCurrentThreadId();

        // Присоединяем потоки ввода
        AttachThreadInput(mainThreadId, currentThreadId, TRUE);

        // Делаем главное окно активным и на переднем плане
        SetForegroundWindow(pThis->m_hMainWnd);
        SetFocus(pThis->m_hMainWnd);

        // Отключаем связь потоков ввода
        AttachThreadInput(mainThreadId, currentThreadId, FALSE);
    }

    // Выводим MessageBox на передний план с фокусом
    MessageBoxW(NULL, L"Win + U Pressed!", L"Hotkey", MB_OK | MB_TOPMOST | MB_SETFOREGROUND);

    return 0;
}

// Обработка выхода
void TrayApp::OnExit() {
    DestroyWindow(m_hMainWnd);
}

// Обработчик оконных сообщений
LRESULT CALLBACK TrayApp::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    TrayApp* pThis = nullptr;
    if (msg == WM_CREATE) {
        pThis = reinterpret_cast<TrayApp*>(((LPCREATESTRUCT)lParam)->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
    } else {
        pThis = reinterpret_cast<TrayApp*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pThis) {
        switch (msg) {
            case WM_APP + 1:
                if (LOWORD(lParam) == WM_RBUTTONUP) {
                    pThis->ShowTrayMenu();
                }
                return 0;

            case WM_COMMAND:
                if (LOWORD(wParam) == 2001) {
                    pThis->OnExit();
                }
                return 0;

            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;

            default:
                break;
        }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// Обработчик клавиатуры
LRESULT CALLBACK TrayApp::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* pKeyBoard = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        if (pKeyBoard->vkCode == 'U' && ((GetAsyncKeyState(VK_LWIN) & 0x8000) || (GetAsyncKeyState(VK_RWIN) & 0x8000))) {
            if (wParam == WM_KEYDOWN) {
                if (s_instance) {
                    s_instance->OnHotkey();
                }
                return 1; // Блокируем дальнейшую обработку комбинации
            }
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}
