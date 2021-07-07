#pragma once

#include <cstring>
#include <mutex>
#include <new>
#include <thread>
#include <Windows.h>
#include "../conf/version.h"
#include "../utils/io.h"
#include "../utils/hook.h"
#include "../utils/classutils.h"
#include "../utils/memory.h"
#include "../dllstruct.h"
#include "../spi/shared_hook_manager.h"
#include "../spi/interface.h"


#define SPI_IMPL_INSTANCE_LOCK(MUTEX) const std::lock_guard<std::mutex> lock(this->MUTEX);


namespace SPI
{

    // Concrete implementation of ISharedProxyInterface.

    class SharedProxyInterface
        : public ISharedProxyInterface
        , public NonCopyMovable
    {
        // Implementation details.

        DWORD version_;
        BOOL isRelease_;

        SharedHookManager hookMngr_;

        std::mutex mtxGetVersion_;
        std::mutex mtxGetBuildMode_;
        std::mutex mtxGetHostGame_;
        std::mutex mtxFindPattern_;
        std::mutex mtxInstallHook_;
        std::mutex mtxUninstallHook_;

        // Implementation methods.

        __forceinline DWORD getVersion_() const noexcept { return version_; }
        __forceinline bool getReleaseMode_() const noexcept { return isRelease_; }

        __declspec(noinline) bool parseCombinedPattern_(char* inPattern, BYTE* outPatternBuffer, BYTE* outMaskBuffer, size_t* outLength)
        {
            auto len = inPattern ? std::strlen(inPattern) : 0;
            if (len < 6)
            {
                return false;
            }

            char* token = nullptr;
            char* endPtr = nullptr;
            long byteValue = 0;
            size_t parsedLength = 0;
            size_t patternLength = 0;

            do
            {
                token = std::strtok(parsedLength == 0 ? inPattern : nullptr, " ");
                if (token)
                {
                    parsedLength = token + 2 - inPattern;

                    //GLogger.writeln(L"parseCombinedPattern_: token = %S, parsedLength = %llu, len = %llu, strlen(token) = %llu", token, parsedLength, len, strlen(token));
                    
                    if (strlen(token) != 2)
                    {
                        GLogger.writeln(L"parseCombinedPattern_: ERROR: parsed token was not 2 chars long: %S", token);
                        return false;
                    }

                    if (0 == strcmp(token, "??"))
                    {
                        outPatternBuffer[patternLength] = 0u;
                        outMaskBuffer[patternLength] = 'x';
                        ++patternLength;
                    }
                    else
                    {
                        errno = 0;
                        if (0 == (byteValue = std::strtol(token, &endPtr, 0x10)) && errno != 0)
                        {
                            GLogger.writeln(L"parseCombinedPattern_: ERROR: strtol returned an error (errno = %d)", errno);
                            return false;
                        }
                        else
                        {
                            auto foo = (*outPatternBuffer);
                            outPatternBuffer[patternLength] = static_cast<BYTE>(byteValue);
                            outMaskBuffer[patternLength] = '0';
                            ++patternLength;
                        }
                    }
                }
                else
                {
                    if (parsedLength != len)
                    {
                        GLogger.writeln(L"parseCombinedPattern_: ERROR: token was null but inPattern end wasn't reached (%d != %d)", parsedLength, len);
                        return false;
                    }
                }
            } while (token);

            *outLength = patternLength;
            return true;
        }

    public:
        SharedProxyInterface()
            : NonCopyMovable()
            , version_{ ASI_SPI_VERSION }
            , isRelease_{ false }
            , hookMngr_{ }
        {
#ifndef ASI_DEBUG
            isRelease_ = true;
#endif
        }

        // ISharedProxyInterface implementation.

        SPIDEFN GetVersion(unsigned long* outVersionPtr)
        {
            SPI_IMPL_INSTANCE_LOCK(mtxGetVersion_);

            if (!outVersionPtr)
            {
                return SPIReturn::FailureInvalidParam;
            }

            *outVersionPtr = this->getVersion_();
            return SPIReturn::Success;
        }

        SPIDEFN GetBuildMode(bool* outIsRelease)
        {
            SPI_IMPL_INSTANCE_LOCK(mtxGetBuildMode_);

            if (!outIsRelease)
            {
                return SPIReturn::FailureInvalidParam;
            }

            *outIsRelease = this->getReleaseMode_();
            return SPIReturn::Success;
        }

        SPIDEFN GetHostGame(SPIGameVersion* outGameVersion)
        {
            SPI_IMPL_INSTANCE_LOCK(mtxGetHostGame_);

            if (!outGameVersion)
            {
                return SPIReturn::FailureInvalidParam;
            }

            *outGameVersion = static_cast<SPIGameVersion>(GLEBinkProxy.Game);
            return SPIReturn::FailureUnsupportedYet;
        }

        SPIDEFN FindPattern(void** outOffsetPtr, char* combinedPattern)
        {
            SPI_IMPL_INSTANCE_LOCK(mtxFindPattern_);

            if (!outOffsetPtr || !combinedPattern)
            {
                return SPIReturn::FailureInvalidParam;
            }
            if (strlen(combinedPattern) > 300)
            {
                return SPIReturn::FailurePatternTooLong;
            }


            // Unpack the combined pattern into pattern and a mask.

            auto inPatternCopy = _strdup(combinedPattern);

            BYTE patternBytes[100];
            BYTE maskBytes[100];
            size_t patternLength = 0;

            ZeroMemory(patternBytes, 100);
            ZeroMemory(maskBytes, 100);
            
            if (!this->parseCombinedPattern_(inPatternCopy, (BYTE*)patternBytes, (BYTE*)maskBytes, &patternLength))
            {
                return SPIReturn::FailurePatternInvalid;
            }

            //GLogger.writeln(L"Pattern length = %llu", patternLength);
            //for (size_t i = 0; i < patternLength; i++) printf(" %02x", patternBytes[i]);  printf("\n\n");
            //for (size_t i = 0; i < patternLength; i++) printf(" %02x", maskBytes[i]);     printf("\n\n");

            free(inPatternCopy);

            // Use the built-in memory scanner.

            auto offset = Utils::ScanProcess(patternBytes, maskBytes);
            if (!offset)
            {
                *outOffsetPtr = nullptr;
                return SPIReturn::FailureGeneric;
            }

            *outOffsetPtr = offset;
            return SPIReturn::Success;
        }

        SPIDEFN InstallHook(const char* name, void* target, void* detour, void** original)
        {
            SPI_IMPL_INSTANCE_LOCK(mtxInstallHook_);

            if (hookMngr_.HookExists(const_cast<char*>(name)))
            {
                GLogger.writeln(L"Failed to install the hook [%S] because it already exists", name);
                return SPIReturn::FailureDuplicacy;
            }

            if (!hookMngr_.Install(target, detour, original, const_cast<char*>(name)))
            {
                GLogger.writeln(L"Failed to install the hook [%S]", name);
                return SPIReturn::FailureHooking;
            }

            return SPIReturn::Success;
        }

        SPIDEFN UninstallHook(const char* name)
        {
            SPI_IMPL_INSTANCE_LOCK(mtxUninstallHook_);

            if (!hookMngr_.HookExists(const_cast<char*>(name)))
            {
                GLogger.writeln(L"Failed to uninstall the hook [%S] because it doesn't exist", name);
                return SPIReturn::FailureDuplicacy;
            }

            if (!hookMngr_.Uninstall(const_cast<char*>(name)))
            {
                GLogger.writeln(L"Failed to uninstall the hook [%S]", name);
                return SPIReturn::FailureHooking;
            }

            return SPIReturn::Success;
        }

        // End of ISharedProxyInterface implementation.
    };
}
