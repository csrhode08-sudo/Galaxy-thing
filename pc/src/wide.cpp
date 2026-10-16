// 16-bit wchar_t string functions (see pc/wide.h).
#include "pc/wide.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static_assert(sizeof(wchar_t) == 2, "Galaxy PC requires a 16-bit wchar_t (-fshort-wchar on non-Windows)");

extern "C" {

size_t pc_wcslen(const wchar_t* s) {
    size_t n = 0;
    while (s[n] != 0) {
        n++;
    }
    return n;
}

wchar_t* pc_wcsncpy(wchar_t* dst, const wchar_t* src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i] != 0; i++) {
        dst[i] = src[i];
    }
    for (; i < n; i++) {
        dst[i] = 0;
    }
    return dst;
}

wchar_t* pc_wcscpy(wchar_t* dst, const wchar_t* src) {
    wchar_t* d = dst;
    while ((*d++ = *src++) != 0) {
    }
    return dst;
}

int pc_wcscmp(const wchar_t* a, const wchar_t* b) {
    while (*a != 0 && *a == *b) {
        a++;
        b++;
    }
    return (int)(uint16_t)*a - (int)(uint16_t)*b;
}

int pc_wcsncmp(const wchar_t* a, const wchar_t* b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i] || a[i] == 0) {
            return (int)(uint16_t)a[i] - (int)(uint16_t)b[i];
        }
    }
    return 0;
}

wchar_t* pc_wcschr(const wchar_t* s, wchar_t c) {
    for (;; s++) {
        if (*s == c) {
            return const_cast<wchar_t*>(s);
        }
        if (*s == 0) {
            return nullptr;
        }
    }
}

wchar_t* pc_wcscat(wchar_t* dst, const wchar_t* src) {
    pc_wcscpy(dst + pc_wcslen(dst), src);
    return dst;
}

// Supports the conversions the game uses: flags/width/precision, h/l/ll modifiers,
// d i u x X o c f F e E g G p s and %ls (wide string), %%.
int pc_vswprintf(wchar_t* buf, size_t n, const wchar_t* fmt, va_list ap) {
    size_t out = 0;
    auto put = [&](wchar_t c) {
        if (out + 1 < n) {
            buf[out] = c;
        }
        out++;
    };

    while (*fmt != 0) {
        if (*fmt != L'%') {
            put(*fmt++);
            continue;
        }
        const wchar_t* start = fmt++;
        if (*fmt == L'%') {
            put(L'%');
            fmt++;
            continue;
        }

        char spec[32];
        size_t sl = 0;
        spec[sl++] = '%';
        while (*fmt != 0 && std::strchr("-+ #0123456789.*", (char)*fmt) != nullptr && sl < sizeof(spec) - 4) {
            if (*fmt == L'*') {
                int v = va_arg(ap, int);
                sl += std::snprintf(spec + sl, sizeof(spec) - sl, "%d", v);
            } else {
                spec[sl++] = (char)*fmt;
            }
            fmt++;
        }
        int lcount = 0, hcount = 0;
        while (*fmt == L'l' || *fmt == L'h') {
            (*fmt == L'l' ? lcount : hcount)++;
            fmt++;
        }
        (void)hcount;
        wchar_t conv = *fmt;
        if (conv == 0) {
            break;
        }
        fmt++;

        char tmp[512];
        int len = 0;
        switch (conv) {
        case L'd':
        case L'i':
        case L'u':
        case L'x':
        case L'X':
        case L'o': {
            if (lcount >= 2) {
                spec[sl++] = 'l';
                spec[sl++] = 'l';
                spec[sl++] = (char)conv;
                spec[sl] = 0;
                len = std::snprintf(tmp, sizeof(tmp), spec, va_arg(ap, long long));
            } else {
                // On the Wii, int and long are both 32-bit.
                spec[sl++] = (char)conv;
                spec[sl] = 0;
                len = std::snprintf(tmp, sizeof(tmp), spec, va_arg(ap, int));
            }
            break;
        }
        case L'c': {
            wchar_t c = (wchar_t)va_arg(ap, int);
            put(c);
            continue;
        }
        case L'f':
        case L'F':
        case L'e':
        case L'E':
        case L'g':
        case L'G': {
            spec[sl++] = (char)conv;
            spec[sl] = 0;
            len = std::snprintf(tmp, sizeof(tmp), spec, va_arg(ap, double));
            break;
        }
        case L'p': {
            spec[sl++] = 'p';
            spec[sl] = 0;
            len = std::snprintf(tmp, sizeof(tmp), spec, va_arg(ap, void*));
            break;
        }
        case L's':
        case L'S': {
            int width = 0, prec = -1;
            bool left = false;
            for (size_t i = 1; i < sl; i++) {
                if (spec[i] == '-') left = true;
                else if (spec[i] == '.') {
                    prec = std::atoi(spec + i + 1);
                    break;
                } else if (spec[i] >= '1' && spec[i] <= '9' && width == 0) {
                    width = std::atoi(spec + i);
                    while (i + 1 < sl && spec[i + 1] >= '0' && spec[i + 1] <= '9') i++;
                }
            }
            if (lcount > 0 || conv == L'S') {
                const wchar_t* s = va_arg(ap, const wchar_t*);
                if (s == nullptr) s = L"(null)";
                int sn = (int)pc_wcslen(s);
                if (prec >= 0 && sn > prec) sn = prec;
                if (!left) for (int i = sn; i < width; i++) put(L' ');
                for (int i = 0; i < sn; i++) put(s[i]);
                if (left) for (int i = sn; i < width; i++) put(L' ');
            } else {
                const char* s = va_arg(ap, const char*);
                if (s == nullptr) s = "(null)";
                int sn = (int)std::strlen(s);
                if (prec >= 0 && sn > prec) sn = prec;
                if (!left) for (int i = sn; i < width; i++) put(L' ');
                for (int i = 0; i < sn; i++) put((wchar_t)(unsigned char)s[i]);
                if (left) for (int i = sn; i < width; i++) put(L' ');
            }
            continue;
        }
        default:
            for (const wchar_t* p = start; p < fmt; p++) put(*p);
            continue;
        }
        if (len < 0) len = 0;
        if (len >= (int)sizeof(tmp)) len = sizeof(tmp) - 1;
        for (int i = 0; i < len; i++) {
            put((wchar_t)(unsigned char)tmp[i]);
        }
    }
    if (n > 0) {
        buf[out < n ? out : n - 1] = 0;
    }
    return out < n ? (int)out : -1;
}

int pc_swprintf(wchar_t* buf, size_t n, const wchar_t* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = pc_vswprintf(buf, n, fmt, ap);
    va_end(ap);
    return r;
}

}  // extern "C"
