#pragma once

#include <Windows.h>
#include "../utils/io.h"
#include "_base.h"
#include "drm.h"


class ConsoleEnablerModule
    : public IModule
{
private:

    // Config parameters.

    // Fields.

    // Methods.

public:
    ConsoleEnablerModule()
        : IModule{ "ConsoleEnabler" }
    {
        active_ = true;
    }

    bool Activate() override
    {
        return true;
    }

    void Deactivate() override
    {

    }
};
