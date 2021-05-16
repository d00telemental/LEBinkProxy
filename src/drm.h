#pragma once
#include "dllincludes.h"
#include "io.h"
#include "proxy_info.h"

// DRM handling utilities.
namespace DRM {
    int GoodTitleHit = 0;
    const int RequiredHits = 1;

    static BOOL CALLBACK enumWindowCallback(HWND hWnd, LPARAM lparam)
    {
        int length = GetWindowTextLength(hWnd);
        wchar_t buffer[512];
        GetWindowText(hWnd, buffer, length + 1);

        if (GoodTitleHit == RequiredHits)
        {
            GLogger.writeFormatLine(L"WaitForDenuvo: hit enough times!");
            return FALSE;
        }

        if (IsWindowVisible(hWnd) && length != 0 && wcsstr(buffer, L"Mass Effect") != NULL)
        {
            //wprintf_s(L"  Window: %s\n", buffer);
            if (0 == wcscmp(GAppProxyInfo.WinTitle, buffer))
            {
                ++GoodTitleHit;
            }
        }

        return TRUE;
    }

    void WaitForFuckingDenuvo()
    {
        GLogger.writeFormatLine(L"WaitForDenuvo: exe path = %s", GAppProxyInfo.ExePath);
        GLogger.writeFormatLine(L"WaitForDenuvo: exe name = %s", GAppProxyInfo.ExeName);
        GLogger.writeFormatLine(L"WaitForDenuvo: win title = %s", GAppProxyInfo.WinTitle);

        GLogger.writeFormatLine(L"WaitForDenuvo: waiting for DRM...");
        do
        {
            EnumWindows(enumWindowCallback, NULL);
        } while (GoodTitleHit != RequiredHits);
        GLogger.writeFormatLine(L"WaitForDenuvo: finished waiting for DRM!");
    }
}