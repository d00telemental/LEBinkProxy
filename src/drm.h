#pragma once
#include "dllincludes.h"
#include "io.h"

// DRM handling utilities.
namespace DRM {
    wchar_t ExePath[MAX_PATH];
    wchar_t ExeName[MAX_PATH];
    wchar_t WinTitle[MAX_PATH];
    int GoodTitleHit = 0;
    const int RequiredHits = 1;

    static BOOL CALLBACK enumWindowCallback(HWND hWnd, LPARAM lparam)
    {
        int length = GetWindowTextLength(hWnd);
        wchar_t buffer[512];
        GetWindowText(hWnd, buffer, length + 1);

        if (GoodTitleHit == RequiredHits)
        {
            IO::GLogger.writeFormatLine(L"WaitForDenuvo: hit enough times!");
            return FALSE;
        }

        if (IsWindowVisible(hWnd) && length != 0 && wcsstr(buffer, L"Mass Effect") != NULL)
        {
            //wprintf_s(L"  Window: %s\n", buffer);
            if (0 == wcscmp(WinTitle, buffer))
            {
                ++GoodTitleHit;
            }
        }

        return TRUE;
    }

    void StripPathFromFileName(wchar_t* path, wchar_t* newPath)
    {
        auto selectionStart = path;
        while (*path != L'\0')
        {
            if (*path == L'\\')
            {
                //IO::GLogger.writeFormatLine(L"   %s", path);
                selectionStart = path;
            }
            path++;
        }
        
        wcscpy(newPath, selectionStart + 1);
    }

    void AssociateWindowTitle(wchar_t* exeName, wchar_t* winTitle)
    {
        if (0 == wcscmp(exeName, L"MassEffect1.exe")) wcscpy(winTitle, L"Mass Effect");
        else if (0 == wcscmp(exeName, L"MassEffect2.exe")) wcscpy(winTitle, L"Mass Effect 2");
        else if (0 == wcscmp(exeName, L"MassEffect3.exe")) wcscpy(winTitle, L"Mass Effect 3");
        else
        {
            IO::GLogger.writeFormatLine(L"WaitForDenuvo: UNSUPPORTED EXE NAME %s", exeName);
            exit(-1);
        }
    }

    void WaitForFuckingDenuvo()
    {
        GetModuleFileNameW(NULL, ExePath, MAX_PATH);
        StripPathFromFileName(ExePath, ExeName);
        AssociateWindowTitle(ExeName, WinTitle);

        IO::GLogger.writeFormatLine(L"WaitForDenuvo: exe path = %s", ExePath);
        IO::GLogger.writeFormatLine(L"WaitForDenuvo: exe name = %s", ExeName);
        IO::GLogger.writeFormatLine(L"WaitForDenuvo: win title = %s", WinTitle);

        IO::GLogger.writeFormatLine(L"WaitForDenuvo: waiting for DRM...");
        do
        {
            EnumWindows(enumWindowCallback, NULL);
        } while (GoodTitleHit != RequiredHits);
        IO::GLogger.writeFormatLine(L"WaitForDenuvo: finished waiting for DRM!");
    }
}
