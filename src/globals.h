#pragma once
#include "proxy.h"
#include "dllincludes.h"

#include "drm.h"
#include "io.h"
#include "launcher.h"
#include "memory.h"
#include "modules.h"
#include "patterns.h"
#include "proxy_info.h"
#include "ue_types.h"

#include "modules/asi_loader.h"
#include "modules/launcher_args.h"


static AppProxyInfo GAppProxyInfo;   // game version, executable path, window title
static ModuleList<64> GModules;  // console enabler, asi loader, launcher arg tool are all registered as modules
