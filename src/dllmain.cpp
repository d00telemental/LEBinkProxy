#include "dllexports.h"
#define ASI_LOG_FNAME "bink2w64_proxy.log"

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

    GLogger.writeln(L"LEBinkProxy by d00telemental");
    GLogger.writeln(L"Version=\"" LEBINKPROXY_VERSION L"\", built=\"" LEBINKPROXY_BUILDTM L"\", config=\"" LEBINKPROXY_BUILDMD L"\"");
    GLogger.writeln(L"Only trust distributions from the official NexusMods page:");
    GLogger.writeln(L"https://www.nexusmods.com/masseffectlegendaryedition/mods/9");

    // Initialize MinHook.
    MH_STATUS mhStatus;
    if (!GHookManager.IsOK(mhStatus))
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

    // Load ASIs, which needs to happen before we wait for DRM.
    // Setup the SPI in the same block to save some time.
    {
        // Use the power of ~~flex tape~~ RAII to freeze/unfreeze all other threads.
        Utils::ScopedThreadFreeze threadFreeze;

        // Spawn the SPI implementation.
        GLEBinkProxy.SPI = new SPI::SharedProxyInterface();
        GLogger.writeln(L"OnAttach: instanced the SPI! (ver = %d)", ASI_SPI_VERSION);

        // Find all native mods and iteratively call LoadLibrary().
        if (!GLEBinkProxy.AsiLoader->Activate())
        {
            GLogger.writeln(L"OnAttach: ERROR: loading of one or more ASI plugins failed!");
        }

        // Load all native mods that declare being pre-drm.
        // Post-drm mods are loaded in the switch below.
        GLEBinkProxy.AsiLoader->PreLoad(GLEBinkProxy.SPI);
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
                GLogger.writeln(L"OnAttach: ERROR: failed to detour CreateWindowEx, aborting!");
                return;  // not using break here because this is a critical failure
            }

            // Wait for an event that would be fired by the hooked CreateWindowEx.
            DRM::WaitForDRMv2();

            // Suspend game threads for the duration of bypass initialization
            // because IsShippingPCBuild gets called *very* quickly.
            {
                Utils::ScopedThreadFreeze threadFreeze;

                // Unlock the console.
                if (!GLEBinkProxy.ConsoleEnabler->Activate())
                {
                    GLogger.writeln(L"OnAttach: ERROR: console bypass installation failed, aborting!");
                    break;
                }

                // Load all native mods that declare being post-drm.
                GLEBinkProxy.AsiLoader->PostLoad(GLEBinkProxy.SPI);
            }

            break;
        }
        case LEGameVersion::Launcher:
        {
            // Wait for three seconds instead of waiting for DRM because launcher has no urgent hooks.
            Sleep(3000);

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

    if (GLEBinkProxy.LauncherArgs)    GLEBinkProxy.LauncherArgs->Deactivate();
    if (GLEBinkProxy.ConsoleEnabler)  GLEBinkProxy.ConsoleEnabler->Deactivate();
    if (GLEBinkProxy.AsiLoader)       GLEBinkProxy.AsiLoader->Deactivate();

    GLogger.writeln(L"OnDetach: goodbye, I thought we were friends :(");
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
