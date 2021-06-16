#pragma once

#include <new>
#include <Windows.h>
#include "../conf/version.h"
#include "../utils/io.h"
#include "../dllstruct.h"

#include "interface.h"

namespace SPI
{
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
        ISPIPtr ConcretePtr;   // 0x34 (0x08)  ==  0x3C

        // ONLY ADD NEW MEMBERS AFTER THIS LINE

    private:

        static bool created_;
        static HANDLE mapFile_;
        static SharedMemory* bufferPtr_;

        SharedMemory(void* concretePtr)
            : Magic { "SPI" }
            , Version{ ASI_SPI_VERSION }
            , Size{ GetSize() }
            , BuildDate{__DATE__ " " __TIME__}
            , ConcretePtr{ reinterpret_cast<ISPIPtr>(concretePtr) }
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
        __declspec(noinline) static bool Create() noexcept
        {
            if (created_)
            {
                return true;
            }

            wchar_t fileMappingName[256];
            auto rc = ISharedProxyInterface::MakeMemoryName(fileMappingName, 256, (int)GLEBinkProxy.Game);
            if (rc != SPIReturn::Success)
            {
                GLogger.writeFormatLine(L"SPISharedMemory::Create: ERROR: failed to make memory mapping name, error = %d", static_cast<int>(rc));
                return false;
            }

            GLogger.writeFormatLine(L"SPISharedMemory::Create: creating a file mapping with name \"%s\"", fileMappingName);
            mapFile_ = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, GetSize(), fileMappingName);
            if (!mapFile_)
            {
                GLogger.writeFormatLine(L"SPISharedMemory::Create: ERROR: failed to create a file mapping, error code = %d", GetLastError());
                return false;
            }

            bufferPtr_ = (SharedMemory*)MapViewOfFile(mapFile_, FILE_MAP_ALL_ACCESS, 0, 0, GetSize());
            if (!bufferPtr_)
            {
                GLogger.writeFormatLine(L"SPISharedMemory::Create: ERROR: failed to map view of file, error code = %d", GetLastError());
                return false;
            }

            const auto& memoryPtr = new(bufferPtr_) SharedMemory(nullptr);

#ifdef ASI_DEBUG
            GLogger.writeFormatLine(L"SPISharedMemory::Create: successfully created an SPI memory object at %p!", memoryPtr);
#else
            GLogger.writeFormatLine(L"SPISharedMemory::Create: successfully created an SPI memory object!");
#endif
            return (created_ = true);
        }

        // Destroy an instance of SPISharedMemory.
        __declspec(noinline) static void Close() noexcept
        {
            if (!created_)
            {
                return;
            }

            bufferPtr_->~SharedMemory();

            UnmapViewOfFile(bufferPtr_);
            CloseHandle(mapFile_);

            created_ = false;
        }
    };
}

bool SPI::SharedMemory::created_;
HANDLE SPI::SharedMemory::mapFile_;
SPI::SharedMemory* SPI::SharedMemory::bufferPtr_;
