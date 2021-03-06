#pragma once

#include "../minhook/include/MinHook.h"
#include "utils/io.h"


namespace Utils
{
    class HookManager
    {
    private:
        bool initialized_ = false;
        MH_STATUS lastStatus_ = MH_UNKNOWN;

    public:
        HookManager()
        {
            lastStatus_ = MH_OK;
            initialized_ = lastStatus_ == MH_OK;
        }

        bool IsOK(MH_STATUS& status) const noexcept { return (status = lastStatus_) == MH_OK; }

        bool Install(LPVOID pTarget, LPVOID pDetour, LPVOID* ppOriginal, char* name)
        {
            if (!initialized_)
            {
                GLogger.writeln(L"HookManager.Install: ERROR: the manager wasn't initialized properly.");
                return false;
            }

            lastStatus_ = MH_CreateHook(pTarget, pDetour, ppOriginal);
            if (lastStatus_ != MH_OK)
            {
                GLogger.writeln(L"HookManager.Install: ERROR: creating [%S] failed, status = %d", name, lastStatus_);
                return false;
            }
            GLogger.writeln(L"HookManager.Install: created hook [%S]", name);


            lastStatus_ = MH_EnableHook(pTarget);
            if (lastStatus_ != MH_OK)
            {
                GLogger.writeln(L"HookManager.Install: ERROR: enabling [%S] failed, status = %d", name, lastStatus_);
                return false;
            }
            GLogger.writeln(L"HookManager.Install: installed hook [%S]", name);

            return true;
        }

        bool Uninstall(LPVOID pTarget)
        {
            return true;
        }
    };
}

// Global instance.

Utils::HookManager GHookManager;
