#pragma once

#define LEBINKPROXY_VERSION  L"1.1.0.0"
#define LEBINKPROXY_BUILDTM  __DATE__ " " __TIME__

#ifdef ASI_DEBUG
#define LEBINKPROXY_BUILDMD  L"DEBUG"
#else
#define LEBINKPROXY_BUILDMD  L"RELEASE"
#endif
