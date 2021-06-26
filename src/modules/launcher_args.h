#pragma once

#include <Windows.h>
#include "gamever.h"
#include "../utils/io.h"
#include "_base.h"

#include "dllstruct.h"


struct LaunchGameParams
{
    LEGameVersion Target;
    bool AutoTerminate;

    char* GameExePath;
    char* GameCmdLine;
    char* GameWorkDir;
};

void LaunchGameThread(LaunchGameParams launchParams)
{
    auto gameExePath = launchParams.GameExePath;
    auto gameCmdLine = launchParams.GameCmdLine;
    auto gameWorkDir = launchParams.GameWorkDir;


    STARTUPINFOA startupInfo;
    PROCESS_INFORMATION processInfo;

    ZeroMemory(&startupInfo, sizeof(startupInfo));
    ZeroMemory(&processInfo, sizeof(processInfo));
    startupInfo.cb = sizeof(startupInfo);


    GLogger.writeln(L"LaunchGameThread: lpApplicationName = %S", gameExePath);
    GLogger.writeln(L"LaunchGameThread: lpCommandLine = %S", gameCmdLine);
    GLogger.writeln(L"LaunchGameThread: lpCurrentDirectory = %S", gameWorkDir);

    DWORD flags = 0;

    //flags = CREATE_SUSPENDED;
    //GLogger.writeln(L"LaunchGameThread: WARNING! CREATING CHILD PROCESS IN SUSPENDED STATE!");

    auto rc = CreateProcessA(gameExePath, const_cast<char*>(gameCmdLine), nullptr, nullptr, false, flags, nullptr, gameWorkDir, &startupInfo, &processInfo);
    if (rc == 0)
    {
        GLogger.writeln(L"LaunchGameThread: failed to create a process (error code = %d)", GetLastError());
        return;
    }

    // If the user requested auto-termination, just kill the launcher now.
    if (launchParams.AutoTerminate)
    {
        GLogger.writeln(L"LaunchGameThread: created a process (pid = %d), terminating the launcher...", processInfo.dwProcessId);
        exit(0);
        return;  // just in case
    }

    // If not auto-terminated, wait for the process to end.
    GLogger.writeln(L"LaunchGameThread: created a process (pid = %d), waiting until it exits...", processInfo.dwProcessId);
    WaitForSingleObject(processInfo.hProcess, INFINITE);

    GLogger.writeln(L"LaunchGameThread: process exited, closing handles...");
    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);
    return;
}


class LauncherArgsModule
    : public IModule
{
private:

    // Config parameters.

    // Fields.

    LEGameVersion launchTarget_ = LEGameVersion::Unsupported;
    bool autoTerminate_ = false;

    bool needsConfigMade_ = true;

    LaunchGameParams launchParams_;
    char cmdArgsBuffer_[2048];
    char debuggerPath_[512];

    // Methods.

    bool parseCmdLine_(wchar_t* cmdLine)
    {
        auto startCmdLine = cmdLine;
        auto endCmdLine = startCmdLine + wcslen(startCmdLine);
        auto startWCPtr = wcsstr(startCmdLine, L"-game ");

        if (startWCPtr != nullptr && (size_t)(startWCPtr + 6) < (size_t)endCmdLine)
        {
            GLogger.writeln(L"LauncherArgsModule.parseCmdLine_: startWCPtr = %s", startWCPtr);

            auto numStrWCPtr = startWCPtr + 6;
            auto gameNum = wcstol(numStrWCPtr, nullptr, 10);

            // If the game number was parsed to be in [1;3], return it via out arg, and check for autoterminate flag.
            if (gameNum >= 1 && gameNum <= 3)
            {
                this->launchTarget_ = static_cast<LEGameVersion>(gameNum);

                // If found autoterminate flag, return it via out arg.
                if (0 != wcsstr(startCmdLine, L"-autoterminate"))
                {
                    this->autoTerminate_ = true;
                }

                return true;
            }

            GLogger.writeln(L"LauncherArgsModule.parseCmdLine_: wcstol failed to retrieve a code in [1;3]");
            return false;
        }

        this->launchTarget_ = LEGameVersion::Unsupported;
        GLogger.writeln(L"LauncherArgsModule.parseCmdLine_: couldn't find '-game ', startWCPtr = %p", startWCPtr);
        return true;
    }
    bool parseLauncherConfig_()
    {
        int subtitlesSize = 20;
        char* overrideLang = "INT";

        // 3 keys from LauncherConfig are needed to set the settings above correctly:
        //
        //   - EnglishVOEnabled
        //   - Language
        //   - SubtitleSize
        //

        sprintf(this->cmdArgsBuffer_,
            " -NoHomeDir -SeekFreeLoadingPCConsole -locale {locale} -Subtitles %d -OVERRIDELANGUAGE=%s",
            subtitlesSize, overrideLang);

        // -NoHomeDir -SeekFreeLoadingPCConsole -locale {locale} -Subtitles 20 -OVERRIDELANGUAGE=INT

        return true;
    }

    bool overrideForDebug_()
    {
        if (0 != GetEnvironmentVariableA("LEBINK_DEBUGGER", debuggerPath_, 512))
        {
            GLogger.writeln(L"overrideForDebug_ DETECTED, doing the override...");

            char prependBuffer[2048];
            sprintf(prependBuffer, "%s %s", launchParams_.GameExePath, launchParams_.GameCmdLine);
            sprintf(cmdArgsBuffer_, "%s", prependBuffer);

            launchParams_.GameCmdLine = static_cast<char*>(cmdArgsBuffer_);
            launchParams_.GameExePath = static_cast<char*>(debuggerPath_);

            // "D:\\Games Origin\\Mass Effect Legendary Edition\\Game\\ME1\\Binaries\\Win64\\MassEffect1.exe"
            
            return true;
        }

        return false;
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

public:

    // IModule interface.

    LauncherArgsModule()
        : IModule{ "LauncherArgs" }
    {
        ZeroMemory(cmdArgsBuffer_, 2048);        
    }

    bool Activate() override
    {
        // Get options from command line.
        if (!this->parseCmdLine_(GLEBinkProxy.CmdLine))
        {
            GLogger.writeln(L"LauncherArgsModule.Activate: failed to parse cmd line, aborting...");
            // TODO: MessageBox here maybe?
            return false;
        }

        // Return normally if we don't need to do anything.
        if (this->launchTarget_ == LEGameVersion::Unsupported)
        {
            GLogger.writeln(L"LauncherArgsModule.Activate: options not detected, aborting (not a failure)...");
            return true;
        }

        // Report failure if we can't do wonders because of an incorrect arg.
        // This is a redundant check and it's okay.
        if (this->launchTarget_ != LEGameVersion::LE1
            && this->launchTarget_ != LEGameVersion::LE2
            && this->launchTarget_ != LEGameVersion::LE3)
        {
            GLogger.writeln(L"LauncherArgsModule.Activate: options detected but invalid launch target, aborting...");
            return false;
        }

        GLogger.writeln(L"LauncherArgsModule.Activate: valid autoboot target detected: Mass Effect %d",
            static_cast<int>(this->launchTarget_));

        // Pre-parse the launcher config file.
        if (!this->parseLauncherConfig_())
        {
            GLogger.writeln(L"LauncherArgsModule.Activate: failed to parse Launcher config, aborting...");
            // TODO: MessageBox here maybe?
            return false;
        }

        // Bail out without an error if the config file doesn't exist - is this the first launch?
        if (FALSE && this->needsConfigMade_)
        {
            GLogger.writeln(L"LauncherArgsModule.Activate: launcher config file is missing, aborting (not a failure)...");
            // TODO: MessageBox here maybe?
            return true;
        }

        // Set up everything for the process start.
        launchParams_.Target = this->launchTarget_;
        launchParams_.AutoTerminate = this->autoTerminate_;
        launchParams_.GameExePath = const_cast<char*>(gameToPath_(this->launchTarget_));
        launchParams_.GameCmdLine = const_cast<char*>(this->cmdArgsBuffer_);
        launchParams_.GameWorkDir = const_cast<char*>(gameToWorkingDir_(this->launchTarget_));

        // Allow debugging functionality.
        this->overrideForDebug_();

        // Start the process in a new thread.
        auto rc = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)LaunchGameThread, &(this->launchParams_), 0, nullptr);
        if (rc == nullptr)
        {
            GLogger.writeln(L"LauncherArgsModule.Activate: failed to create a thread (error code = %d)", GetLastError());
            // TODO: MessageBox here maybe?
            return false;
        }

        return true;
    }

    void Deactivate() override
    {
        return;
    }

    // End of IModule interface.
};
