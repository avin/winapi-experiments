#include <windows.h>
#include <string>
#include <ctime>
#include <tlhelp32.h>
#include <psapi.h>
#include <shellapi.h>
#include <vector>
#include <fstream>
#include <sstream>

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define ID_EDIT 1
#define ID_TIMER 1

HWND hEdit;

// Получение временной метки
std::wstring GetTimeStamp() {
    time_t now = time(0);
    tm ltm;
    localtime_s(&ltm, &now);

    wchar_t timestamp[20];
    swprintf(timestamp, 20, L"%02d:%02d:%02d", ltm.tm_hour, ltm.tm_min, ltm.tm_sec);
    return std::wstring(timestamp);
}

// Получение заголовка окна
std::wstring GetWindowTitle(HWND hwnd) {
    wchar_t title[256];
    GetWindowText(hwnd, title, sizeof(title) / sizeof(wchar_t));
    return std::wstring(title);
}

// Получение имени папки из заголовка
std::wstring ExtractFolderName(const std::wstring& title) {
    size_t pos = title.find(L' ');
    if (pos != std::wstring::npos) {
        return title.substr(0, pos);
    }
    return title;
}

// Функция для конвертации UTF-8 в wstring
std::wstring Utf8ToWstring(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// Проверка наличия Git в папке и получение имени ветки
std::wstring GetGitBranch(const std::wstring& folderPath) {
    // Команда для выполнения
    std::wstring gitCommand = L"git -C \"" + folderPath + L"\" rev-parse --abbrev-ref HEAD";

    // Структуры для CreateProcess
    STARTUPINFO si = {sizeof(STARTUPINFO)};
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE; // Скрыть окно консоли

    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

    // Буфер для чтения данных из pipe
    HANDLE hReadPipe, hWritePipe;
    CreatePipe(&hReadPipe, &hWritePipe, &sa, 0);
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    // Настройка на вывод в pipe
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.dwFlags |= STARTF_USESTDHANDLES;

    // Буфер для чтения вывода
    std::vector<char> buffer(128);
    std::string result;

    // Запуск процесса
    if (CreateProcess(NULL, &gitCommand[0], NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(hWritePipe); // Закрываем ненужную часть pipe

        DWORD bytesRead;
        // Чтение вывода команды как байт
        while (ReadFile(hReadPipe, buffer.data(), (DWORD)buffer.size(), &bytesRead, NULL) && bytesRead > 0) {
            result.append(buffer.data(), bytesRead);
        }

        // Ожидание завершения процесса
        WaitForSingleObject(pi.hProcess, INFINITE);

        // Проверка кода завершения процесса
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(hReadPipe);

        // Если команда успешно выполнена (exitCode == 0)
        if (exitCode == 0) {
            // Убираем пробелы и переносы строк
            if (!result.empty()) {
                result.erase(result.find_last_not_of(" \n\r\t") + 1);
            }

            // Конвертируем результат из UTF-8 в wstring
            return Utf8ToWstring(result);
        }
    }

    // Если команда не была успешно выполнена или произошла ошибка
    CloseHandle(hWritePipe);
    CloseHandle(hReadPipe);
    return L"";
}

// Проверка, является ли активное окно окном WebStorm
bool IsWebstormWindow(HWND hwnd) {
    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess) {
        wchar_t processName[MAX_PATH];
        if (GetModuleBaseName(hProcess, NULL, processName, MAX_PATH)) {
            if (std::wstring(processName) == L"webstorm64.exe") {
                CloseHandle(hProcess);
                return true;
            }
        }
        CloseHandle(hProcess);
    }
    return false;
}

// Обновление текстового поля с информацией об активном окне
void UpdateEditWithActiveWindowInfo(HWND hwnd) {
    HWND hActiveWindow = GetForegroundWindow();
    if (hActiveWindow && IsWebstormWindow(hActiveWindow)) {
        std::wstring title = GetWindowTitle(hActiveWindow);
        std::wstring folderName = ExtractFolderName(title);
        std::wstring projectPath = L"C:\\_\\" + folderName;

        std::wstring timestamp = GetTimeStamp();
        std::wstring logEntry = timestamp + L" - " + folderName;

        // Проверяем наличие Git в проектной папке
        std::wstring gitBranch = GetGitBranch(projectPath);
        if (!gitBranch.empty()) {
            logEntry += L" - " + gitBranch;
        }

        logEntry += L"\r\n";

        // Добавляем запись в текстовое поле
        int length = GetWindowTextLength(hEdit);
        SendMessage(hEdit, EM_SETSEL, length, length);
        SendMessage(hEdit, EM_REPLACESEL, 0, (LPARAM)logEntry.c_str());
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                   WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL,
                                   10, 10, 100, 100, hwnd, (HMENU)ID_EDIT, GetModuleHandle(NULL), NULL);
            SetTimer(hwnd, ID_TIMER, 3000, NULL); // Установка таймера на 5 секунд
            return 0;

        case WM_SIZE: {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);

            MoveWindow(hEdit, 0, 0, width, height, TRUE);

            return 0;
        }
        case WM_TIMER:
            if (wParam == ID_TIMER) {
                UpdateEditWithActiveWindowInfo(hwnd);
            }
            return 0;

        case WM_DESTROY:
            KillTimer(hwnd, ID_TIMER);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wcex = {sizeof(WNDCLASSEX)};
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WindowProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.lpszClassName = L"MainWndClass";
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

    RegisterClassEx(&wcex);

    HWND hwnd = CreateWindowEx(0, wcex.lpszClassName, L"Webstorm Logger", WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
