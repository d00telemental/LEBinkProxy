#pragma once
#include "dllincludes.h"
#include "event.h"
#include "io.h"

namespace Launcher
{
    static LEGameVersion GLaunchTarget = LEGameVersion::Unsupported;
    static bool GAutoTerminate = false;

    const char* GameVersionToPath(LEGameVersion version)
    {
        switch (version)
        {
        case LEGameVersion::LE1: return "../ME1/Binaries/Win64/MassEffect1.exe";
        case LEGameVersion::LE2: return "../ME2/Binaries/Win64/MassEffect2.exe";
        case LEGameVersion::LE3: return "../ME3/Binaries/Win64/MassEffect3.exe";
        default:
            return nullptr;
        }
    }
    const char* GameVersionToCwd(LEGameVersion version)
    {
        switch (version)
        {
        case LEGameVersion::LE1: return "../ME1/Binaries/Win64";
        case LEGameVersion::LE2: return "../ME2/Binaries/Win64";
        case LEGameVersion::LE3: return "../ME3/Binaries/Win64";
        default:
            return nullptr;
        }
    }

    void ParseCmdLine(wchar_t* cmdLine)
    {
        GLogger.writeFormatLine(L"ParseCmdLine: cmdLine = %s", cmdLine);

        auto startCmdLine = cmdLine;
        auto endCmdLine = startCmdLine + wcslen(startCmdLine);
        auto startWCPtr = wcsstr(startCmdLine, L"-game=");

        if (startWCPtr != nullptr && (size_t)(startWCPtr + 6) < (size_t)endCmdLine)
        {
            GLogger.writeFormatLine(L"ParseCmdLine: startWCPtr = %s", startWCPtr);

            auto numStrWCPtr = startWCPtr + 6;
            auto gameNum = wcstol(numStrWCPtr, nullptr, 10);

            if (gameNum >= 1 && gameNum <= 3)
            {
                GLogger.writeFormatLine(L"ParseCmdLine: game selection is %d", gameNum);
                GLaunchTarget = static_cast<LEGameVersion>(gameNum);

                if (0 != wcsstr(startCmdLine, L"-autoterminate"))
                {
                    GAutoTerminate = true;
                    GLogger.writeFormatLine(L"ParseCmdLine: found -autoterminate flag, the launcher will commit suicide asap");
                }

                return;
            }

            GLogger.writeFormatLine(L"ParseCmdLine: wcstol failed to retrieve a code in [1;3]");
        }

        GLogger.writeFormatLine(L"ParseCmdLine: failed to find '-game=', startCmdLine = %p, endCmdLine = %p, startWCPtr = %p",
            startCmdLine, endCmdLine, startWCPtr);
    }

    void LaunchGame()
    {
        auto gamePath = GameVersionToPath(GLaunchTarget);
        auto gameParms = " -NoHomeDir -SeekFreeLoadingPCConsole -locale {locale} -Subtitles 20 -OVERRIDELANGUAGE=INT";
        auto gameCwd = GameVersionToCwd(GLaunchTarget);

        STARTUPINFOA si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        GLogger.writeFormatLine(L"LaunchGameAndWait: CreateProcessA: lpApplicationName = %S", gamePath);
        GLogger.writeFormatLine(L"LaunchGameAndWait: CreateProcessA: lpCommandLine = %S", gameParms);
        GLogger.writeFormatLine(L"LaunchGameAndWait: CreateProcessA: lpCurrentDirectory = %S", gameCwd);

        if (0 != CreateProcessA(gamePath, const_cast<char*>(gameParms), nullptr, nullptr, false, 0, nullptr, gameCwd, &si, &pi))
        {
            if (GAutoTerminate)
            {
                GLogger.writeFormatLine(L"LaunchGameAndWait: created a process (pid = %d), terminating the launcher...", pi.dwProcessId);
                exit(0);
            }

            GLogger.writeFormatLine(L"LaunchGameAndWait: created a process (pid = %d), waiting until it ends...", pi.dwProcessId);
            WaitForSingleObject(pi.hProcess, INFINITE);

            GLogger.writeFormatLine(L"LaunchGameAndWait: process ended, closing handles...");
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);

            return;
        }
        
        GLogger.writeFormatLine(L"LaunchGameAndWait: failed to create process, error code = %d", GetLastError());
    }
}
