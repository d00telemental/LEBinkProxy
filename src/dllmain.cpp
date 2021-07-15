#include "dllexports.h"
#define ASI_LOG_FNAME "bink2w64_proxy.log"

#include <Windows.h>
#include <Dbghelp.h>

#include <cwchar>

#include "conf/version.h"
#include "gamever.h"
#include "utils/io.h"
#include "utils/hook.h"
#include "dllstruct.h"
#include "utils/memory.h"
#include "spi.h"
#include "modules/asi_loader.h"
#include "modules/console_enabler.h"
#include "modules/launcher_args.h"


PVOID GhVEH = NULL;

LONG LEBinkProxyVectoredExceptionHandler(PEXCEPTION_POINTERS pExceptionPtrs)
{
    // If this is a debugging exception, skip.

    auto code = pExceptionPtrs->ExceptionRecord->ExceptionCode;
    if (code == CONTROL_C_EXIT
        || code == STATUS_BREAKPOINT
        || code == DBG_PRINTEXCEPTION_C || code == DBG_PRINTEXCEPTION_WIDE_C
        || code == 0xE06D7363 || code == 0x406D1388)  // the stuff that gets thrown "normally"
    {
        goto return_from_handler;
    }


    // Load dbghelp and the MiniDumpWriteDump function.
    
    auto libDbghelp = LoadLibraryA("dbghelp");
    if (!libDbghelp)
    {
        goto return_from_handler;
    }
    auto fnMiniDumpWriteDump = (decltype(&MiniDumpWriteDump))GetProcAddress(libDbghelp, "MiniDumpWriteDump");
    if (!fnMiniDumpWriteDump)
    {
        goto return_from_handler;
    }

    
    // Get the base dump file name, which will be completed depending on the dump mode.

    wchar_t wstrDumpFile[MAX_PATH];
    auto dwModuleNameLen = GetModuleFileNameW(GetModuleHandleW(0), wstrDumpFile, MAX_PATH);

    MINIDUMP_TYPE dumpType;
    SYSTEMTIME time;
    GetSystemTime(&time);

    
    // If -killmydisk is set, dump the entire process memory.
    // Otherwise, only the crash call stack.

    if (nullptr != std::wcsstr(GetCommandLineW(), L" -killmydisk"))
    {
        dumpType = MINIDUMP_TYPE(MiniDumpWithFullMemory);
        swprintf(wstrDumpFile + dwModuleNameLen - 4, MAX_PATH, L"_%4d%02d%02d_%02d%02d%02df.dmp",
            time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);
    }
    else
    {
        dumpType = MINIDUMP_TYPE(MiniDumpNormal);
        swprintf(wstrDumpFile + dwModuleNameLen - 4, MAX_PATH, L"_%4d%02d%02d_%02d%02d%02dn.dmp",
            time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);
    }


    // Open a dump file in the game's directory.

    auto dumpFile = CreateFileW(wstrDumpFile, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (dumpFile == INVALID_HANDLE_VALUE)
    {
        goto return_from_handler;
    }

    MINIDUMP_EXCEPTION_INFORMATION exInfo;
    exInfo.ThreadId = GetCurrentThreadId();
    exInfo.ExceptionPointers = pExceptionPtrs;
    exInfo.ClientPointers = FALSE;


    // Write the dump.

    auto dumped = fnMiniDumpWriteDump(
        GetCurrentProcess(), GetCurrentProcessId(),
        dumpFile, dumpType,
        pExceptionPtrs ? &exInfo : nullptr,
        nullptr, nullptr);

    CloseHandle(dumpFile);


return_from_handler:

    return EXCEPTION_CONTINUE_SEARCH;
}
void SetVectoredExceptionHandler()
{
    GhVEH = AddVectoredExceptionHandler(1, LEBinkProxyVectoredExceptionHandler);
}
void UnsetVectoredExceptionHandler()
{
    if (GhVEH)
    {
        RemoveVectoredExceptionHandler(GhVEH);
    }
}

void __stdcall OnAttach()
{
    // Open console or log.
    Utils::SetupOutput();

    GLogger.writeln(L"Attached...\n"
                    L"LEBinkProxy by d00telemental\n"
                    L"Version=\"" LEBINKPROXY_VERSION L"\", built=\"" LEBINKPROXY_BUILDTM L"\", config=\"" LEBINKPROXY_BUILDMD L"\"\n"
                    L"Only trust distributions installed by ME3Tweaks Mod Manager 7.0+ !");

    // Register exception handler for memory dumps.
    // Removed on DETACH.
    SetVectoredExceptionHandler();

    // Initialize MinHook.
    MH_STATUS mhStatus = MH_Initialize();
    if (mhStatus != MH_OK)
    {
        GLogger.writeln(L"OnAttach: ERROR: failed to initialize the hooking library (code = %d).", mhStatus);
        return;
    }

    // Initialize global settings.
    GLEBinkProxy.Initialize();

    // Register modules (console enabler, launcher arg handler, asi loader).
    GLEBinkProxy.AsiLoader = new AsiLoaderModule;
    GLEBinkProxy.ConsoleEnabler = new ConsoleEnablerModule;
    GLEBinkProxy.LauncherArgs = new LauncherArgsModule;

    // Spawn the SPI implementation.
    GLEBinkProxy.SPI = new SPI::SharedProxyInterface();
    GLogger.writeln(L"OnAttach: instanced the SPI! (ver = %d)", ASI_SPI_VERSION);

    // Find all native mods and iteratively call LoadLibrary().
    if (!GLEBinkProxy.AsiLoader->Activate())
    {
        GLogger.writeln(L"OnAttach: ERROR: loading of one or more ASI plugins failed!");
    }

    GLogger.writeln(L"OnAttach: got to preload / 0x%p", GLEBinkProxy);

    // Load all native mods that declare being pre-drm.
    // Post-drm mods are loaded in the switch below.
    GLEBinkProxy.AsiLoader->PreLoad(GLEBinkProxy.SPI);

    // Prevent the compiler from *potentially* reordering instructions before and after.
    MemoryBarrier();

    // Handle logic depending on the attached-to exe.
    switch (GLEBinkProxy.Game)
    {
        case LEGameVersion::LE1:
        case LEGameVersion::LE2:
        case LEGameVersion::LE3:
        {
            // Keep trying to find a pattern we are *almost* guaranteed to have until we find it.
            DRM::WaitForDRMv3();

            // Unlock the console.
            if (!GLEBinkProxy.ConsoleEnabler->Activate())
            {
                GLogger.writeln(L"OnAttach: ERROR: console bypass installation failed, aborting!");
                break;
            }

            // Load all native mods that declare being post-drm.
            GLEBinkProxy.AsiLoader->PostLoad(GLEBinkProxy.SPI);

            break;
        }
        case LEGameVersion::Launcher:
        {
            if (!GLEBinkProxy.LauncherArgs->Activate())
            {
                GLogger.writeln(L"OnAttach: ERROR: handling of Launcher args failed, aborting!");
            }
            break;
        }
        default:
        {
            GLogger.writeln(L"OnAttach: unsupported game, bye!");
            return;
        }
    }

    return;
}

void __stdcall OnDetach()
{
    GLogger.writeln(L"OnDetach: entered...");

    // Unload the DLLs.
    if (GLEBinkProxy.AsiLoader)       GLEBinkProxy.AsiLoader->Deactivate();

    // No-ops
    if (GLEBinkProxy.LauncherArgs)    GLEBinkProxy.LauncherArgs->Deactivate();
    if (GLEBinkProxy.ConsoleEnabler)  GLEBinkProxy.ConsoleEnabler->Deactivate();

    GLogger.writeln(L"OnDetach: goodbye, I thought we were friends :(");
    Utils::TeardownOutput();

    // Remove the VEH handler we set in OnAttach.
    UnsetVectoredExceptionHandler();
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)OnAttach, nullptr, 0, nullptr);
        return TRUE;

    case DLL_PROCESS_DETACH:
        OnDetach();
        return TRUE;

    default:
        return TRUE;
    }
}
