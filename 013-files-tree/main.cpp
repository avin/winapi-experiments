#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <shlwapi.h>
#include <tchar.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")

HINSTANCE hInst;
HWND hTreeView;
HWND hButton;

std::vector<std::wstring> selectedFiles;

void AddItemsToTree(HWND hTree, HTREEITEM hParent, const std::wstring& path);

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void CopySelectedFilesToClipboard(HWND hwnd);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow)
{
    hInst = hInstance;

    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_TREEVIEW_CLASSES };
    InitCommonControlsEx(&icex);

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"TreeViewApp";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0,
        L"TreeViewApp",
        L"File Tree with Checkboxes",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 400,
        NULL, NULL, hInstance, NULL);

    if (!hwnd)
        return 0;

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}

void InitializeTreeView(HWND hwndParent)
{
    hTreeView = CreateWindowExW(WS_EX_CLIENTEDGE, WC_TREEVIEWW, L"",
        WS_VISIBLE | WS_CHILD | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_CHECKBOXES,
        10, 10, 560, 300,
        hwndParent, (HMENU)1, hInst, NULL);

    std::wstring currentPath(MAX_PATH, L'\0');
    GetCurrentDirectoryW(MAX_PATH, &currentPath[0]);
    currentPath.resize(wcslen(currentPath.c_str()));

    AddItemsToTree(hTreeView, NULL, currentPath);
}

void AddItemsToTree(HWND hTree, HTREEITEM hParent, const std::wstring& path)
{
    WIN32_FIND_DATAW fd;
    HANDLE hFind = INVALID_HANDLE_VALUE;

    std::wstring searchPath = path + L"\\*";

    hFind = FindFirstFileW(searchPath.c_str(), &fd);

    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do
    {
        if (wcscmp(fd.cFileName, L".") != 0 && wcscmp(fd.cFileName, L"..") != 0)
        {
            TVINSERTSTRUCTW tvis = {};
            tvis.hParent = hParent;
            tvis.hInsertAfter = TVI_LAST;
            tvis.item.mask = TVIF_TEXT | TVIF_CHILDREN;
            tvis.item.pszText = fd.cFileName;
            tvis.item.cchTextMax = lstrlenW(fd.cFileName);

            std::wstring fullPath = path + L'\\' + fd.cFileName;

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                tvis.item.cChildren = 1;
                HTREEITEM hItem = TreeView_InsertItem(hTree, &tvis);

                AddItemsToTree(hTree, hItem, fullPath);
            }
            else
            {
                tvis.item.cChildren = 0;
                tvis.item.mask |= TVIF_STATE;
                tvis.item.stateMask = TVIS_STATEIMAGEMASK;
                tvis.item.state = INDEXTOSTATEIMAGEMASK(1); // Unchecked
                TreeView_InsertItem(hTree, &tvis);
            }
        }
    } while (FindNextFileW(hFind, &fd) != 0);

    FindClose(hFind);
}

void CollectCheckedFiles(HWND hTree, HTREEITEM hItem, const std::wstring& path)
{
    TVITEM tvi = {};
    tvi.mask = TVIF_HANDLE | TVIF_STATE | TVIF_TEXT;
    tvi.hItem = hItem;
    tvi.stateMask = TVIS_STATEIMAGEMASK;

    wchar_t buffer[MAX_PATH];
    tvi.pszText = buffer;
    tvi.cchTextMax = MAX_PATH;

    if (!TreeView_GetItem(hTree, &tvi))
        return;

    std::wstring fullPath = path + L'\\' + tvi.pszText;

    int checked = ((tvi.state & TVIS_STATEIMAGEMASK) >> 12) - 1;

    if (checked == 1) // Checked
    {
        DWORD attr = GetFileAttributesW(fullPath.c_str());
        if (!(attr & FILE_ATTRIBUTE_DIRECTORY))
        {
            selectedFiles.push_back(fullPath);
        }
    }

    // Рекурсивный обход дочерних элементов
    HTREEITEM hChild = TreeView_GetChild(hTree, hItem);
    while (hChild)
    {
        CollectCheckedFiles(hTree, hChild, fullPath);
        hChild = TreeView_GetNextSibling(hTree, hChild);
    }
}

void CopySelectedFilesToClipboard(HWND hwnd)
{
    selectedFiles.clear();

    std::wstring currentPath(MAX_PATH, L'\0');
    GetCurrentDirectoryW(MAX_PATH, &currentPath[0]);
    currentPath.resize(wcslen(currentPath.c_str()));

    HTREEITEM hRoot = TreeView_GetRoot(hTreeView);
    while (hRoot)
    {
        CollectCheckedFiles(hTreeView, hRoot, currentPath);
        hRoot = TreeView_GetNextSibling(hTreeView, hRoot);
    }

    if (selectedFiles.empty())
    {
        MessageBoxW(hwnd, L"No files selected.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    std::wstring clipboardText;

    for (const auto& filePath : selectedFiles)
    {
        clipboardText += L"------------------\r\n";
        // Получаем относительный путь
        std::wstring relativePath = L"." + filePath.substr(currentPath.length());
        clipboardText += relativePath + L"\r\n";
        clipboardText += L"------------------\r\n\r\n";

        // Читаем содержимое файла
        HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            DWORD fileSize = GetFileSize(hFile, NULL);
            if (fileSize != INVALID_FILE_SIZE)
            {
                std::vector<char> buffer(fileSize + 1, 0);
                DWORD bytesRead = 0;
                ReadFile(hFile, buffer.data(), fileSize, &bytesRead, NULL);
                // Предполагаем, что файл в кодировке UTF-8; при необходимости измените
                std::wstring fileContent;
                int len = MultiByteToWideChar(CP_UTF8, 0, buffer.data(), bytesRead, NULL, 0);
                if (len > 0)
                {
                    fileContent.resize(len);
                    MultiByteToWideChar(CP_UTF8, 0, buffer.data(), bytesRead, &fileContent[0], len);
                }
                clipboardText += fileContent + L"\r\n";
            }
            CloseHandle(hFile);
        }
    }

    // Копируем в буфер обмена
    if (OpenClipboard(hwnd))
    {
        EmptyClipboard();

        size_t dataSize = (clipboardText.length() + 1) * sizeof(wchar_t);
        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, dataSize);
        if (hGlobal)
        {
            LPVOID pGlobal = GlobalLock(hGlobal);
            memcpy(pGlobal, clipboardText.c_str(), dataSize);
            GlobalUnlock(hGlobal);

            SetClipboardData(CF_UNICODETEXT, hGlobal);
        }
        CloseClipboard();
    }
    else
    {
        MessageBoxW(hwnd, L"Cannot open clipboard.", L"Error", MB_OK | MB_ICONERROR);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        InitializeTreeView(hwnd);
        hButton = CreateWindowW(L"BUTTON", L"Copy", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            10, 320, 100, 30, hwnd, (HMENU)2, hInst, NULL);
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == 2) // Кнопка "Copy"
        {
            CopySelectedFilesToClipboard(hwnd);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}
