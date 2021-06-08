#pragma once
#include "dllincludes.h"
#include "io.h"

#include <string>
#include <vector>

#define ASILOADER_MAX_FILES 128
#define ASILOADER_DIR_NAME L"ASI/"

// ASI loader.
namespace Loader
{
    wchar_t FileNames[MAX_PATH][ASILOADER_MAX_FILES];
    int FileCount = 0;

    BOOL DirectoryExists(const wchar_t* name)
    {
        DWORD attributes = GetFileAttributesW(name);
        if (attributes == INVALID_FILE_ATTRIBUTES)
        {
            return false;
        }
        return (attributes & FILE_ATTRIBUTE_DIRECTORY);
    }

    bool FindFiles()
    {
        WIN32_FIND_DATA fd;
        HANDLE hFind = ::FindFirstFile(ASILOADER_DIR_NAME L"*.asi", &fd);
        if (hFind == INVALID_HANDLE_VALUE)
        {
            GLogger.writeFormatLine(L"LoadAllASIs: FindFirstFile failed! No files to find, hopefully?");
            return false;
        }

        do
        {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                wcscpy(FileNames[FileCount++], fd.cFileName);
            }
        } while (::FindNextFile(hFind, &fd));
        ::FindClose(hFind);

        return true;
    }

    bool LoadAllASIs()
    {
        // Check that ASI dir exists.
        if (!DirectoryExists(ASILOADER_DIR_NAME))
        {
            GLogger.writeFormatLine(L"LoadAllASIs: not loading anything because " ASILOADER_DIR_NAME " doesn't exist.");
            return true;  // this is not technically an error.
        }


        // Find all .asi files in it.
        if (!FindFiles())
        {
            GLogger.writeFormatLine(L"LoadAllASIs: aborting.");
            return false;
        }


        // Load every file as a library.
        bool noErrors = true;
        wchar_t buffer[MAX_PATH];
        for (int i = 0; i < FileCount; i++)
        {
            GLogger.writeFormatLine(L"LoadAllASIs: loading %s", FileNames[i]);
            wsprintf(buffer, L"%s%s", ASILOADER_DIR_NAME, FileNames[i]);
            if (NULL == LoadLibraryW(buffer))
            {
                GLogger.writeFormatLine(L"LoadAllASIs:   failed with code %d!", GetLastError());
                noErrors = false;
            }
        }

        return noErrors;
    }
}
