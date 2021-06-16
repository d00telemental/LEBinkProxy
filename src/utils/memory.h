#pragma once

#include <Windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include "../utils/io.h"


namespace Utils
{
    bool IsExecutableAddress(LPVOID pAddress)
    {
        MEMORY_BASIC_INFORMATION mi;
        VirtualQuery(pAddress, &mi, sizeof(mi));
        return (mi.State == MEM_COMMIT && (mi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)));
    }

    HMODULE GetGameModuleRange(BYTE** pStart, BYTE** pEnd)
    {
        HMODULE exeModule = GetModuleHandle(nullptr);
        MODULEINFO exeModuleInfo;
        if (!GetModuleInformation(GetCurrentProcess(), exeModule, &exeModuleInfo, sizeof(MODULEINFO)))
        {
            return nullptr;
        }

        *pStart = (BYTE*)exeModuleInfo.lpBaseOfDll;
        *pEnd = *pStart + exeModuleInfo.SizeOfImage;
        return exeModule;
    }

    /// <summary>
    /// Scan the game module for a sequence of bytes defined by a pattern and a mask.
    /// TODO: refactor it to use PEiD patterns.
    /// </summary>
    BYTE* ScanProcess(BYTE* pattern, BYTE* mask)
    {
        size_t patternLength = strlen((char*)mask);

        BYTE* start, * end, * pointer;
        if (!GetGameModuleRange(&start, &end))
        {
            GLogger.writeFormatLine(L"ScanProcess: ERROR: GetGameModuleRange failed.");
            return nullptr;
        }

        pointer = start;
        while (pointer < end)
        {
            for (size_t matchLength = 0; matchLength < patternLength; matchLength++)
            {
                if (matchLength + 1 == patternLength)
                {
                    return pointer;
                }
                if (pointer + matchLength >= end)
                {
                    return nullptr;
                }
                if ((pattern[matchLength] != pointer[matchLength]) && (mask[matchLength] != '?'))
                {
                    break;
                }
            }

            pointer++;
        }
        return nullptr;
    }


    /// <summary>
    /// Object which freezes all but the current thread for the duration of the scope.
    /// </summary>
    class ScopedThreadFreeze
    {
    private:

        // Private methods which do the heavy lifting.

        void suspendAllOtherThreads_()
        {
            DWORD currentThreadId = GetCurrentThreadId();
            DWORD currentProcessId = GetCurrentProcessId();

            HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
            if (h != INVALID_HANDLE_VALUE)
            {
                THREADENTRY32 te;
                te.dwSize = sizeof(te);
                if (Thread32First(h, &te))
                {
                    do
                    {
                        if (te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(te.th32OwnerProcessID))
                        {
                            if (te.th32OwnerProcessID == currentProcessId && te.th32ThreadID != currentThreadId)
                            {
                                //printf_s("Suspending pid 0x%04x tid 0x%04x\n", te.th32OwnerProcessID, te.th32ThreadID);
                                HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
                                if (thread == NULL)
                                {
                                    GLogger.writeFormatLine(L"SuspendAllOtherThreads: failed to open thread.");
                                }
                                else
                                {
                                    if (SuspendThread(thread) == -1)
                                    {
                                        GLogger.writeFormatLine(L"SuspendAllOtherThreads: failed to suspend thread.");
                                    }
                                    else
                                    {
                                        CloseHandle(thread);
                                    }
                                }
                            }
                        }

                        te.dwSize = sizeof(te);
                    } while (Thread32Next(h, &te));
                }
                CloseHandle(h);
            }

            GLogger.writeLine(L"SuspendAllOtherThreads: returning.");
        }
        void resumeAllOtherThreads_()
        {
            DWORD currentThreadId = GetCurrentThreadId();
            DWORD currentProcessId = GetCurrentProcessId();

            HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
            if (h != INVALID_HANDLE_VALUE)
            {
                THREADENTRY32 te;
                te.dwSize = sizeof(te);
                if (Thread32First(h, &te))
                {
                    do
                    {
                        if (te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(te.th32OwnerProcessID))
                        {
                            if (te.th32OwnerProcessID == currentProcessId && te.th32ThreadID != currentThreadId)
                            {
                                //printf("Resuming pid 0x%04x tid 0x%04x\n", te.th32OwnerProcessID, te.th32ThreadID);
                                HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
                                if (thread == NULL)
                                {
                                    GLogger.writeFormatLine(L"ResumeAllOtherThreads: failed to open thread.");
                                }
                                else
                                {
                                    if (ResumeThread(thread) == -1)
                                    {
                                        GLogger.writeFormatLine(L"ResumeAllOtherThreads: failed to resume thread.");
                                    }
                                    else
                                    {
                                        CloseHandle(thread);
                                    }
                                }
                            }
                        }

                        te.dwSize = sizeof(te);
                    } while (Thread32Next(h, &te));
                }
                CloseHandle(h);
            }

            GLogger.writeLine(L"ResumeAllOtherThreads: returning.");
        }

    public:

        // Disable copying and moving.

        ScopedThreadFreeze(const ScopedThreadFreeze& other) = delete;
        ScopedThreadFreeze(ScopedThreadFreeze&& other) = delete;
        ScopedThreadFreeze& operator=(const ScopedThreadFreeze& other) = delete;
        ScopedThreadFreeze& operator=(ScopedThreadFreeze&& other) = delete;

        // RAII logic.

        ScopedThreadFreeze()
        {
            suspendAllOtherThreads_();
        }
        ~ScopedThreadFreeze()
        {
            resumeAllOtherThreads_();
        }
    };
}
