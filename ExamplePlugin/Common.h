#pragma once
#include <windows.h>
#include <cstdio>

namespace Common
{
    void OpenConsole()
    {
        AllocConsole();

        freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
        freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);

        HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo;
        GetConsoleScreenBufferInfo(console, &lpConsoleScreenBufferInfo);
        SetConsoleScreenBufferSize(console, { lpConsoleScreenBufferInfo.dwSize.X, 30000 });
    }

    void CloseConsole()
    {
        FreeConsole();
    }
}

#define writeln(msg,...) fwprintf_s(stdout, L"ExamplePlugin::" msg "\n", __VA_ARGS__)
