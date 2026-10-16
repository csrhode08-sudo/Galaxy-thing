// 16-bit wchar_t string functions. The game's text is UTF-16 (wchar_t is 16-bit on the Wii and on
// Windows; Linux builds use -fshort-wchar), so we provide our own implementations instead of the
// C library's, which may assume a 32-bit wchar_t.
#pragma once

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
#include <cwchar>
extern "C" {
#else
#include <wchar.h>
#endif

size_t pc_wcslen(const wchar_t* s);
wchar_t* pc_wcsncpy(wchar_t* dst, const wchar_t* src, size_t n);
wchar_t* pc_wcscpy(wchar_t* dst, const wchar_t* src);
int pc_wcscmp(const wchar_t* a, const wchar_t* b);
int pc_wcsncmp(const wchar_t* a, const wchar_t* b, size_t n);
wchar_t* pc_wcschr(const wchar_t* s, wchar_t c);
wchar_t* pc_wcscat(wchar_t* dst, const wchar_t* src);
int pc_vswprintf(wchar_t* buf, size_t n, const wchar_t* fmt, va_list ap);
int pc_swprintf(wchar_t* buf, size_t n, const wchar_t* fmt, ...);

#ifdef __cplusplus
}
#endif

#define wcslen pc_wcslen
#define wcsncpy pc_wcsncpy
#define wcscpy pc_wcscpy
#define wcscmp pc_wcscmp
#define wcsncmp pc_wcsncmp
#define wcschr pc_wcschr
#define wcscat pc_wcscat
#define vswprintf pc_vswprintf
#define swprintf pc_swprintf
