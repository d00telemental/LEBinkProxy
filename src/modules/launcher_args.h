#pragma once
#include "../dllincludes.h"
#include "../utils/io.h"

#include "_base.h"


class LauncherArgsModule
    : public IModule
{
private:

    // Config parameters.

    // Fields.

    LEGameVersion launchTarget_ = LEGameVersion::Unsupported;
    bool autoTerminate_ = false;

    // Methods.

    bool parseCmdLine_(wchar_t* cmdLine)
    {
        auto startCmdLine = cmdLine;
        auto endCmdLine = startCmdLine + wcslen(startCmdLine);
        auto startWCPtr = wcsstr(startCmdLine, L"-game ");

        if (startWCPtr != nullptr && (size_t)(startWCPtr + 6) < (size_t)endCmdLine)
        {
            GLogger.writeFormatLine(L"LauncherArgsModule.parseCmdLine_: startWCPtr = %s", startWCPtr);

            auto numStrWCPtr = startWCPtr + 6;
            auto gameNum = wcstol(numStrWCPtr, nullptr, 10);

            // If the game number was parsed to be in [1;3], return it via out arg, and check for autoterminate flag.
            if (gameNum >= 1 && gameNum <= 3)
            {
                launchTarget_ = static_cast<LEGameVersion>(gameNum);

                // If found autoterminate flag, return it via out arg.
                if (0 != wcsstr(startCmdLine, L"-autoterminate"))
                {
                    autoTerminate_ = true;
                }

                return true;
            }

            GLogger.writeFormatLine(L"LauncherArgsModule.parseCmdLine_: wcstol failed to retrieve a code in [1;3]");
            return false;
        }

        GLogger.writeFormatLine(L"LauncherArgsModule.parseCmdLine_: couldn't find '-game ', startWCPtr = %p", startWCPtr);
        return true;
    }

    const char* gameToPath_(LEGameVersion version) const noexcept
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
    const char* gameToWorkingDir_(LEGameVersion version) const noexcept
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

    bool launchGame_() const
    {
        auto gameExePath = gameToPath_(launchTarget_);
        auto gameCmdLine = " -NoHomeDir -SeekFreeLoadingPCConsole -locale {locale} -Subtitles 20 -OVERRIDELANGUAGE=INT";  // TODO
        auto gameWorkDir = gameToWorkingDir_(launchTarget_);


        STARTUPINFOA startupInfo;
        PROCESS_INFORMATION processInfo;

        ZeroMemory(&startupInfo, sizeof(startupInfo));
        ZeroMemory(&processInfo, sizeof(processInfo));
        startupInfo.cb = sizeof(startupInfo);


        GLogger.writeFormatLine(L"LauncherArgsModule.launchGame_: lpApplicationName = %S", gameExePath);
        GLogger.writeFormatLine(L"LauncherArgsModule.launchGame_: lpCommandLine = %S", gameCmdLine);
        GLogger.writeFormatLine(L"LauncherArgsModule.launchGame_: lpCurrentDirectory = %S", gameWorkDir);


        auto rc = CreateProcessA(gameExePath, const_cast<char*>(gameCmdLine), nullptr, nullptr, false, 0, nullptr, gameWorkDir, &startupInfo, &processInfo);
        if (rc == 0)
        {
            GLogger.writeFormatLine(L"LauncherArgsModule.launchGame_: failed to create a process (error code = %d)", GetLastError());
            return false;
        }

        // If the user requested auto-termination, just kill the launcher now.
        if (autoTerminate_)
        {
            GLogger.writeFormatLine(L"LauncherArgsModule.launchGame_: created a process (pid = %d), terminating the launcher...", processInfo.dwProcessId);
            exit(0);
            return true;  // just in case
        }

        // If not auto-terminated, wait for the process to end.
        GLogger.writeFormatLine(L"LauncherArgsModule.launchGame_: created a process (pid = %d), waiting until it exits...", processInfo.dwProcessId);
        WaitForSingleObject(processInfo.hProcess, INFINITE);

        GLogger.writeFormatLine(L"LauncherArgsModule.launchGame_: process exited, closing handles...");
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
        return true;
    }

public:

    // IModule interface.

    LauncherArgsModule()
        : IModule{ "LauncherArgs" }
    {

    }

    bool Activate() override
    {
        
    }

    void Deactivate() override
    {

    }

    // End of IModule interface.



};
