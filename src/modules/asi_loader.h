#pragma once

#include <vector>
#include <Windows.h>
#include "../utils/io.h"
#include "_base.h"

#include "../spi/interface.h"


typedef void(* AsiSpiSupportType)(wchar_t** name, wchar_t** author, int* gameIndex, int* attachMode, int* spiMinVersion);
typedef bool(* AsiSpiShouldPreloadType)(void);
typedef bool(* AsiOnAttachType)(ISharedProxyInterface* InterfacePtr);
typedef bool(* AsiOnDetachType)(void);

struct AsiPluginLoadInfo
{
private:
    bool shouldPreloadFetched_;

public:
    wchar_t* FileName;
    HINSTANCE LibInstance;
    AsiSpiSupportType SpiSupport;
    AsiSpiShouldPreloadType DoPreload;
    AsiOnAttachType OnAttach;
    AsiOnDetachType OnDetach;
    bool AllSpiProcsLoaded;

    wchar_t* PluginName;
    wchar_t* PluginAuthor;
    int SupportedGamesBitset;
    int AttachMode;
    int MinInterfaceVersion;

    [[nodiscard]] __forceinline bool SupportsSPI() const noexcept { return SpiSupport != nullptr && AllSpiProcsLoaded; }
    [[nodiscard]] __forceinline bool ShouldPreload()
    {
        if (!SupportsSPI())
        {
            return false;
        }

        static bool ranCall = false;
        if (!ranCall && DoPreload)
        {
            shouldPreloadFetched_ = DoPreload();
            ranCall = true;
        }

        if (!ranCall)
        {
            GLogger.writeFormatLine(L"ShouldPreload: fell through the call check, most likely DoPreload was NULL");
            return false;
        }

        return shouldPreloadFetched_;
    }
    [[nodiscard]] __forceinline bool ShouldPostload()
    {
        if (!SupportsSPI())
        {
            return false;
        }

        static bool ranCall = false;
        if (!ranCall && DoPreload)
        {
            shouldPreloadFetched_ = DoPreload();
            ranCall = true;
        }

        if (!ranCall)
        {
            GLogger.writeFormatLine(L"ShouldPostload: fell through the call check, most likely DoPreload was NULL");
            return false;
        }

        return !shouldPreloadFetched_;
    }

    [[nodiscard]] bool HasCorrectVersionFor(int proxyVer) const
    {
        return !(MinInterfaceVersion < 2 || MinInterfaceVersion > proxyVer);
    }
    [[nodiscard]] bool HasCorrectFlagFor(LEGameVersion gameVer) const
    {
        switch (gameVer)
        {
        case LEGameVersion::Launcher:
            if (SupportedGamesBitset & SPI_GAME_L) return true;
            return false;
        case LEGameVersion::LE1:
            if (SupportedGamesBitset & SPI_GAME_1) return true;
            return false;
        case LEGameVersion::LE2:
            if (SupportedGamesBitset & SPI_GAME_2) return true;
            return false;
        case LEGameVersion::LE3:
            if (SupportedGamesBitset & SPI_GAME_3) return true;
            return false;
        default:
            return false;
        }
    }

    void LoadConditionalProcs()
    {
        DoPreload = (AsiSpiShouldPreloadType)GetProcAddress(LibInstance, "SpiShouldPreload");
#ifdef ASI_DEBUG
        if (DoPreload == NULL)
        {
            AllSpiProcsLoaded = false;
            GLogger.writeFormatLine(L"LoadConditionalProcs: failed to find SpiShouldPreload (last error = %d)", GetLastError());
        }
#endif

        OnAttach = (AsiOnAttachType)GetProcAddress(LibInstance, "SpiOnAttach");
#ifdef ASI_DEBUG
        if (OnAttach == NULL)
        {
            AllSpiProcsLoaded = false;
            GLogger.writeFormatLine(L"LoadConditionalProcs: failed to find SpiOnAttach (last error = %d)", GetLastError());
        }
#endif

        OnDetach = (AsiOnDetachType)GetProcAddress(LibInstance, "SpiOnDetach");
#ifdef ASI_DEBUG
        if (OnDetach == NULL)
        {
            AllSpiProcsLoaded = false;
            GLogger.writeFormatLine(L"LoadConditionalProcs: failed to find SpiOnDetach (last error = %d)", GetLastError());
        }
#endif
    }
};

class AsiLoaderModule
    : public IModule
{
private:
    typedef wchar_t* wstr;
    typedef const wchar_t* wcstr;

    // Config parameters.

    static const int MAX_FILES = 128;       // Max. number of ASI plugins in the directory.
    static const bool TRY_LOAD_ALL = true;  // Attempt to load further ASIs after an error on loading one.

    // Fields.

    int fileCount_ = 0;
    wchar_t fileNames_[MAX_PATH][MAX_FILES];
    DWORD lastErrorCode_ = 0;
    std::vector<AsiPluginLoadInfo> pluginLoadInfos_;

    // Methods.

    bool directoryExists_(wcstr name) const
    {
        auto attributes = GetFileAttributesW(name);
        if (attributes == INVALID_FILE_ATTRIBUTES)
        {
            return false;
        }
        return static_cast<bool>(attributes & FILE_ATTRIBUTE_DIRECTORY);
    }

    bool findPluginFiles_()
    {
        WIN32_FIND_DATA fd;
        HANDLE findHandle = ::FindFirstFile(L"ASI/*.asi", &fd);
        if (findHandle == INVALID_HANDLE_VALUE)
        {
            return (this->lastErrorCode_ = GetLastError()) == ERROR_FILE_NOT_FOUND;  // It's not a true error if we can't find anything.
        }

        do
        {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                wcscpy(this->fileNames_[this->fileCount_++], fd.cFileName);
            }
        } while (::FindNextFile(findHandle, &fd));
        ::FindClose(findHandle);

        return true;
    }

    bool registerLoadInfo_(HINSTANCE dllModuleInstance, wchar_t* fileName)
    {
        AsiPluginLoadInfo loadInfo{};

        loadInfo.FileName = fileName;
        loadInfo.LibInstance = dllModuleInstance;
        loadInfo.AllSpiProcsLoaded = true;

        loadInfo.SpiSupport = (AsiSpiSupportType)GetProcAddress(dllModuleInstance, "SpiSupportDecl");
        if (loadInfo.SpiSupport == NULL)
        {
            loadInfo.AllSpiProcsLoaded = false;
            GLogger.writeFormatLine(L"registerLoadInfo_: failed to find SpiSupportDecl (last error = %d). Likely, SPI is just not supported.", GetLastError());
            // not an error
        }

        // If the plugins declares SPI support, attempt to load other required procedures.
        if (loadInfo.SupportsSPI())
        {
            loadInfo.LoadConditionalProcs();
            if (!loadInfo.AllSpiProcsLoaded)
            {
                GLogger.writeFormatLine(L"registerLoadInfo_: SpiSupportDecl was found but some procs are missing:");
                GLogger.writeFormatLine(L"registerLoadInfo_: SpiSupportDecl = %p, SpiShouldPreload = %p, SpiOnAttach = %p, SpiOnDetach = %p",
                    loadInfo.SpiSupport, loadInfo.DoPreload, loadInfo.OnAttach, loadInfo.OnDetach);
                return false;
            }
            GLogger.writeFormatLine(L"registerLoadInfo_: all SPI procs were found!");

            // Get SPI-required info from the plugin via SpiSupportDecl.
            loadInfo.SpiSupport(&loadInfo.PluginName, &loadInfo.PluginAuthor, &loadInfo.SupportedGamesBitset, &loadInfo.AttachMode, &loadInfo.MinInterfaceVersion);
            GLogger.writeFormatLine(L"registerLoadInfo_: provided info: '%s' by '%s', supported games (bitset) = %d, attachMode = %d, min ver = %d",
                loadInfo.PluginName, loadInfo.PluginAuthor, loadInfo.SupportedGamesBitset, loadInfo.AttachMode, loadInfo.MinInterfaceVersion);

            // Ensure that the plugin's declared min SPI version is valid and less or equal to our version.
            if (!loadInfo.HasCorrectVersionFor(ASI_SPI_VERSION))
            {
                GLogger.writeFormatLine(L"registerLoadInfo_: filtering out because the min version is higher than the build's one (%d)!", loadInfo.MinInterfaceVersion);
                return false;
            }

            // Ensure that the plugin's declared game targets match the game we (the proxy) are attached to.
            if (!loadInfo.HasCorrectFlagFor(GLEBinkProxy.Game))
            {
                GLogger.writeFormatLine(L"registerLoadInfo_: filtering out because the plugin was not designed for this game (need to have %d)!", GLEBinkProxy.Game);
                return false;
            }
        }

        pluginLoadInfos_.push_back(loadInfo);
        return true;
    }

public:
    AsiLoaderModule()
        : IModule{ "AsiLoader" }
        , pluginLoadInfos_{}
    {
        active_ = true;
    }

    bool Activate() override
    {
        for (int f = 0; f < MAX_FILES; f++)
        {
            memset(this->fileNames_[f], 0, MAX_PATH - 4);
        }

        // Bail out early if the directory doesn't even exist.
        if (!this->directoryExists_(L"ASI"))
        {
            return true;
        }

        // Report an error if the file search fails (no matches is not a failure).
        if (!this->findPluginFiles_())
        {
            GLogger.writeFormatLine(L"AsiLoaderModule.Activate: aborting after findPluginFiles_ (error code = %d).", this->lastErrorCode_);
            return false;
        }

        // For each found files, load the library and register SPI info.
        wchar_t fileNameBuffer[MAX_PATH];
        HINSTANCE lastModule = nullptr;
        for (int f = 0; f < this->fileCount_; f++)
        {
            GLogger.writeFormatLine(L"AsiLoaderModule.Activate: loading %s...", this->fileNames_[f]);

            wsprintf(fileNameBuffer, L"ASI/%s", this->fileNames_[f]);

            // Load the DLL file.
            // DllMain() code will be executed here, SPI will be executed later, from dllmain.cpp:OnAttach().
            if (NULL == (lastModule = LoadLibraryW(fileNameBuffer)))
            {
                GLogger.writeFormatLine(L"AsiLoaderModule.Activate:   failed with error code = %d.", (this->lastErrorCode_ = GetLastError()));
                if (TRY_LOAD_ALL) continue;
                return false;
            }

            // Get SPI info from the plugins.
            // This will populate pluginLoadInfos_ with data needed for executing plugins' attach points.
            if (!this->registerLoadInfo_(lastModule, this->fileNames_[f]))
            {
                GLogger.writeFormatLine(L"AsiLoaderModule.Activate:   registerLoadInfo_ failed.");
                if (TRY_LOAD_ALL) continue;
                return false;
            }

            // DEBUG THING
            GLogger.writeFormatLine(L"AsiLoaderModule.Activate:   successfully registered the load info.");
            // END DEBUG THING
        }

        return true;
    }
    void Deactivate() override
    {
        // maybe force-unload the ASIs?
    }

    bool PreLoad(ISharedProxyInterface* interfacePtr)
    {
        for (auto& loadInfo : pluginLoadInfos_)
        {
            if (loadInfo.ShouldPreload())
            {
                if (!loadInfo.OnAttach(interfacePtr))
                {
                    GLogger.writeFormatLine(L"PostLoad: OnAttach returned an error [%s]", loadInfo.FileName);
                    continue;
                }

                GLogger.writeFormatLine(L"PostLoad: OnAttach succeeded [%s]", loadInfo.FileName);
            }
        }

        return true;
    }
    bool PostLoad(ISharedProxyInterface* interfacePtr)
    {
        for (auto& loadInfo : pluginLoadInfos_)
        {
            if (loadInfo.ShouldPostload())
            {
                if (!loadInfo.OnAttach(interfacePtr))
                {
                    GLogger.writeFormatLine(L"PostLoad: OnAttach returned an error [%s]", loadInfo.FileName);
                }
            }
        }

        return true;
    }
};
