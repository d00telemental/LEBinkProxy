#pragma once

#include <cstdio>
#include <cstring>
#include <windows.h>

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
            printf("WaitForDenuvo: hit enough times!\n");
            return FALSE;
        }

        if (IsWindowVisible(hWnd) && length != 0 && wcsstr(buffer, L"Mass Effect") != NULL)
        {
            //wprintf_s(L"  Window: %s\n", buffer);
            if (0 == wcscmp(L"Mass Effect", buffer))
            {
                ++GoodTitleHit;
            }
        }

        return TRUE;
    }

    void WaitForFuckingDenuvo()
    {
        printf("WaitForDenuvo: waiting for DRM...\n");
        do
        {
            EnumWindows(enumWindowCallback, NULL);
        } while (GoodTitleHit != RequiredHits);
        printf("WaitForDenuvo: finished waiting for DRM!\n");
    }
}
