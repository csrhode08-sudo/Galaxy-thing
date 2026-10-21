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
#include <cstdint>
#include <atomic>
#include <chrono>
#include <thread>

#if defined(__linux__)
#include <signal.h>
#include <ucontext.h>
#include <execinfo.h>
#include <pthread.h>
#include <unistd.h>
#endif

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

// ---- Integer division by zero ------------------------------------------------------------------------------
// The Wii's divw/divwu never trap: dividing by zero (or INT_MIN by -1) leaves 0 in the result, or -1 for a negative
// dividend, and code built for it occasionally relies on that. x86 raises #DE instead, so the faulting DIV/IDIV is
// decoded and completed the way the Wii would: quotient as above, remainder = the dividend (what a - (a/b)*b gives).
namespace {

// Returns the length of the DIV/IDIV instruction at `ip` (0 if it isn't one) and its operand size in bytes.
int decodeDiv(const uint8_t* ip, int* opSize, bool* isSigned) {
    int len = 0;
    int size = 4;
    bool rexW = false;
    for (;;) {
        const uint8_t b = ip[len];
        if (b == 0x66) {
            size = 2;
            len++;
        } else if (b >= 0x40 && b <= 0x4F) {
            rexW = (b & 8) != 0;
            len++;
            break;  // REX is the last prefix
        } else if (b == 0xF2 || b == 0xF3 || b == 0x2E || b == 0x3E || b == 0x26 || b == 0x64 || b == 0x65 || b == 0x36) {
            len++;
        } else {
            break;
        }
    }
    const uint8_t op = ip[len++];
    if (op != 0xF7 && op != 0xF6) {
        return 0;
    }
    if (op == 0xF6) {
        size = 1;
    } else if (rexW) {
        size = 8;
    }
    const uint8_t modrm = ip[len++];
    const int mod = modrm >> 6, reg = (modrm >> 3) & 7, rm = modrm & 7;
    if (reg != 6 && reg != 7) {
        return 0;
    }
    if (mod != 3) {
        if (rm == 4) {
            const uint8_t sib = ip[len++];
            if (mod == 0 && (sib & 7) == 5) {
                len += 4;
            }
        } else if (mod == 0 && rm == 5) {
            len += 4;  // RIP-relative
        }
        if (mod == 1) {
            len += 1;
        } else if (mod == 2) {
            len += 4;
        }
    }
    *opSize = size;
    *isSigned = reg == 7;
    return len;
}

// Completes the division in the saved registers. Returns false if the instruction isn't understood.
bool emulateDivide(uint64_t& rip, uint64_t& rax, uint64_t& rdx) {
    int size = 0;
    bool isSigned = false;
    const int len = decodeDiv(reinterpret_cast<const uint8_t*>(rip), &size, &isSigned);
    if (len == 0) {
        return false;
    }
    switch (size) {
    case 8: {
        const bool neg = isSigned && static_cast<int64_t>(rdx) < 0;
        rdx = rax;
        rax = neg ? ~0ull : 0;
        break;
    }
    case 4: {
        const bool neg = isSigned && static_cast<int32_t>(rdx) < 0;
        const uint32_t lo = static_cast<uint32_t>(rax);
        rdx = lo;
        rax = neg ? 0xFFFFFFFFull : 0;
        break;
    }
    case 2: {
        const bool neg = isSigned && static_cast<int16_t>(rdx) < 0;
        const uint16_t lo = static_cast<uint16_t>(rax);
        rdx = (rdx & ~0xFFFFull) | lo;
        rax = (rax & ~0xFFFFull) | (neg ? 0xFFFFu : 0);
        break;
    }
    default: {  // 8-bit: AX / r/m8 -> AL quotient, AH remainder
        const bool neg = isSigned && static_cast<int16_t>(rax) < 0;
        const uint8_t lo = static_cast<uint8_t>(rax);
        rax = (rax & ~0xFFFFull) | (static_cast<uint64_t>(lo) << 8) | (neg ? 0xFFu : 0);
        break;
    }
    }
    rip += len;
    return true;
}

void reportDivide(uint64_t rip) {
    static int sReported = 0;
    if (sReported < 16) {
        sReported++;
#ifdef _WIN32
        log("[pc] integer division by zero at exe+0x%llx handled like the Wii\n",
            (unsigned long long)(rip - reinterpret_cast<uint64_t>(GetModuleHandleA(nullptr))));
#else
        log("[pc] integer division by zero at %p handled like the Wii\n", reinterpret_cast<void*>(rip));
#endif
    }
}

}  // namespace

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
        // Walk the crashing thread's own stack from the exception context.
        log("  crashing thread:\n");
        CONTEXT c = *ctx;
        DWORD64 probeBase = 0;
        if (c.Rip == 0 || RtlLookupFunctionEntry(c.Rip, &probeBase, nullptr) == nullptr) {
            // Called through a bad (e.g. null) pointer: the return address is on top of the stack.
            printAddr(reinterpret_cast<void*>(c.Rip));
            c.Rip = *reinterpret_cast<DWORD64*>(c.Rsp);
            c.Rsp += 8;
        }
        for (int depth = 0; depth < 48 && c.Rip != 0; depth++) {
            printAddr(reinterpret_cast<void*>(c.Rip));
            DWORD64 imageBase = 0;
            PRUNTIME_FUNCTION fn = RtlLookupFunctionEntry(c.Rip, &imageBase, nullptr);
            if (fn == nullptr) {
                // Leaf function: return address at [rsp].
                c.Rip = *reinterpret_cast<DWORD64*>(c.Rsp);
                c.Rsp += 8;
                continue;
            }
            void* handlerData = nullptr;
            DWORD64 establisher = 0;
            RtlVirtualUnwind(UNW_FLAG_NHANDLER, imageBase, c.Rip, fn, &c, &handlerData, &establisher, nullptr);
        }
        log("  handler:\n");
    }
    for (USHORT i = 0; i < n; i++) {
        printAddr(frames[i]);
    }
}

LONG WINAPI onVectoredException(EXCEPTION_POINTERS* info) {
    const DWORD code = info->ExceptionRecord->ExceptionCode;
    if (code == EXCEPTION_INT_DIVIDE_BY_ZERO || code == EXCEPTION_INT_OVERFLOW) {
        CONTEXT* c = info->ContextRecord;
        uint64_t rip = c->Rip, rax = c->Rax, rdx = c->Rdx;
        if (emulateDivide(rip, rax, rdx)) {
            reportDivide(c->Rip);
            c->Rip = rip;
            c->Rax = rax;
            c->Rdx = rdx;
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
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

#if defined(__linux__) && defined(__x86_64__)
namespace {
void onSigFpe(int sig, siginfo_t* si, void* uctx) {
    auto* uc = static_cast<ucontext_t*>(uctx);
    greg_t* g = uc->uc_mcontext.gregs;
    uint64_t rip = g[REG_RIP], rax = g[REG_RAX], rdx = g[REG_RDX];
    if ((si->si_code == FPE_INTDIV || si->si_code == FPE_INTOVF) && emulateDivide(rip, rax, rdx)) {
        reportDivide(g[REG_RIP]);
        g[REG_RIP] = rip;
        g[REG_RAX] = rax;
        g[REG_RDX] = rdx;
        return;
    }
    std::signal(sig, SIG_DFL);
    std::raise(sig);
}
}  // namespace
#endif

void installCrashHandler() {
#if defined(__linux__) && defined(__x86_64__)
    struct sigaction sa {};
    sa.sa_sigaction = onSigFpe;
    sa.sa_flags = SA_SIGINFO | SA_NODEFER;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGFPE, &sa, nullptr);
#endif
#ifdef _WIN32
    AddVectoredExceptionHandler(1, onVectoredException);
    SetUnhandledExceptionFilter(onUnhandledException);
    // Leave room on the stack for the handler itself when the crash is a stack overflow.
    ULONG guarantee = 64 * 1024;
    SetThreadStackGuarantee(&guarantee);
    std::signal(SIGABRT, onAbort);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
    if (std::getenv("PC_DIV_SELFTEST") != nullptr) {
        volatile int32_t a = 7, b = 0, n = -7;
        volatile int64_t a64 = 9, b64 = 0;
        volatile uint32_t ua = 5, ub = 0;
        log("[pc] divide test: 7/0=%d 7%%0=%d -7/0=%d -7%%0=%d 9/0=%lld 5u/0=%u (expect 0 7 -1 -7 0 0)\n", a / b, a % b, n / b,
            n % b, (long long)(a64 / b64), ua / ub);
    }
}

// ---- Freeze watchdog ----------------------------------------------------------------------------------------
// If the game stops finishing frames, log where the main thread is so a freeze can be traced like a crash.
namespace {
std::atomic<uint64_t> sFramesDone{0};
constexpr int kHangFrames = 48;

#ifdef _WIN32
HANDLE sMainThreadHandle = nullptr;

int captureMainStack(void** out) {
    if (sMainThreadHandle == nullptr || SuspendThread(sMainThreadHandle) == static_cast<DWORD>(-1)) {
        return 0;
    }
    int n = 0;
    CONTEXT c{};
    c.ContextFlags = CONTEXT_FULL;
    if (GetThreadContext(sMainThreadHandle, &c)) {
        // Only record addresses while the thread is suspended; logging could need a lock it holds.
        for (; n < kHangFrames && c.Rip != 0; n++) {
            out[n] = reinterpret_cast<void*>(c.Rip);
            DWORD64 imageBase = 0;
            PRUNTIME_FUNCTION fn = RtlLookupFunctionEntry(c.Rip, &imageBase, nullptr);
            if (fn == nullptr) {
                c.Rip = *reinterpret_cast<DWORD64*>(c.Rsp);
                c.Rsp += 8;
                continue;
            }
            void* handlerData = nullptr;
            DWORD64 establisher = 0;
            RtlVirtualUnwind(UNW_FLAG_NHANDLER, imageBase, c.Rip, fn, &c, &handlerData, &establisher, nullptr);
        }
    }
    ResumeThread(sMainThreadHandle);
    return n;
}

void logHangStack(void** frames, int n) {
    for (int i = 0; i < n; i++) {
        HMODULE mod = nullptr;
        char name[MAX_PATH] = "?";
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               static_cast<LPCSTR>(frames[i]), &mod) &&
            mod != nullptr) {
            GetModuleFileNameA(mod, name, sizeof(name));
            const char* base = std::strrchr(name, '\\');
            log("  %s+0x%llx\n", base ? base + 1 : name,
                (unsigned long long)(reinterpret_cast<uintptr_t>(frames[i]) - reinterpret_cast<uintptr_t>(mod)));
        } else {
            log("  %p\n", frames[i]);
        }
    }
}
#elif defined(__linux__)
pthread_t sMainThreadId;
void* sHangBuf[kHangFrames];
std::atomic<int> sHangCaptured{-1};

void onHangSignal(int) {
    sHangCaptured.store(backtrace(sHangBuf, kHangFrames));
}

int captureMainStack(void** out) {
    sHangCaptured.store(-1);
    pthread_kill(sMainThreadId, SIGUSR2);
    for (int i = 0; i < 400 && sHangCaptured.load() < 0; i++) {
        usleep(5000);
    }
    const int n = sHangCaptured.load();
    if (n <= 0) {
        return 0;
    }
    std::memcpy(out, sHangBuf, sizeof(void*) * n);
    return n;
}

void logHangStack(void** frames, int n) {
    char** names = backtrace_symbols(frames, n);
    for (int i = 0; i < n; i++) {
        log("  %s\n", names ? names[i] : "?");
    }
    std::free(names);
}
#else
int captureMainStack(void**) { return 0; }
void logHangStack(void**, int) {}
#endif

void watchdogMain() {
    uint64_t last = 0;
    int stalled = 0;
    for (;;) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        const uint64_t now = sFramesDone.load(std::memory_order_relaxed);
        if (now != last || now == 0) {
            if (stalled >= 8) {
                log("[hang] frames resumed after %d s\n", stalled);
            }
            last = now;
            stalled = 0;
            continue;
        }
        stalled++;
        if (stalled == 8 || stalled == 30) {
            void* frames[kHangFrames];
            const int n = captureMainStack(frames);
            log("[hang] no frame finished for %d s (frame %llu); main thread is at:\n", stalled, (unsigned long long)now);
            logHangStack(frames, n);
        }
    }
}
}  // namespace

void notifyFrameDone() {
    sFramesDone.fetch_add(1, std::memory_order_relaxed);
}

// Call on the game's main thread.
void startFreezeWatchdog() {
    if (std::getenv("GALAXY_NO_WATCHDOG") != nullptr) {
        return;
    }
#ifdef _WIN32
    DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &sMainThreadHandle,
                    THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, 0);
#elif defined(__linux__)
    sMainThreadId = pthread_self();
    void* prime[2];
    backtrace(prime, 2);  // load the unwinder now, not inside the signal handler
    struct sigaction sa {};
    sa.sa_handler = onHangSignal;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR2, &sa, nullptr);
#endif
    std::thread(watchdogMain).detach();
}

}  // namespace pc
