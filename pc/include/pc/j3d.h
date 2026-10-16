// Byte-order conversion state for J3D (and other JSystem) resource files that are partly converted in place.
#pragma once
#include <cstdint>

namespace pc::j3d {

// True while a loader is processing a file that has not been converted yet. Loaders use it to decide
// whether to byte-swap arrays in place.
extern thread_local bool gConvert;

// Marks a J3D/JSystem file (which starts with a 0x20-byte header whose last 16 bytes are padding) as converted.
// Returns true the first time it is called for that file data.
bool beginConvert(const void* file);

// RAII helper for loader entry points.
struct ConvertScope {
    bool prev;
    explicit ConvertScope(const void* file) : prev(gConvert) { gConvert = beginConvert(file); }
    ~ConvertScope() { gConvert = prev; }
};

}  // namespace pc::j3d
