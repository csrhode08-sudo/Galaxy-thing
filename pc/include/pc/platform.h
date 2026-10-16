// Galaxy PC platform layer: shared state and logging helpers.
#pragma once

#include <cstdio>

namespace pc {
extern bool IsRunning;
extern bool IsShuttingDown;
}  // namespace pc

#ifdef __cplusplus
extern "C" {
#endif
void OSSetCurrentThreadName(const char* name);
void pc_log(const char* fmt, ...);
#ifdef __cplusplus
}
#endif

// Logs the first call of an unimplemented SDK function.
#define STUB_LOG()                                                   \
    do {                                                             \
        static bool _stub_logged = false;                            \
        if (!_stub_logged) {                                         \
            _stub_logged = true;                                     \
            pc_log("[stub] %s (%s:%d)\n", __func__, __FILE__, __LINE__); \
        }                                                            \
    } while (0)

#define ZoneScoped
