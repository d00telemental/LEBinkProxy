#pragma once

#include <Windows.h>
#include "../utils/io.h"
#include "../utils/hook.h"
#include "../dllstruct.h"
#include "_base.h"
#include "drm.h"
#include "ue_types.h"


#define FIND_PATTERN(TYPE,VAR,NAME,PAT,MASK) \
temp = Utils::ScanProcess(PAT, MASK); \
if (!temp) { \
    GLogger.writeln(L"findOffsets_: ERROR: failed to find " NAME L"."); \
    return false; \
} \
GLogger.writeln(L"findOffsets_: found " NAME L" at %p.", temp); \
VAR = (TYPE)temp;


class ConsoleEnablerModule
    : public IModule
{
private:

    // Config parameters.

    // Fields.

    // Methods.

    bool findOffsets_()
    {
        BYTE* temp = nullptr;

        switch (GLEBinkProxy.Game)
        {
        case LEGameVersion::LE1:
            FIND_PATTERN(UE::tUFunctionBind, UE::UFunctionBind, L"UFunction::Bind", LE1_UFunctionBind_Pattern, LE1_UFunctionBind_Mask);
            FIND_PATTERN(UE::tGetName, UE::GetName, L"GetName", LE1_GetName_Pattern, LE1_GetName_Mask);
            break;
        case LEGameVersion::LE2:
            FIND_PATTERN(UE::tUFunctionBind, UE::UFunctionBind, L"UFunction::Bind", LE2_UFunctionBind_Pattern, LE2_UFunctionBind_Mask);
            FIND_PATTERN(UE::tGetName, UE::NewGetName, L"NewGetName", LE2_NewGetName_Pattern, LE2_NewGetName_Mask);
            break;
        case LEGameVersion::LE3:
            FIND_PATTERN(UE::tUFunctionBind, UE::UFunctionBind, L"UFunction::Bind", LE3_UFunctionBind_Pattern, LE3_UFunctionBind_Mask);
            FIND_PATTERN(UE::tGetName, UE::NewGetName, L"NewGetName", LE3_NewGetName_Pattern, LE3_NewGetName_Mask);
            break;
        default:
            GLogger.writeln(L"findOffsets_: ERROR: unsupported game version.");
            break;
        }
        return true;
    }

    bool detourOffsets_()
    {
        GLogger.writeln(L"detourOffsets_: installing %p into %p, preserving into %p",
            UE::HookedUFunctionBind, UE::UFunctionBind, reinterpret_cast<LPVOID*>(&UE::UFunctionBind_orig));
        return GHookManager.Install(UE::UFunctionBind, UE::HookedUFunctionBind, reinterpret_cast<LPVOID*>(&UE::UFunctionBind_orig), "UFunctionBind");
    }

public:
    ConsoleEnablerModule()
        : IModule{ "ConsoleEnabler" }
    {
        active_ = true;
    }

    bool Activate() override
    {
        if (!this->findOffsets_())
        {
            return false;
        }

        if (!this->detourOffsets_())
        {
            return false;
        }

        return true;
    }

    void Deactivate() override
    {

    }
};
