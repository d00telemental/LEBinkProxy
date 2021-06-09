#pragma once

#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>

#include <cstdio>
#include <cstring>
#include "../minhook/include/MinHook.h"

#include "utils/io.h"
#include "utils/event.h"

#include "conf/patterns.h"
#include "conf/version.h"
