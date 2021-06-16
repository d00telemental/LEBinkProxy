#pragma once

#include <Windows.h>
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

public:
    AsiLoaderModule()
        : IModule{ "AsiLoader" }
    {
        active_ = true;
    }

    bool Activate() override
    {
        for (int f = 0; f < MAX_FILES; f++)
        {
            memset(this->fileNames_[f], 0, MAX_PATH);
        }

        // Bail out early if the directory doesn't even exist.
        if (!this->directoryExists_(L"ASI"))
        {
            return true;
        }

        if (!this->findPluginFiles_())
        {
            GLogger.writeFormatLine(L"AsiLoaderModule.Activate: aborting (error code = %d).", this->lastErrorCode_);
            return false;
        }

        wchar_t fileNameBuffer[MAX_PATH];
        for (int f = 0; f < this->fileCount_; f++)
        {
            GLogger.writeFormatLine(L"AsiLoaderModule.Activate: loading %s", this->fileNames_[f]);

            wsprintf(fileNameBuffer, L"ASI/%s", this->fileNames_[f]);
            if (nullptr == LoadLibraryW(fileNameBuffer))
            {
                this->lastErrorCode_ = GetLastError();
                GLogger.writeFormatLine(L"AsiLoaderModule.Activate:   failed with error code = %d", this->lastErrorCode_);

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
