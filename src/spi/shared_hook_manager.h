#pragma once

#include "../minhook/include/MinHook.h"
#include "utils/classutils.h"
#include "../dllstruct.h"
#include <map>
#include <mutex>
#include <string>
#include <thread>

#define SHOOKMNGR_LOCK(MUTEX) const std::lock_guard<std::mutex> lock(MUTEX);

namespace SPI
{
    struct HookComboData
    {
        LPVOID Target;
        ULONG_PTR Identity;

        HookComboData() = default;
        HookComboData(ULONG_PTR ident, LPVOID target) : Identity{ ident }, Target{ target } { }
    };

    /// <summary>
    /// Hook manager to be used internally by SPI.
    /// Use the Utils::HookManager for everything aside from SPI!
    /// </summary>
    class SharedHookManager
        : public NonCopyMovable
    {
    private:
        ULONG_PTR hookCounter_;  // incremented every time a function is hooked

        bool mhInitialized_;
        MH_STATUS mhLastStatus_;

        std::mutex installMtx_;
        std::mutex uninstallMtx_;

        std::map<std::string, HookComboData> nameToHookMap_;

    public:

        SharedHookManager()
            : hookCounter_{ 0 }
            , mhInitialized_{ false }
            , mhLastStatus_ { MH_UNKNOWN }
            , nameToHookMap_{ }
        {
            // MH_Initialize must have been called by now!
            mhLastStatus_ = MH_OK;
            mhInitialized_ = mhLastStatus_ == MH_OK;
        }

        __forceinline bool IsOK(MH_STATUS& status) const noexcept { return (status = mhLastStatus_) == MH_OK; }
        __forceinline bool IsInitialized() const noexcept { return mhInitialized_; }

        bool HookExists(char* name)
        {
            return nameToHookMap_.find(std::string{ name }) != nameToHookMap_.end();
        }

        bool Install(LPVOID target, LPVOID detour, LPVOID* original, char* name)
        {
            SHOOKMNGR_LOCK(installMtx_);

            if (!IsInitialized() || !IsOK(mhLastStatus_))
            {
                GLogger.writeln(L"SharedHookMngr.Install: was not initialized or was in bad status");
                return false;
            }

            if (HookExists(name))
            {
                GLogger.writeln(L"SharedHookMngr.Install: hook of this name already exists");
                return false;
            }

            // Hook counter serves as the hook identity.
            // Sometime around v3 I want to limit it to one hook per plugin for the same function.
            ++hookCounter_;

            mhLastStatus_ = MH_CreateHookEx(hookCounter_, original, detour, target);
            if (mhLastStatus_ != MH_OK)
            {
                GLogger.writeln(L"SharedHookMngr.Install: create failed, status = %d", mhLastStatus_);
                return false;
            }

            GLogger.writeln(L"SharedHookMngr.Install: created [%S] 0x%p -> 0x%p", name, target, detour);

            mhLastStatus_ = MH_EnableHookEx(hookCounter_, target);
            if (mhLastStatus_ != MH_OK)
            {
                GLogger.writeln(L"SharedHookMngr.Install: enable failed, status = %d", mhLastStatus_);
                return false;
            }

            // Save the installed hook info
            nameToHookMap_.insert({ name, HookComboData{ hookCounter_, target } });

            GLogger.writeln(L"SharedHookMngr.Install: enabled [%S] 0x%p", name, target);
            return true;

        }

        bool Uninstall(char* name)
        {
            SHOOKMNGR_LOCK(uninstallMtx_);

            if (!IsInitialized() || !IsOK(mhLastStatus_))
            {
                GLogger.writeln(L"SharedHookMngr.Uninstall: was not initialized or was in bad status");
                return false;
            }

            if (!HookExists(name))
            {
                GLogger.writeln(L"SharedHookMngr.Uninstall: hook of this name already exists");
                return false;
            }

            auto hookInfo = nameToHookMap_.at(std::string{ name });
            mhLastStatus_ = MH_RemoveHookEx((void*)4123, hookInfo.Identity, hookInfo.Target);
            if (mhLastStatus_ != MH_OK)
            {
                GLogger.writeln(L"SharedHookMngr.Uninstall: remove failed, status = %d", mhLastStatus_);
                return false;
            }

            return true;
        }
    };
}
