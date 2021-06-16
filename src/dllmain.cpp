#include "dllexports.h"

#include <Windows.h>

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


void __stdcall OnAttach()
{
    // Open console or log.
    Utils::SetupOutput();

    GLogger.writeFormatLine(L"LEBinkProxy by d00telemental");
    GLogger.writeFormatLine(L"Version=\"" LEBINKPROXY_VERSION L"\", built=\"" LEBINKPROXY_BUILDTM L"\", config=\"" LEBINKPROXY_BUILDMD L"\"");
    GLogger.writeFormatLine(L"Only trust distributions from the official NexusMods page:");
    GLogger.writeFormatLine(L"https://www.nexusmods.com/masseffectlegendaryedition/mods/9");

    // Initialize MinHook.
    MH_STATUS mhStatus;
    if (!GHookManager.IsOK(mhStatus))
    {
        GLogger.writeFormatLine(L"OnAttach: ERROR: failed to initialize the hooking library (code = %d).", mhStatus);
        return;
    }

    // Initialize global settings.
    GLEBinkProxy.Initialize();

    // Register modules (console enabler, launcher arg handler, asi loader).
    GLEBinkProxy.AsiLoader = new AsiLoaderModule;
    GLEBinkProxy.ConsoleEnabler = new ConsoleEnablerModule;
    GLEBinkProxy.LauncherArgs = new LauncherArgsModule;

    // Setup the SPI.
    if (!SPI::SharedMemory::Create())
    {
        GLogger.writeFormatLine(L"OnAttach: ERROR: failed to initialize the SPI!");
        // This should not be a fatal error.
        // Or should it?
    }

    // Load ASIs, which needs to happen before we wait for DRM.
    {
        // Use the power of ~~flex tape~~ RAII to freeze/unfreeze all other threads.
        Utils::ScopedThreadFreeze threadFreeze;

        // Find all files and iteratively call LoadLibrary().
        if (!GLEBinkProxy.AsiLoader->Activate())
        {
            GLogger.writeFormatLine(L"OnAttach: ERROR: loading of one or more ASI plugins failed!");
        }
    }

    // Handle logic depending on the attached-to exe.
    switch (GLEBinkProxy.Game)
    {
        case LEGameVersion::LE1:
        case LEGameVersion::LE2:
        case LEGameVersion::LE3:
        {
            // Set things up for DRM wait further.
            if (!GHookManager.Install(CreateWindowExW, DRM::CreateWindowExW_hooked, reinterpret_cast<LPVOID*>(&DRM::CreateWindowExW_orig), "CreateWindowExW"))
            {
                GLogger.writeFormatLine(L"OnAttach: ERROR: failed to detour CreateWindowEx, aborting!");
                return;  // not using break here because this is a critical failure
            }

            // Wait for an event that would be fired by the hooked CreateWindowEx.
            DRM::WaitForDRMv2();

            // Suspend game threads for the duration of bypass initialization
            // because IsShippingPCBuild gets called *very* quickly.
            // Should be resumed on error or just before loading ASIs.
            {
                Utils::ScopedThreadFreeze threadFreeze;

                // Unlock the console.
                if (!GLEBinkProxy.ConsoleEnabler->Activate())
                {
                    GLogger.writeFormatLine(L"OnAttach: ERROR: console bypass installation failed, aborting!");
                    break;
                }
            }

            break;
        }
        case LEGameVersion::Launcher:
        {
            // Wait for three seconds instead of waiting for DRM because launcher has no urgent hooks.
            Sleep(3000);

            if (!GLEBinkProxy.LauncherArgs->Activate())
            {
                GLogger.writeFormatLine(L"OnAttach: ERROR: handling of Launcher args failed, aborting!");
            }
            break;
        }
        default:
        {
            GLogger.writeFormatLine(L"OnAttach: unsupported game, bye!");
            return;
        }
    }

    return;
}

void __stdcall OnDetach()
{
    // Close the handles in SPI.
    SPI::SharedMemory::Close();

    GLogger.writeFormatLine(L"OnDetach: goodbye, I thought we were friends :(");
    Utils::TeardownOutput();
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)OnAttach, nullptr, 0, nullptr);
        return TRUE;

    case DLL_PROCESS_DETACH:
        OnDetach();
        return TRUE;

    default:
        return TRUE;
    }
}
