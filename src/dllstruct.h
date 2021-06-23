#pragma once
#include <Windows.h>

#include "conf/patterns.h"
#include "gamever.h"
#include "utils/io.h"
#include "spi/interface.h"


// Forward-declare these to avoid a cyclical header dependency.

class AsiLoaderModule;
class ConsoleEnablerModule;
class LauncherArgsModule;


struct LEBinkProxy
{
public:
    wchar_t ExePath[MAX_PATH];
    wchar_t ExeName[MAX_PATH];
    wchar_t WinTitle[MAX_PATH];
    wchar_t* CmdLine;

    LEGameVersion Game;

    AsiLoaderModule*       AsiLoader;
    ConsoleEnablerModule*  ConsoleEnabler;
    LauncherArgsModule*    LauncherArgs;

    ISharedProxyInterface* SPI;

private:
    __forceinline
    void stripExecutableName_(wchar_t* path, wchar_t* newPath)
    {
        auto selectionStart = path;
        while (*path != L'\0')
        {
            if (*path == L'\\')
            {
                selectionStart = path;
            }
            path++;
        }

        wcscpy(newPath, selectionStart + 1);
    }

    __forceinline
    void associateWindowTitle_(wchar_t* exeName, wchar_t* winTitle)
    {
        if (0 == wcscmp(exeName, LEL_ExecutableName))
        {
            wcscpy(winTitle, LEL_WindowTitle);
            Game = LEGameVersion::Launcher;
        }
        else if (0 == wcscmp(exeName, LE1_ExecutableName))
        {
            wcscpy(winTitle, LE1_WindowTitle);
            Game = LEGameVersion::LE1;
        }
        else if (0 == wcscmp(exeName, LE2_ExecutableName))
        {
            wcscpy(winTitle, LE2_WindowTitle);
            Game = LEGameVersion::LE2;
        }
        else if (0 == wcscmp(exeName, LE3_ExecutableName))
        {
            wcscpy(winTitle, LE3_WindowTitle);
            Game = LEGameVersion::LE3;
        }
        else
        {
            GLogger.writeln(L"..AssociateWindowTitle: UNSUPPORTED EXE NAME %s", exeName);
            Game = LEGameVersion::Unsupported;
            exit(-1);
        }
    }

public:
    __forceinline
    void Initialize()
    {
        CmdLine = GetCommandLineW();
        GetModuleFileNameW(NULL, ExePath, MAX_PATH);
        stripExecutableName_(ExePath, ExeName);
        associateWindowTitle_(ExeName, WinTitle);

        GLogger.writeln(L"..Initialize: cmd line = %s", CmdLine);
        GLogger.writeln(L"..Initialize: exe path = %s", ExePath);
        GLogger.writeln(L"..Initialize: exe name = %s", ExeName);
        GLogger.writeln(L"..Initialize: win title = %s", WinTitle);
    }
};

// Global instance.

static LEBinkProxy GLEBinkProxy;
