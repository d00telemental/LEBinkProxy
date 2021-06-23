#pragma once

#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>

#include <cstdio>
#include <cstring>

#include <mutex>
#include <thread>


#ifndef ASI_LOG_FNAME
#error Must set ASI log filename!
#endif


namespace Utils
{
    FILE* FGLog = nullptr;
	std::mutex GOpenConsoleMtx;
	std::mutex GCloseConsoleMtx;
	std::mutex GWriteLineMtx;

	void OpenConsole(FILE* out, FILE* err)
	{
		const std::lock_guard<std::mutex> lock(GOpenConsoleMtx);

		AllocConsole();

		freopen_s((FILE**)out, "CONOUT$", "w", out);
		freopen_s((FILE**)err, "CONOUT$", "w", err);

		HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo;
		GetConsoleScreenBufferInfo(console, &lpConsoleScreenBufferInfo);
		SetConsoleScreenBufferSize(console, { lpConsoleScreenBufferInfo.dwSize.X, 30000 });
	}
	void CloseConsole()
	{
		const std::lock_guard<std::mutex> lock(GCloseConsoleMtx);
		FreeConsole();
	}

    // Open a console window and redirect output to it.
    // In release mode, redirect all output to a file.
    void SetupOutput()
    {
#ifdef ASI_DEBUG
#define ASIOUT stdout
		OpenConsole(stdout, stderr);
#else
#define ASIOUT FGLog

		FGLog = fopen(ASI_LOG_FNAME, "w");
        if (FGLog == NULL)
        {
            wchar_t errorBuffer[512];
            wsprintf(errorBuffer, L"Failed to create / open a log file.\r\nError code: %d.\r\nSince this is a release build, no diagnostics would be possible.", GetLastError());
            MessageBoxW(NULL, errorBuffer, L"Bink proxy error", MB_OK | MB_ICONERROR | MB_TOPMOST);
        }
#endif
    }

    // Close the console window.
    void TeardownOutput()
    {
#ifdef ASI_DEBUG
		CloseConsole();
#else
        if (FGLog != nullptr)
        {
            fclose(FGLog);
        }
#endif
    }

	struct RuntimeLogger
	{
	private:
		bool initialized = false;
		SYSTEMTIME localTime;

		int writeLineInternal(FILE* stream, wchar_t* line)
		{
			//return fwprintf(stream, L"%s\n", line);
			GetLocalTime(&localTime);
			return fwprintf(stream, L"%02d:%02d:%02d.%03d  %s\n", localTime.wHour, localTime.wMinute, localTime.wSecond, localTime.wMilliseconds, line);
		}

		int writeLineAndFlush(wchar_t* line)
		{
			int rv = writeLineInternal(ASIOUT, line);
			fflush(ASIOUT);
			return rv;
		}

	public:
		int writeln(wchar_t* fmt_line, ...)
		{
			const std::lock_guard<std::mutex> lock(GWriteLineMtx);

			int rc;
			wchar_t buffer[1024];

			va_list args;
			va_start(args, fmt_line);
			rc = _vsnwprintf(buffer, 1024, fmt_line, args);
			va_end(args);

			if (rc < 0)
			{
				writeLineAndFlush(L"writeln: vsnprintf encoding error");
				return -1;
			}
			else if (rc == 0)
			{
				writeLineAndFlush(L"writeln: vsnprintf runtime constraint violation");
				return -1;
			}

			return writeLineAndFlush(buffer);
		}
	};
}

// Global instance.

static Utils::RuntimeLogger GLogger;
