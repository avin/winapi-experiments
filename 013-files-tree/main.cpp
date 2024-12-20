#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <shlwapi.h>
#include <tchar.h>
#include <shlobj.h>
#include <shellapi.h>
#include <commdlg.h> // For IFileDialog

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "Ole32.lib") // For COM initialization

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define IDM_SELECT_FOLDER 4

HINSTANCE hInst;
HWND hTreeView;
HWND hButton;
HIMAGELIST hImageList = NULL;

std::wstring basePath;
std::vector<std::wstring> selectedFiles;

void AddItemsToTree(HWND hTree, HTREEITEM hParent, const std::wstring& path);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void CopySelectedFilesToClipboard(HWND hwnd);
std::wstring SelectFolder(HWND hwnd);
void CollectCheckedFiles(HWND hTree, HTREEITEM hItem, const std::wstring& path);
HICON CreatePaddedIcon(HICON hOriginalIcon, int padding) ;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    hInst = hInstance;

    // Initialize COM
    CoInitialize(NULL);

    INITCOMMONCONTROLSEX icex = {sizeof(icex), ICC_TREEVIEW_CLASSES};
    InitCommonControlsEx(&icex);

    WNDCLASSEXW wcex = {sizeof(WNDCLASSEX)};
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.lpszClassName = L"TreeViewApp";
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassExW(&wcex);

    // Получаем размеры экрана
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Задаем размеры окна
    int windowWidth = 600;
    int windowHeight = 450;

    // Вычисляем координаты для центра экрана
    int windowX = (screenWidth - windowWidth) / 2;
    int windowY = (screenHeight - windowHeight) / 2;

    HWND hwnd = CreateWindowExW(
        0,
        L"TreeViewApp",
        L"File Tree with Checkboxes",
        WS_OVERLAPPEDWINDOW,
        windowX, windowY, windowWidth, windowHeight,
        NULL, NULL, hInstance, NULL);

    if (!hwnd) {
        CoUninitialize();
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // Uninitialize COM
    CoUninitialize();

    return (int)msg.wParam;
}

std::wstring SelectFolder(HWND hwnd) {
    std::wstring folderPath;
    IFileDialog* pfd = NULL;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
    if (SUCCEEDED(hr)) {
        DWORD dwOptions;
        hr = pfd->GetOptions(&dwOptions);
        if (SUCCEEDED(hr)) {
            hr = pfd->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
            if (SUCCEEDED(hr)) {
                // Set the initial folder to basePath
                IShellItem* psiFolder;
                hr = SHCreateItemFromParsingName(basePath.c_str(), NULL, IID_PPV_ARGS(&psiFolder));
                if (SUCCEEDED(hr)) {
                    pfd->SetFolder(psiFolder);
                    psiFolder->Release();
                }

                hr = pfd->Show(hwnd);
                if (SUCCEEDED(hr)) {
                    IShellItem* psiResult;
                    hr = pfd->GetResult(&psiResult);
                    if (SUCCEEDED(hr)) {
                        PWSTR pszPath = NULL;
                        hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
                        if (SUCCEEDED(hr)) {
                            folderPath = pszPath;
                            CoTaskMemFree(pszPath);
                        }
                        psiResult->Release();
                    }
                }
            }
        }
        pfd->Release();
    }
    return folderPath;
}

void DisableCheckboxForParentItems(HWND hTreeView, HTREEITEM hItem) {
    TVITEM tvi = { 0 };
    tvi.mask = TVIF_STATE | TVIF_HANDLE;
    tvi.hItem = hItem;
    tvi.stateMask = TVIS_STATEIMAGEMASK;

    // Устанавливаем "пустое" состояние для чекбокса (0 - отсутствует иконка чекбокса)
    tvi.state = INDEXTOSTATEIMAGEMASK(0);
    TreeView_SetItem(hTreeView, &tvi);
}

void AddItemsToTree(HWND hTree, HTREEITEM hParent, const std::wstring& path) {
    WIN32_FIND_DATAW fd;
    HANDLE hFind = INVALID_HANDLE_VALUE;

    std::wstring searchPath = path + L"\\*";

    hFind = FindFirstFileW(searchPath.c_str(), &fd);

    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do {
        if (wcscmp(fd.cFileName, L".") != 0 && wcscmp(fd.cFileName, L"..") != 0) {
            TVINSERTSTRUCTW tvis = {};
            tvis.hParent = hParent;
            tvis.hInsertAfter = TVI_LAST;
            tvis.item.mask = TVIF_TEXT | TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
            tvis.item.pszText = fd.cFileName;
            tvis.item.cchTextMax = lstrlenW(fd.cFileName);

            std::wstring fullPath = path + L'\\' + fd.cFileName;

            // Variable for storing the icon index
            int imageIndex = 0;

            // Get the icon
            SHFILEINFOW sfi = {};
            SHGetFileInfoW(fullPath.c_str(), fd.dwFileAttributes, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON);

            // Add padding to the icon
            if (sfi.hIcon) {
                HICON hPaddedIcon = CreatePaddedIcon(sfi.hIcon, 4); // Adjust padding as needed
                imageIndex = ImageList_AddIcon(hImageList, hPaddedIcon);
                DestroyIcon(sfi.hIcon);
                DestroyIcon(hPaddedIcon);
            }

            tvis.item.iImage = imageIndex;
            tvis.item.iSelectedImage = imageIndex;

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                tvis.item.cChildren = 1;
                HTREEITEM hItem = TreeView_InsertItem(hTree, &tvis);
                DisableCheckboxForParentItems(hTreeView, hItem);
                AddItemsToTree(hTree, hItem, fullPath);
            } else {
                tvis.item.cChildren = 0;
                TreeView_InsertItem(hTree, &tvis);
            }
        }
    } while (FindNextFileW(hFind, &fd) != 0);

    FindClose(hFind);
}

void CollectCheckedFiles(HWND hTree, HTREEITEM hItem, const std::wstring& path) {
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
        if (!(attr & FILE_ATTRIBUTE_DIRECTORY)) {
            selectedFiles.push_back(fullPath);
        }
    }

    HTREEITEM hChild = TreeView_GetChild(hTree, hItem);
    while (hChild) {
        CollectCheckedFiles(hTree, hChild, fullPath);
        hChild = TreeView_GetNextSibling(hTree, hChild);
    }
}

void CopySelectedFilesToClipboard(HWND hwnd) {
    selectedFiles.clear();

    HTREEITEM hRoot = TreeView_GetRoot(hTreeView);
    while (hRoot) {
        CollectCheckedFiles(hTreeView, hRoot, basePath);
        hRoot = TreeView_GetNextSibling(hTreeView, hRoot);
    }

    if (selectedFiles.empty()) {
        MessageBoxW(hwnd, L"No files selected.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    std::wstring clipboardText;

    for (const auto& filePath : selectedFiles) {
        clipboardText += L"------------------\r\n";
        // Get relative path
        std::wstring relativePath = L"." + filePath.substr(basePath.length());
        clipboardText += relativePath + L"\r\n";
        clipboardText += L"------------------\r\n\r\n";

        // Read file content
        HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD fileSize = GetFileSize(hFile, NULL);
            if (fileSize != INVALID_FILE_SIZE) {
                std::vector<char> buffer(fileSize + 1, 0);
                DWORD bytesRead = 0;
                ReadFile(hFile, buffer.data(), fileSize, &bytesRead, NULL);
                // Assume UTF-8 encoding; adjust if necessary
                std::wstring fileContent;
                int len = MultiByteToWideChar(CP_UTF8, 0, buffer.data(), bytesRead, NULL, 0);
                if (len > 0) {
                    fileContent.resize(len);
                    MultiByteToWideChar(CP_UTF8, 0, buffer.data(), bytesRead, &fileContent[0], len);
                }
                clipboardText += fileContent + L"\r\n\r\n\r\n\r\n";
            }
            CloseHandle(hFile);
        }
    }

    // Copy to clipboard
    if (OpenClipboard(hwnd)) {
        EmptyClipboard();

        size_t dataSize = (clipboardText.length() + 1) * sizeof(wchar_t);
        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, dataSize);
        if (hGlobal) {
            LPVOID pGlobal = GlobalLock(hGlobal);
            memcpy(pGlobal, clipboardText.c_str(), dataSize);
            GlobalUnlock(hGlobal);

            SetClipboardData(CF_UNICODETEXT, hGlobal);
        }
        CloseClipboard();
    } else {
        MessageBoxW(hwnd, L"Cannot open clipboard.", L"Error", MB_OK | MB_ICONERROR);
    }
}

HICON CreatePaddedIcon(HICON hOriginalIcon, int padding) {
    ICONINFO iconInfo = {};
    GetIconInfo(hOriginalIcon, &iconInfo);

    BITMAP bmp = {};
    GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bmp);

    int newWidth = bmp.bmWidth + padding;
    int height = bmp.bmHeight;

    HBITMAP hNewBitmap = CreateBitmap(newWidth, height, bmp.bmPlanes, bmp.bmBitsPixel, NULL);
    HBITMAP hNewMask = CreateBitmap(newWidth, height, bmp.bmPlanes, bmp.bmBitsPixel, NULL);

    HDC hDC = GetDC(NULL);
    HDC hMemDC = CreateCompatibleDC(hDC);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hNewBitmap);

    // Fill the new bitmap with transparency
    BLENDFUNCTION bf = {AC_SRC_OVER, 0, 0, AC_SRC_ALPHA};
    GdiAlphaBlend(hMemDC, 0, 0, newWidth, height, hMemDC, 0, 0, 0, 0, bf);

    // Draw the original icon at the offset position
    DrawIconEx(hMemDC, padding, 0, hOriginalIcon, bmp.bmWidth, height, 0, NULL, DI_NORMAL);

    SelectObject(hMemDC, hOldBitmap);
    DeleteDC(hMemDC);
    ReleaseDC(NULL, hDC);

    ICONINFO newIconInfo = {};
    newIconInfo.fIcon = TRUE;
    newIconInfo.hbmColor = hNewBitmap;
    newIconInfo.hbmMask = hNewMask;

    HICON hPaddedIcon = CreateIconIndirect(&newIconInfo);

    // Clean up
    DeleteObject(iconInfo.hbmColor);
    DeleteObject(iconInfo.hbmMask);
    DeleteObject(hNewBitmap);
    DeleteObject(hNewMask);

    return hPaddedIcon;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // Create the image list for icons
            hImageList = ImageList_Create(20, 16, ILC_COLOR32 | ILC_MASK, 1, 1); // Создание списка изображений

            // Create the tree view
            hTreeView = CreateWindowExW(
                WS_EX_CLIENTEDGE, WC_TREEVIEWW, L"",
                WS_VISIBLE | WS_CHILD | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_CHECKBOXES | TVS_DISABLEDRAGDROP,
                0, 0, 600, 320,
                hwnd, (HMENU)1, hInst, NULL);

            TreeView_SetImageList(hTreeView, hImageList, TVSIL_NORMAL); // Привязка списка к дереву

            // Создаём меню
            HMENU hMenuBar = CreateMenu();
            HMENU hFileMenu = CreateMenu();

            AppendMenuW(hFileMenu, MF_STRING, IDM_SELECT_FOLDER, L"Select folder...");

            AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, L"File");

            SetMenu(hwnd, hMenuBar);

            // Create the "Copy" button
            hButton = CreateWindowW(L"BUTTON", L"Copy", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                    10, 380, 580, 30, hwnd, (HMENU)2, hInst, NULL);

            // Initialize the tree view with the current directory
            wchar_t currentPath[MAX_PATH];
            GetCurrentDirectoryW(MAX_PATH, currentPath);

            basePath = currentPath;
            AddItemsToTree(hTreeView, NULL, basePath);

            break;
        }

        case WM_SIZE: {
            int width = LOWORD(lParam); // Получаем новую ширину окна
            int height = HIWORD(lParam); // Получаем новую высоту окна

            // Изменяем размер дерева
            MoveWindow(hTreeView, 0, 0, width, height - 50, TRUE);

            // Изменяем положение и размер кнопки "Copy"
            MoveWindow(hButton, 10, height - 40, width - 20, 30, TRUE);

            break;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == 2) {
                // "Copy" button
                CopySelectedFilesToClipboard(hwnd);
            } else if (LOWORD(wParam) == IDM_SELECT_FOLDER) {
                // "Select Folder" menu item
                std::wstring selectedPath = SelectFolder(hwnd);
                if (!selectedPath.empty()) {
                    // Clear the tree view and add items from the selected folder
                    TreeView_DeleteAllItems(hTreeView);

                    basePath = selectedPath;
                    AddItemsToTree(hTreeView, NULL, selectedPath);
                }
            }
            break;

        case WM_DESTROY:
            if (hImageList)
                ImageList_Destroy(hImageList); // Уничтожаем список изображений
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}
