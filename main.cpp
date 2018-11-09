#include <iostream>
#include <windows.h>
#include <dirent.h>
#include <string>
#include "winregion.h"
#include "lua.h"

#define ADD_NUMBER_CONSTANT(c) lua_pushnumber(L, c); lua_setfield(L, -2, #c);

    int lua_MouseAction(lua_State* L)
    {
        mouse_event(lua_tonumber(L, 1), lua_tonumber(L, 2), lua_tonumber(L, 3), 0, 0);
        return 0;
    }

    int lua_MessageBox(lua_State* L)
    {
        lua_pushnumber(L, MessageBox((HWND)lua_topointer(L, 1), luaL_optstring(L, 2, ""), luaL_optstring(L, 3, ""), luaL_optinteger(L, 4, 0)));
        return 1;
    }

    int lua_KeyAction(lua_State* L)
    {
        keybd_event(lua_tonumber(L, 1), 0, lua_tonumber(L, 2), 0);
        return 0;
    }

    int lua_MouseClip(lua_State* L)
    {
        RECT Rect;
        Rect.left = lua_tonumber(L, 1);
        Rect.top = lua_tonumber(L, 2);
        Rect.right = Rect.left + lua_tonumber(L, 3);
        Rect.bottom = Rect.top + lua_tonumber(L, 4);
        lua_pushboolean(L, ClipCursor(&Rect));
        return 1;
    }

    int lua_DirectoryFilesWin(lua_State* L)
    {
        std::string path(lua_tostring(L, 1));

        path += lua_isstring(L, 2) ? lua_tostring(L, 2) : "\\*";

        WIN32_FIND_DATA ffd;
        HANDLE hFind = FindFirstFile(path.c_str(), &ffd);

        if (hFind == INVALID_HANDLE_VALUE)
            return 0;

        lua_newtable(L);
        int index = 1;

        do
        {
            path = ffd.cFileName;

            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                path += '/';

            lua_pushstring(L, path.c_str());
            lua_rawseti(L, -2, index++);
        }
        while (FindNextFile(hFind, &ffd) != 0);

        FindClose(hFind);
        return 1;
    }

    int lua_DirectoryFiles(lua_State* L)
    {
        lua_newtable(L);
        DIR* dir = opendir(lua_tostring(L, 1));
        if (!dir)
            return 0;
        int index = 1;
        struct dirent* ent;
        while ((ent = readdir(dir)) != NULL)
        {
            lua_pushstring(L, ent->d_name);
            lua_rawseti(L, -2, index++);
        }
        closedir(dir);
        return 1;
    }

    int lua_SetSystemTime(lua_State* L)
    {
        if (!lua_istable(L, 1))
            return 0;
        SYSTEMTIME time;
        GetLocalTime(&time);

        lua_getfield(L, 1, "year");
        lua_pop(L, 1);
        if (lua_isnumber(L, 0))
            time.wYear = lua_tonumber(L, 0);

        lua_getfield(L, 1, "month");
        lua_pop(L, 1);
        if (lua_isnumber(L, 0))
            time.wMonth = lua_tonumber(L, 0);

        lua_getfield(L, 1, "dayofweek");
        lua_pop(L, 1);
        if (lua_isnumber(L, 0))
            time.wDayOfWeek = lua_tonumber(L, 0);

        lua_getfield(L, 1, "day");
        lua_pop(L, 1);
        if (lua_isnumber(L, 0))
            time.wDay = lua_tonumber(L, 0);

        lua_getfield(L, 1, "hour");
        lua_pop(L, 1);
        if (lua_isnumber(L, 0))
            time.wHour = lua_tonumber(L, 0);

        SetLocalTime(&time);
        return 0;
    }

    int lua_OpenFileDialog(lua_State* L)
    {
        OPENFILENAME ofn;
        char szFile[260];
        HWND hwnd = (HWND)lua_topointer(L, 1);

        if (!hwnd && lua_isnumber(L, 1))
            hwnd = (HWND)lua_tointeger(L, 1);

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFile = szFile;
        ofn.lpstrFile[0] = '\0';
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = luaL_optstring(L, 2, "All\0*.*\0\0");
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
        if (lua_toboolean(L, 3))
            ofn.Flags |= OFN_ALLOWMULTISELECT;

        if (GetOpenFileName(&ofn))
        {
            if (!ofn.nFileExtension)
            {
                unsigned index = 1, i;
                lua_newtable(L);
                lua_pushstring(L, ofn.lpstrFile);
                lua_rawseti(L, -2, index++);
                for (i = 0; i < ofn.nMaxFile - 1; i++)
                {
                    if (!ofn.lpstrFile[i])
                    {
                        if (!ofn.lpstrFile[i + 1])
                            break;
                        lua_pushstring(L, ofn.lpstrFile + i + 1);
                        lua_rawseti(L, -2, index++);
                    }
                }
                return 1;
            }
            lua_pushstring(L, ofn.lpstrFile);
            return 1;
        }
        return 0;
    }

    int lua_SaveFileDialog(lua_State* L)
    {
        OPENFILENAME ofn;
        char szFile[260];
        HWND hwnd = (HWND)lua_topointer(L, 1);

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFile = szFile;
        ofn.lpstrFile[0] = '\0';
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = luaL_optstring(L, 2, "All\0*.*\0\0");
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_EXPLORER;

        if (GetSaveFileName(&ofn))
        {
            lua_pushstring(L, ofn.lpstrFile);
            return 1;
        }

        return 0;
    }

    int lua_SetWindowSplash(lua_State* L)
    {
        if (!lua_islightuserdata(L, 1) || !lua_islightuserdata(L, 2) ||
            !lua_isnumber(L, 3) || !lua_isnumber(L, 4))
            return 0;
        unsigned width = lua_tointeger(L, 3);
        unsigned height = lua_tointeger(L, 4);
        if (width <= 0 || height <= 0)
            return 0;
        HRGN Region = CreateRectRgn(0, 0, width, height);
        unsigned char* data_bgn = (unsigned char*)lua_topointer(L, 2);
        unsigned char* data_end = data_bgn + width * height * 4;
        unsigned x = 0, y = 1;
        while (data_bgn < data_end)
        {
            x++;
            if (x > width)
            {
                x = 1;
                y++;
            }
            unsigned char A = *(data_bgn + 3);
            data_bgn += 4;
            if (!A)
            {
                HRGN TempRegion = CreateRectRgn(x - 1, y - 1, x, y);
                CombineRgn(Region, Region, TempRegion, RGN_XOR);
                DeleteObject(TempRegion);
            }
        }
        SetWindowRgn((HWND)lua_topointer(L, 1), Region, true);
        lua_pushboolean(L, true);
        return 1;
    }

    int lua_FindWindow(lua_State* L)
    {
        HWND window = FindWindow(NULL, lua_tostring(L, 1));
        if (window)
            lua_pushlightuserdata(L, window);
        else
            return 0;
        return 1;
    }

    BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM param)
    {
        if (!IsWindowVisible(hwnd))
            return TRUE;
        lua_State* L = (lua_State*)param;
        char title[80];
        GetWindowText(hwnd, title, sizeof(title));
        lua_pushlightuserdata(L, hwnd);
        lua_setfield(L, -2, title);
        return TRUE;
    }

    int lua_GetOpenWindows(lua_State* L)
    {
        lua_newtable(L);
        EnumWindows(EnumWindowsProc, (LPARAM)L);
        return 1;
    }

    int lua_CaptureWindow(lua_State* L)
    {
        bool success = true;
        std::string error;
        HWND hWnd = (HWND)lua_topointer(L, 1);
        if (lua_isnil(L, 1))
            hWnd = NULL;
        HDC hdcWindow;
        HDC hdcMemDC = NULL;
        HBITMAP hbmScreen = NULL;
        BITMAP bmpScreen;
        DWORD dwBytesWritten;
        DWORD dwSizeofDIB;
        HANDLE hFile;
        char *lpbitmap;
        HANDLE hDIB;
        DWORD dwBmpSize;

        hdcWindow = GetDC(hWnd);

        // Create a compatible DC which is used in a BitBlt from the window DC
        hdcMemDC = CreateCompatibleDC(hdcWindow);

        if(!hdcMemDC)
        {
            error = "CreateCompatibleDC has failed.";
            success = false;
            goto done;
        }

        // Get the client area for size calculation
        RECT rcClient;
        if (!lua_isnil(L, 1))
            GetClientRect(hWnd, &rcClient);
        else
        {
            rcClient.top = rcClient.left = 0;
            rcClient.right = GetSystemMetrics(SM_CXSCREEN);
            rcClient.bottom = GetSystemMetrics(SM_CYSCREEN);
        }

        // Create a compatible bitmap from the Window DC
        hbmScreen = CreateCompatibleBitmap(hdcWindow, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top);

        if(!hbmScreen)
        {
            error = "CreateCompatibleBitmap failed.";
            success = false;
            goto done;
        }

        // Select the compatible bitmap into the compatible memory DC.
        SelectObject(hdcMemDC, hbmScreen);

        // Bit block transfer into our compatible memory DC.
        if(!BitBlt(hdcMemDC,
                   0,0,
                   rcClient.right-rcClient.left, rcClient.bottom-rcClient.top,
                   hdcWindow,
                   0,0,
                   SRCCOPY))
        {
            error = "BitBlt has failed.";
            success = false;
            goto done;
        }

        // Get the BITMAP from the HBITMAP
        GetObject(hbmScreen, sizeof(BITMAP), &bmpScreen);

        BITMAPFILEHEADER   bmfHeader;
        BITMAPINFOHEADER   bi;

        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = bmpScreen.bmWidth;
        bi.biHeight = bmpScreen.bmHeight;
        bi.biPlanes = 1;
        bi.biBitCount = 32;
        bi.biCompression = BI_RGB;
        bi.biSizeImage = 0;
        bi.biXPelsPerMeter = 0;
        bi.biYPelsPerMeter = 0;
        bi.biClrUsed = 0;
        bi.biClrImportant = 0;

        dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;

        // Starting with 32-bit Windows, GlobalAlloc and LocalAlloc are implemented as wrapper functions that
        // call HeapAlloc using a handle to the process's default heap. Therefore, GlobalAlloc and LocalAlloc
        // have greater overhead than HeapAlloc.
        hDIB = GlobalAlloc(GHND,dwBmpSize);
        lpbitmap = (char *)GlobalLock(hDIB);

        // Gets the "bits" from the bitmap and copies them into a buffer
        // which is pointed to by lpbitmap.
        GetDIBits(hdcWindow, hbmScreen, 0,
            (UINT)bmpScreen.bmHeight,
            lpbitmap,
            (BITMAPINFO *)&bi, DIB_RGB_COLORS);

        // A file is created, this is where we will save the screen capture.
        hFile = CreateFile(luaL_optstring(L, 2, "capture.bmp"),
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);

        // Add the size of the headers to the size of the bitmap to get the total file size
        dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

        //Offset to where the actual bitmap bits start.
        bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);

        //Size of the file
        bmfHeader.bfSize = dwSizeofDIB;

        //bfType must always be BM for Bitmaps
        bmfHeader.bfType = 0x4D42; //BM

        dwBytesWritten = 0;
        WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
        WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
        WriteFile(hFile, (LPSTR)lpbitmap, dwBmpSize, &dwBytesWritten, NULL);

        //Unlock and Free the DIB from the heap
        GlobalUnlock(hDIB);
        GlobalFree(hDIB);

        //Close the handle for the file that was created
        CloseHandle(hFile);

        //Clean up
        done:
        DeleteObject(hbmScreen);
        DeleteObject(hdcMemDC);
        //ReleaseDC(NULL,hdcScreen);
        ReleaseDC(hWnd,hdcWindow);

        if (!success)
        {
            lua_pushnil(L);
            lua_pushstring(L, error.c_str());
            return 2;
        }

        lua_pushboolean(L, true);
        return 1;
    }

    int lua_HideFromTaskbar(lua_State* L)
    {
        HWND hWnd = (HWND)lua_topointer(L, 1);
        ShowWindow(hWnd, SW_HIDE);
        SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_TOOLWINDOW);
        ShowWindow(hWnd, SW_SHOW);
        return 0;
    }

    int lua_SetTopMost(lua_State* L)
    {
        SetWindowPos((HWND)lua_topointer(L, 1), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        return 0;
    }

    int lua_SystemCall(lua_State* L)
    {
        lua_pushnumber(L, system(lua_tostring(L, 1)));
        return 1;
    }

    int lua_IsDirectory(lua_State* L)
    {
        DWORD att = GetFileAttributes(lua_tostring(L, 1));
        if (att == INVALID_FILE_ATTRIBUTES)
            return 0;
        lua_pushboolean(L, att & FILE_ATTRIBUTE_DIRECTORY);
        return 1;
    }

    int lua_MakeDirectory(lua_State* L)
    {
        lua_pushboolean(L, CreateDirectory(lua_tostring(L, 1), NULL));
        return 1;
    }

    int lua_SetWindowParent(lua_State* L)
    {
        SetParent((HWND)lua_topointer(L, 1), (HWND)lua_topointer(L, 2));
        return 0;
    }

    int lua_IsWindowVisible(lua_State* L)
    {
        lua_pushboolean(L, IsWindowVisible((HWND)lua_topointer(L, 1)));
        return 1;
    }

    int lua_GetWindowParent(lua_State* L)
    {
        lua_pushlightuserdata(L, GetParent((HWND)lua_topointer(L, 1)));
        return 1;
    }

    int lua_CreateSubWindow(lua_State* L)
    {
        HWND sub = CreateWindow(TEXT("STATIC"), NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                                luaL_optnumber(L, 2, 0),  luaL_optnumber(L, 3, 0),
                                luaL_optnumber(L, 4, 100), luaL_optnumber(L, 5, 100),
                                (HWND)lua_topointer(L, 1), NULL,
                                (HINSTANCE)GetWindowLong((HWND)lua_topointer(L, 1),  GWL_HINSTANCE), NULL);
        lua_pushlightuserdata(L, sub);
        return 1;
    }

    int lua_SendMessage(lua_State* L)
    {
        SendMessage((HWND)lua_topointer(L, 1), lua_tointeger(L, 2), (WPARAM)lua_tointeger(L, 3), (LPARAM)lua_tointeger(L, 4));
        return 0;
    }

    int lua_GetWindowText(lua_State* L)
    {
        char buffer[256]; buffer[0] = '\0';
        GetWindowText((HWND)lua_topointer(L, 1), buffer, sizeof(buffer));
        lua_pushstring(L, buffer);
        return 1;
    }

    int lua_GetForegroundWindow(lua_State* L)
    {
        lua_pushlightuserdata(L, GetForegroundWindow());
        return 1;
    }

    int lua_SetForegroundWindow(lua_State* L)
    {
        SetForegroundWindow((HWND)lua_topointer(L, 1));
        return 0;
    }

    int lua_GetWindowSize(lua_State* L)
    {
        RECT rect;
        HWND window = (HWND)lua_topointer(L, 1);
        GetWindowRect(window, &rect);
        lua_pushnumber(L, rect.right - rect.left);
        lua_pushnumber(L, rect.bottom - rect.top);
        return 2;
    }

    int lua_GetDesktopWindow(lua_State* L)
    {
        lua_pushlightuserdata(L, GetDesktopWindow());
        return 1;
    }

    int lua_SetWindowExtendedStyle(lua_State* L)
    {
        HWND window = (HWND)lua_topointer(L, 1);
        lua_pushnumber(L, SetWindowLong(window, GWL_EXSTYLE, (GetWindowLong(window, GWL_EXSTYLE) |
                      luaL_optinteger(L, 2, 0)) &
                      (~luaL_optinteger(L, 3, 0))));
        return 1;
    }

    int lua_Execute(lua_State* L)
    {
        lua_pushnumber(L, (int)ShellExecuteA(NULL, "open", lua_tostring(L, 1), luaL_optstring(L, 2, NULL),
                                             luaL_optstring(L, 3, NULL), luaL_optinteger(L, 4, SW_HIDE)));
        //WinExec(lua_tostring(L, 1), luaL_optint(L, 2, SW_HIDE));
        return 1;
    }

    int lua_SetClipboardText(lua_State* L)
    {
        const char* text = luaL_optstring(L, 1, "");
        const size_t len = lua_rawlen(L, 1) + 1;
        HGLOBAL hMem =  GlobalAlloc(GMEM_MOVEABLE, len);

        if (!hMem)
            return 0;

        LPVOID lockedPtr = GlobalLock(hMem);

        if (lockedPtr)
            memcpy(lockedPtr, text, len);
        else
            return 0;

        GlobalUnlock(hMem);

        if (!OpenClipboard(NULL))
            return 0;

        if (!EmptyClipboard() || !SetClipboardData(CF_TEXT, hMem))
        {
            CloseClipboard();
            return 0;
        }

        CloseClipboard();
        lua_pushboolean(L, true);
        return 1;
    }

    int lua_GetClipboardText(lua_State* L)
    {

        if (!OpenClipboard(NULL))
            return 0;

        if (IsClipboardFormatAvailable(CF_TEXT))
        {
            HGLOBAL global = GetClipboardData(CF_TEXT);
            LPTSTR ptr = static_cast<char*>(GlobalLock(global));

            if (ptr)
            {
                lua_pushstring(L, ptr);
                GlobalUnlock(global);
            }
            else
                lua_pushnil(L);
        }

        CloseClipboard();
        return 1;
    }

    int lua_ShowWindow(lua_State* L)
    {
        HWND hWnd = (HWND)lua_topointer(L, 1);
        ShowWindow(hWnd, luaL_optinteger(L, 2, SW_SHOW));
        return 0;
    }

extern "C"
{
    LUA_API lua_CFunction luaopen_libwin(lua_State* L)
    {
        lua_newtable(L);

            lua_pushcfunction(L, lua_MessageBox);
            lua_setfield(L, -2, "messagebox");

            lua_pushcfunction(L, lua_MouseAction);
            lua_setfield(L, -2, "mouseaction");

            lua_pushcfunction(L, lua_MouseClip);
            lua_setfield(L, -2, "mouseclip");

            lua_pushcfunction(L, lua_KeyAction);
            lua_setfield(L, -2, "keyaction");

            lua_pushcfunction(L, lua_DirectoryFiles);
            lua_setfield(L, -2, "_dirlist");

            lua_pushcfunction(L, lua_DirectoryFilesWin);
            lua_setfield(L, -2, "dirlist");

            lua_pushcfunction(L, lua_SetSystemTime);
            lua_setfield(L, -2, "setsystime");

            lua_pushcfunction(L, lua_OpenFileDialog);
            lua_setfield(L, -2, "openfiledialog");

            lua_pushcfunction(L, lua_SaveFileDialog);
            lua_setfield(L, -2, "savefiledialog");

            lua_pushcfunction(L, lua_SetWindowSplash);
            lua_setfield(L, -2, "setsplash");

            lua_pushcfunction(L, lua_FindWindow);
            lua_setfield(L, -2, "findwindow");

            lua_pushcfunction(L, lua_GetOpenWindows);
            lua_setfield(L, -2, "listwindows");

            lua_pushcfunction(L, lua_CaptureWindow);
            lua_setfield(L, -2, "capturewindow");

            lua_pushcfunction(L, lua_HideFromTaskbar);
            lua_setfield(L, -2, "taskbarhide");

            lua_pushcfunction(L, lua_SetTopMost);
            lua_setfield(L, -2, "settopmost");

            lua_pushcfunction(L, lua_SystemCall);
            lua_setfield(L, -2, "system");

            lua_pushcfunction(L, lua_IsDirectory);
            lua_setfield(L, -2, "isdir");

            lua_pushcfunction(L, lua_MakeDirectory);
            lua_setfield(L, -2, "mkdir");

            lua_pushcfunction(L, lua_SetWindowParent);
            lua_setfield(L, -2, "setwindowparent");

            lua_pushcfunction(L, lua_IsWindowVisible);
            lua_setfield(L, -2, "iswindowvisible");

            lua_pushcfunction(L, lua_GetWindowParent);
            lua_setfield(L, -2, "getwindowparent");

            lua_pushcfunction(L, lua_CreateSubWindow);
            lua_setfield(L, -2, "createsubwindow");

            lua_pushcfunction(L, lua_SendMessage);
            lua_setfield(L, -2, "sendmessage");

            lua_pushcfunction(L, lua_GetWindowText);
            lua_setfield(L, -2, "getwindowtext");

            lua_pushcfunction(L, lua_GetForegroundWindow);
            lua_setfield(L, -2, "getforegroundwindow");

            lua_pushcfunction(L, lua_SetForegroundWindow);
            lua_setfield(L, -2, "SetForegroundWindow");

            lua_pushcfunction(L, lua_GetWindowSize);
            lua_setfield(L, -2, "getwindowsize");

            lua_pushcfunction(L, lua_GetDesktopWindow);
            lua_setfield(L, -2, "getdesktopwindow");

            lua_pushcfunction(L, lua_SetClipboardText);
            lua_setfield(L, -2, "setclipboardtext");

            lua_pushcfunction(L, lua_GetClipboardText);
            lua_setfield(L, -2, "getclipboardtext");

            lua_pushcfunction(L, lua_ShowWindow);
            lua_setfield(L, -2, "showwindow");

            lua_pushcfunction(L, lua_SetWindowExtendedStyle);
            lua_setfield(L, -2, "SetWindowExtendedStyle");
            ADD_NUMBER_CONSTANT(WS_EX_NOACTIVATE);
            ADD_NUMBER_CONSTANT(WS_EX_APPWINDOW);

            lua_pushcfunction(L, lua_Execute);
            lua_setfield(L, -2, "execute");

            ADD_NUMBER_CONSTANT(MB_YESNO);
            ADD_NUMBER_CONSTANT(MB_YESNOCANCEL);
            ADD_NUMBER_CONSTANT(IDYES);
            ADD_NUMBER_CONSTANT(IDNO);
            ADD_NUMBER_CONSTANT(IDCANCEL);
            ADD_NUMBER_CONSTANT(WM_KEYDOWN);
            ADD_NUMBER_CONSTANT(WM_CHAR);

            lua_register_winreg(L);

        lua_setglobal(L, "libwin");
        return 0;
    }
}
