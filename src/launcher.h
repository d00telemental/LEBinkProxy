#pragma once
#include "dllincludes.h"
#include "io.h"

namespace Launcher
{
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

    LEGameVersion ParseVersionFromCmd(wchar_t* cmdLine)
    {
        GLogger.writeFormatLine(L"HandleCommandLine: cmdLine = %s", cmdLine);

        auto startCmdLine = cmdLine;
        auto endCmdLine = startCmdLine + wcslen(startCmdLine);
        auto startWCPtr = wcsstr(startCmdLine, L"-game=");

        if (startWCPtr != nullptr && (size_t)(startWCPtr + 6) < (size_t)endCmdLine)
        {
            GLogger.writeFormatLine(L"HandleCommandLine: startWCPtr = %s", startWCPtr);

            auto numStrWCPtr = startWCPtr + 6;
            auto gameNum = wcstol(numStrWCPtr, nullptr, 10);

            if (gameNum >= 1 && gameNum <= 3)
            {
                GLogger.writeFormatLine(L"HandleCommandLine: game selection is %d", gameNum);
                return static_cast<LEGameVersion>(gameNum);
            }

            GLogger.writeFormatLine(L"HandleCommandLine: wcstol failed to retrieve a code in [1;3]");
        }

        GLogger.writeFormatLine(L"HandleCommandLine: failed to find '-game=', startCmdLine = %p, endCmdLine = %p, startWCPtr = %p",
            startCmdLine, endCmdLine, startWCPtr);

        return LEGameVersion::Unsupported;
    }
}
