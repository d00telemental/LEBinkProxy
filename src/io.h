#pragma once
#include "dllincludes.h"

// Input/output utilities.
namespace IO
{
    FILE* FGLog = nullptr;

    // Open a console window and redirect output to it.
    // In release mode, redirect all output to a file.
    void SetupOutput()
    {
#ifdef ASI_DEBUG
#define ASIOUT stdout

        AllocConsole();
        freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
        freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);

        HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo;
        GetConsoleScreenBufferInfo(console, &lpConsoleScreenBufferInfo);
        SetConsoleScreenBufferSize(console, { lpConsoleScreenBufferInfo.dwSize.X, 30000 });
#else
#define ASIOUT FGLog

		FGLog = fopen("bink2w64_proxy.log", "w");
        if (FGLog == NULL)
        {
            wchar_t errorBuffer[256];
            wsprintf(errorBuffer, L"Failed to create / open a log file.\r\nError code: %d.", GetLastError());
            MessageBoxW(NULL, errorBuffer, L"Bink proxy error", MB_OK | MB_ICONERROR | MB_TOPMOST);
            exit(-1);
        }
#endif
    }

    // Close the console window.
    void TeardownOutput()
    {
#ifdef ASI_DEBUG
        FreeConsole();
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

	public:

		int writeLine(wchar_t* line)
		{
			int rv = writeLineInternal(ASIOUT, line);
			fflush(ASIOUT);
			return rv;
		}
		int writeFormatLine(wchar_t* fmt_line, ...)
		{
			int rc;
			wchar_t buffer[1024];

			va_list args;
			va_start(args, fmt_line);
			rc = _vsnwprintf(buffer, 1024, fmt_line, args);
			va_end(args);

			if (rc < 0)
			{
				writeLine(L"writeFormatLine: vsnprintf encoding error");
				return -1;
			}
			else if (rc == 0)
			{
				writeLine(L"writeFormatLine: vsnprintf runtime constraint violation");
				return -1;
			}

			return writeLine(buffer);
		}
	};

	static struct RuntimeLogger GLogger;
}
