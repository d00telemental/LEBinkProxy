#pragma once

#include <new>
#include <Windows.h>
#include "../conf/version.h"
#include "../utils/io.h"
#include "../utils/classutils.h"
#include "../dllstruct.h"
#include "../spi/interface.h"

namespace SPI
{
    // Concrete implementation of ISharedProxyInterface.

    class SharedProxyInterface
        : public ISharedProxyInterface
        , public NonCopyMovable
    {
        // Implementation-detail fields.

        DWORD version_;
        BOOL isRelease_;

        // Private methods.

        __forceinline DWORD getVersion_() const noexcept { return version_; }
        __forceinline bool getReleaseMode_() const noexcept { return isRelease_; }

    public:
        SharedProxyInterface()
            : NonCopyMovable()
            , version_{ ASI_SPI_VERSION }
            , isRelease_{ false }
        {
#ifndef ASI_DEBUG
            isRelease_ = true;
#endif
        }

        // ISharedProxyInterface implementation.

        SPIDEFN GetVersion(DWORD* outVersionPtr)
        {
            *outVersionPtr = this->getVersion_();
            return SPIReturn::Success;
        }

        SPIDEFN GetBuildMode(bool* outIsRelease)
        {
            *outIsRelease = this->getReleaseMode_();
            return SPIReturn::Success;
        }

        SPIDEFN OpenConsole(FILE* outStream, FILE* errStream)
        {
            Utils::OpenConsole(outStream, errStream);
            return SPIReturn::Success;
        }

        SPIDEFN CloseConsole()
        {
            Utils::CloseConsole();
            return SPIReturn::Success;
        }

        SPIDEFN InstallHook(ccstring name, LPVOID target, LPVOID detour, LPVOID* original)
        {
            return SPIReturn::FailureUnsupportedYet;
        }

        SPIDEFN UninstallHook(ccstring name)
        {
            return SPIReturn::FailureUnsupportedYet;
        }

        // End of ISharedProxyInterface implementation.
    };
}
