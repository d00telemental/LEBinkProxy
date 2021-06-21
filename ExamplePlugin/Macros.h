#pragma once

#include <Windows.h>

// Utility macros for reporting interface errors.
// Obviously, not required for the SPI to work.
#define SYMCONCAT_INNER(X, Y) X##Y
#define SYMCONCAT(X, Y) SYMCONCAT_INNER(X, Y)
#define RETURN_ON_SPI_ERROR_IMPL(MSG,ERR,FILE,LINE) do { \
    wchar_t SYMCONCAT(temp, LINE)[1024]; \
    swprintf_s(SYMCONCAT(temp, LINE), 1024, L"%s\r\nCode: %d\r\nLocation: %S:%d", MSG, ERR, FILE, LINE); \
    MessageBoxW(nullptr, SYMCONCAT(temp, LINE), PLUGIN_NAME " SPI error", MB_OK | MB_ICONERROR | MB_APPLMODAL | MB_TOPMOST); \
    return false; \
} while (0)
#define RETURN_ON_SPI_ERROR(MSG,ERR) RETURN_ON_SPI_ERROR_IMPL(MSG, ERR, __FILE__, __LINE__);
