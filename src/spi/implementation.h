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

        // ONLY ADD NEW MEMBERS AFTER THIS LINE
        // ONLY ADD NEW MEMBERS BEFORE THIS LINE

        // THIS SHOULD BE THE LAST MEMBER OF THE STRUCT
        // Offset can be calculated as Size - 8.
        void* ConcretePtr;     // 0x34 (0x08)  ==  0x3C

    private:

        static HANDLE mapFile_;
        static SharedMemory* instance_;

        SharedMemory(void* concretePtr)
            : Magic { "SPI" }
            , Version{ ASI_SPI_VERSION }
            , Size{ getSize_() }
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

        // Get size of the memory block.
        [[nodiscard]] __forceinline static unsigned int getSize_() noexcept { return sizeof(SharedMemory); }

        // Get size of the allocated memory block.
        // WARNING: DO NOT CHANGE WITHOUT CHANGING THE INTERFACE!!!
        [[nodiscard]] __forceinline static unsigned int getAllocatedSize_() noexcept { return 0x100; }

        // Get the expected name of a shared memory block for the process.
        // WARNING: DO NOT CHANGE WITHOUT CHANGING THE INTERFACE!!!
        [[nodiscard]] __forceinline static void getMemoryName_(wchar_t* buffer, size_t length) noexcept
        {
            swprintf(buffer, length, L"Local\\LE%dPROXSPI_%d", static_cast<int>(GLEBinkProxy.Game), GetCurrentProcessId());
        }

    public:

        // Create an instance of SPISharedMemory in... shared memory :pikawhoa:
        __declspec(noinline) static SharedMemory* Create() noexcept
        {
            if (instance_)
            {
                return instance_;
            }

            wchar_t fileMappingName[256];
            getMemoryName_(fileMappingName, 256);

            GLogger.writeFormatLine(L"SPISharedMemory::Create: creating a file mapping with name \"%s\"", fileMappingName);
            mapFile_ = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_EXECUTE_READWRITE, 0, getAllocatedSize_(), fileMappingName);
            if (!mapFile_)
            {
                GLogger.writeFormatLine(L"SPISharedMemory::Create: ERROR: failed to create a file mapping, error code = %d", GetLastError());
                return nullptr;
            }

            instance_ = (SharedMemory*)MapViewOfFile(mapFile_, FILE_MAP_ALL_ACCESS | FILE_MAP_EXECUTE, 0, 0, getAllocatedSize_());
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

#define SPI_ASSERT_MIN_VER(VERSION) do { if (getVersion_() < VERSION) { return SPIReturn::FailureUnsupportedYet; } } while (0)

    class SharedProxyInterface
        : public ISharedProxyInterface
        , public NonCopyMovable
    {
    private:

        // Implementation-detail fields.

        SPI::SharedMemory* sharedDataPtr_;
        int consoleStateCounter_;

        // Private methods.

        DWORD getVersion_() const noexcept
        {
            if (nullptr == sharedDataPtr_)
            {
                GLogger.writeFormatLine(L"SharedProxyInterface.getVersion_: ERROR: sharedDataPtr_ is NULL!");
                MessageBoxW(nullptr, L"sharedDataPtr_ is NULL!", L"LEBinkProxy error: SPI", MB_OK | MB_ICONERROR | MB_APPLMODAL);
                return -1;
            }

            return sharedDataPtr_->Version;
        }

        DWORD isReleaseMode_() const noexcept
        {
            if (nullptr == sharedDataPtr_)
            {
                GLogger.writeFormatLine(L"SharedProxyInterface.isReleaseMode_: ERROR: sharedDataPtr_ is NULL!");
                MessageBoxW(nullptr, L"sharedDataPtr_ is NULL!", L"LEBinkProxy error: SPI", MB_OK | MB_ICONERROR | MB_APPLMODAL);
                return -1;
            }

            return 0 == strcmp(sharedDataPtr_->BuildMode, "RELEASE");
        }

    public:
        SharedProxyInterface(SPI::SharedMemory* sharedDataPtr)
            : NonCopyMovable()
            , sharedDataPtr_{ sharedDataPtr }
            , consoleStateCounter_{ 0 }
        {

        }

        // ISharedProxyInterface implementation.

        SPIDEFN GetVersion(DWORD* outVersionPtr)
        {
            if (!sharedDataPtr_)
            {
                *outVersionPtr = 0;
                return SPIReturn::ErrorSharedMemoryUnassigned;
            }

            *outVersionPtr = sharedDataPtr_->Version;
            return SPIReturn::Success;
        }

        SPIDEFN OpenSharedConsole(FILE* outStream, FILE* errStream)
        {
            ++consoleStateCounter_;

            if (consoleStateCounter_ == 1)
            {
                Utils::OpenConsole(outStream, errStream);
            }

            return SPIReturn::Success;
        }

        SPIDEFN CloseSharedConsole()
        {
            --consoleStateCounter_;

            if (consoleStateCounter_ == 0)
            {
                Utils::CloseConsole();
            }

            return SPIReturn::Success;
        }

        SPIDEFN WaitForDRM(int timeoutMs)
        {
            return SPIReturn::FailureUnsupportedYet;
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

HANDLE SPI::SharedMemory::mapFile_;
SPI::SharedMemory* SPI::SharedMemory::instance_;
