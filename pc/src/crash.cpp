// Log file and crash reporting.
//
// Everything the game prints also goes to galaxy-log.txt (next to the executable, or in the user folder if that
// isn't writable). On Windows an unhandled exception or abort writes the exception and a module-relative stack
// trace to the log, then keeps the console open so the player can read or screenshot it.

#include <cstdarg>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace pc {

namespace {
std::FILE* sLogFile = nullptr;
std::recursive_mutex sLogMutex;
bool sCrashed = false;
}  // namespace

void openLogFile(const std::string& exeDir, const std::string& userDir) {
    for (const std::string& dir : {exeDir, userDir}) {
        if (dir.empty()) continue;
        std::string path = dir;
        if (path.back() != '/' && path.back() != '\\') path += '/';
        path += "galaxy-log.txt";
        sLogFile = std::fopen(path.c_str(), "w");
        if (sLogFile != nullptr) {
            std::fprintf(stderr, "[main] Log file: %s\n", path.c_str());
            return;
        }
    }
}

void logV(const char* fmt, va_list args) {
    std::lock_guard lock(sLogMutex);
    va_list copy;
    va_copy(copy, args);
    std::vfprintf(stderr, fmt, args);
    if (sLogFile != nullptr) {
        std::vfprintf(sLogFile, fmt, copy);
        std::fflush(sLogFile);
    }
    va_end(copy);
}

void log(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    logV(fmt, args);
    va_end(args);
}

#ifdef _WIN32
namespace {

void waitForPlayer() {
    std::fprintf(stderr, "\nThe game crashed. A copy of this window is in galaxy-log.txt next to the game.\n"
                         "Press Enter to close.\n");
    std::fflush(stderr);
    HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
    if (in != nullptr && in != INVALID_HANDLE_VALUE) {
        char buf[16];
        DWORD read = 0;
        ReadConsoleA(in, buf, sizeof(buf), &read, nullptr);
    }
}

void printStack(CONTEXT* ctx) {
    HMODULE exe = GetModuleHandleA(nullptr);
    void* frames[48];
    USHORT n = RtlCaptureStackBackTrace(0, 48, frames, nullptr);
    log("[crash] stack (module+offset):\n");
    auto printAddr = [&](void* addr) {
        HMODULE mod = nullptr;
        char name[MAX_PATH] = "?";
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               static_cast<LPCSTR>(addr), &mod) &&
            mod != nullptr) {
            GetModuleFileNameA(mod, name, sizeof(name));
            const char* base = std::strrchr(name, '\\');
            log("  %s+0x%llx%s\n", base ? base + 1 : name,
                (unsigned long long)(reinterpret_cast<uintptr_t>(addr) - reinterpret_cast<uintptr_t>(mod)),
                mod == exe ? "" : "");
        } else {
            log("  %p\n", addr);
        }
    };
    if (ctx != nullptr) {
        log("  at:\n");
        printAddr(reinterpret_cast<void*>(ctx->Rip));
    }
    for (USHORT i = 0; i < n; i++) {
        printAddr(frames[i]);
    }
}

LONG WINAPI onUnhandledException(EXCEPTION_POINTERS* info) {
    std::lock_guard lock(sLogMutex);
    if (sCrashed) {
        return EXCEPTION_EXECUTE_HANDLER;
    }
    sCrashed = true;
    const EXCEPTION_RECORD* rec = info->ExceptionRecord;
    log("\n[crash] exception 0x%08lx at %p", rec->ExceptionCode, rec->ExceptionAddress);
    if (rec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && rec->NumberParameters >= 2) {
        log(" (%s address %p)", rec->ExceptionInformation[0] == 0 ? "reading" : "writing",
            reinterpret_cast<void*>(rec->ExceptionInformation[1]));
    }
    if (rec->ExceptionCode == EXCEPTION_STACK_OVERFLOW) {
        log(" (stack overflow)");
    }
    log(" on thread %lu\n", GetCurrentThreadId());
    printStack(info->ContextRecord);
    waitForPlayer();
    return EXCEPTION_EXECUTE_HANDLER;
}

void onAbort(int) {
    std::lock_guard lock(sLogMutex);
    if (sCrashed) {
        return;
    }
    sCrashed = true;
    log("\n[crash] abort on thread %lu\n", GetCurrentThreadId());
    printStack(nullptr);
    waitForPlayer();
}

}  // namespace
#endif

void onThreadStart() {
#ifdef _WIN32
    ULONG guarantee = 64 * 1024;
    SetThreadStackGuarantee(&guarantee);
#endif
}

void installCrashHandler() {
#ifdef _WIN32
    SetUnhandledExceptionFilter(onUnhandledException);
    // Leave room on the stack for the handler itself when the crash is a stack overflow.
    ULONG guarantee = 64 * 1024;
    SetThreadStackGuarantee(&guarantee);
    std::signal(SIGABRT, onAbort);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
}

}  // namespace pc
