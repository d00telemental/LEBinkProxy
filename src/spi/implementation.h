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
    // Forward-declare both classes.

    class SharedMemory;
    class SharedProxyInterface;


    // Shared memory, used by ASIs to find and acquire the interface.

#pragma pack(1)
    class SharedMemory
    {
    public:

        // DO NOT REORDER OR CHANGE TYPES

        char Magic[4];         // 0x00 (0x04)
        DWORD Version;         // 0x04 (0x04)
        DWORD Size;            // 0x08 (0x04)
        char BuildDate[32];    // 0x0C (0x20)
        char BuildMode[8];     // 0x2C (0x08)
        void* ConcretePtr;     // 0x34 (0x08)  ==  0x3C

        // ONLY ADD NEW MEMBERS AFTER THIS LINE

    private:

        static HANDLE mapFile_;
        static SharedMemory* instance_;

        SharedMemory(void* concretePtr)
            : Magic { "SPI" }
            , Version{ ASI_SPI_VERSION }
            , Size{ GetSize() }
            , BuildDate{__DATE__ " " __TIME__}
            , ConcretePtr{ concretePtr }
        {
#if defined(ASI_DEBUG) && !defined(NDEBUG)
            CopyMemory(BuildMode, "DEBUG", 6);
#elif !defined(ASI_DEBUG) && defined(NDEBUG)
            CopyMemory(BuildMode, "RELEASE", 8);
#else
#error INCONCLUSIVE BUILD MODE PREPROCESSOR DEFINITIONS
#endif
        }

        // Get size of the memory block to allocate.
        [[nodiscard]] __forceinline static unsigned int GetSize() noexcept { return sizeof(SharedMemory); }

    public:

        // Create an instance of SPISharedMemory in... shared memory :pikawhoa:
        __declspec(noinline) static SharedMemory* Create() noexcept
        {
            if (instance_)
            {
                return instance_;
            }

            wchar_t fileMappingName[256];
            auto rc = ISharedProxyInterface::MakeMemoryName(fileMappingName, 256, (int)GLEBinkProxy.Game);
            if (rc != SPIReturn::Success)
            {
                GLogger.writeFormatLine(L"SPISharedMemory::Create: ERROR: failed to make memory mapping name, error = %d", static_cast<int>(rc));
                return nullptr;
            }

            GLogger.writeFormatLine(L"SPISharedMemory::Create: creating a file mapping with name \"%s\"", fileMappingName);
            mapFile_ = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_EXECUTE_READWRITE, 0, GetSize(), fileMappingName);
            if (!mapFile_)
            {
                GLogger.writeFormatLine(L"SPISharedMemory::Create: ERROR: failed to create a file mapping, error code = %d", GetLastError());
                return nullptr;
            }

            instance_ = (SharedMemory*)MapViewOfFile(mapFile_, FILE_MAP_ALL_ACCESS | FILE_MAP_EXECUTE, 0, 0, GetSize());
            if (!instance_)
            {
                GLogger.writeFormatLine(L"SPISharedMemory::Create: ERROR: failed to map view of file, error code = %d", GetLastError());
                return nullptr;
            }

            // Placement-create this in the shared memory.
            auto memoryPtr = new(instance_) SharedMemory(nullptr);

#ifdef ASI_DEBUG
            GLogger.writeFormatLine(L"SPISharedMemory::Create: successfully created an SPI memory object at %p!", memoryPtr);
#else
            GLogger.writeFormatLine(L"SPISharedMemory::Create: successfully created an SPI memory object!");
#endif

            return instance_;
        }

        // Destroy an instance of SPISharedMemory.
        __declspec(noinline) static void Close() noexcept
        {
            if (!instance_)
            {
                return;
            }

            delete instance_->ConcretePtr;
            instance_->~SharedMemory();

            UnmapViewOfFile(instance_);
            CloseHandle(mapFile_);

            instance_ = nullptr;
        }
    };


    // Concrete implementation of ISharedProxyInterface.

#define SPI_ASSERT_MIN_VER(VERSION) do { if (getVersion_() < VERSION) { return SPIReturn::ErrorUnsupportedYet; } } while (0)

    class SharedProxyInterface
        : public ISharedProxyInterface
        , public NonCopyMovable
    {
    private:

        // Implementation-detail fields.

        SPI::SharedMemory* sharedDataPtr_;

        // Private methods.

        DWORD getVersion_() const noexcept
        {
            return sharedDataPtr_->Version;
        }

        DWORD isReleaseMode_() const noexcept
        {
            return 0 == strcmp(sharedDataPtr_->BuildMode, "RELEASE");
        }

    public:
        SharedProxyInterface(SPI::SharedMemory* sharedDataPtr)
            : NonCopyMovable()
            , sharedDataPtr_{ sharedDataPtr }
        {

        }

        // ISharedProxyInterface implementation.

        SPIDECL WaitForKickoff(int timeoutMs)
        {
            SPI_ASSERT_MIN_VER(2);
            return SPIReturn::ErrorUnsupportedYet;
        }

        SPIDECL InstallHook(ccstring name, LPVOID target, LPVOID detour, LPVOID* original)
        {
            SPI_ASSERT_MIN_VER(2);
            return SPIReturn::ErrorUnsupportedYet;
        }

        SPIDECL UninstallHook(ccstring name)
        {
            SPI_ASSERT_MIN_VER(2);
            return SPIReturn::ErrorUnsupportedYet;
        }

        // End of ISharedProxyInterface implementation.
    };
}

HANDLE SPI::SharedMemory::mapFile_;
SPI::SharedMemory* SPI::SharedMemory::instance_;
