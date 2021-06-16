#pragma once

#include <new>
#include <Windows.h>
#include "utils/io.h"
#include "dllstruct.h"

namespace SPI
{
#pragma pack(1)
    class SharedMemory
    {
    public:

        // DO NOT REORDER OR CHANGE TYPES

        char BuildDate[32];
        char ReleaseMode[8];
        void* ConcretePtr;

        // total size of the struct is 0x30
        // ONLY ADD NEW MEMBERS AFTER THIS LINE

    private:

        static bool created_;
        static HANDLE mapFile_;
        static SharedMemory* bufferPtr_;

        inline static const wchar_t* gameToMemoryName_(LEGameVersion version) noexcept
        {
            switch (version)
            {
            case LEGameVersion::Launcher: return L"Local\\LELPROXSPI";
            case LEGameVersion::LE1: return L"Local\\LE1PROXSPI";
            case LEGameVersion::LE2: return L"Local\\LE2PROXSPI";
            case LEGameVersion::LE3: return L"Local\\LE3PROXSPI";
            default:
                return nullptr;
            }
        }

        SharedMemory(void* concretePtr)
            : BuildDate{ __DATE__ " " __TIME__ }
            , ConcretePtr{ concretePtr }
        {
#if defined(ASI_DEBUG) && !defined(NDEBUG)
            CopyMemory(ReleaseMode, "DEBUG", 6);
#elif !defined(ASI_DEBUG) && defined(NDEBUG)
            CopyMemory(ReleaseMode, "RELEASE", 8);
#else
#error INCONCLUSIVE RELEASE MODE PREPROCESSOR DEFINITIONS
#endif
        }

        // Get size of the memory block to allocate.
        [[nodiscard]] __forceinline static int GetSize() noexcept { return sizeof(SharedMemory); }

    public:

        // Create an instance of SPISharedMemory in... shared memory :pikawhoa:
        __declspec(noinline) static bool Create() noexcept
        {
            if (created_)
            {
                return true;
            }

            mapFile_ = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, GetSize(), gameToMemoryName_(GLEBinkProxy.Game));
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
