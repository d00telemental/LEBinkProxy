#pragma once
#include "../dllincludes.h"
#include "../utils/io.h"

#include "_base.h"


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
        // Bail out early if the directory doesn't even exist.
        if (!directoryExists_(L"ASI"))
        {
            return true;
        }

        WIN32_FIND_DATA fd;
        HANDLE findHandle = ::FindFirstFile(L"ASI/*.asi", &fd);
        if (findHandle == INVALID_HANDLE_VALUE)
        {
            lastErrorCode_ = GetLastError();
            return lastErrorCode_ == ERROR_FILE_NOT_FOUND;  // If the file can't be found, it's not a true error.
        }

        do
        {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                wcscpy(fileNames_[fileCount_++], fd.cFileName);
            }
        } while (::FindNextFile(findHandle, &fd));
        ::FindClose(findHandle);

        return true;
    }

public:
    AsiLoaderModule()
        : IModule{ "AsiLoader" }
    {
        for (int f = 0; f < MAX_FILES; f++)
        {
            memset(fileNames_[f], 0, MAX_PATH);
        }

        active_ = true;
    }

    bool Activate() override
    {
        auto rc = findPluginFiles_();
        if (!rc)
        {
            GLogger.writeFormatLine(L"AsiLoaderModule.Activate: aborting (error code = %d).", lastErrorCode_);
            return false;
        }

        wchar_t fileNameBuffer[MAX_PATH];
        for (int f = 0; f < fileCount_; f++)
        {
            GLogger.writeFormatLine(L"AsiLoaderModule.Activate: loading %s", fileNames_[f]);

            wsprintf(fileNameBuffer, L"ASI/%s", fileNames_[f]);
            if (nullptr == LoadLibraryW(fileNameBuffer))
            {
                lastErrorCode_ = GetLastError();
                GLogger.writeFormatLine(L"AsiLoaderModule.Activate:   failed with error code = %d", lastErrorCode_);

                if (!TRY_LOAD_ALL)
                {
                    return false;
                }
            }
        }

        return true;
    }

    void Deactivate() override
    {
        // maybe force-unload the ASIs?
    }
};
