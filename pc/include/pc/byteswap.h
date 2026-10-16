// In-place byte-order conversion helpers for big-endian resource files.
#pragma once
#include <cstdint>
#include <cstring>

namespace pc {

inline uint16_t bswap16(uint16_t v) { return __builtin_bswap16(v); }
inline uint32_t bswap32(uint32_t v) { return __builtin_bswap32(v); }

// Swap `count` 16/32-bit values starting at p (no alignment requirement).
inline void swap16(void* p, size_t count = 1) {
    uint8_t* b = static_cast<uint8_t*>(p);
    for (size_t i = 0; i < count; i++, b += 2) {
        uint8_t t = b[0];
        b[0] = b[1];
        b[1] = t;
    }
}
inline void swap32(void* p, size_t count = 1) {
    uint8_t* b = static_cast<uint8_t*>(p);
    for (size_t i = 0; i < count; i++, b += 4) {
        uint8_t t0 = b[0], t1 = b[1];
        b[0] = b[3];
        b[1] = b[2];
        b[2] = t1;
        b[3] = t0;
    }
}
// Read a big-endian value (from data not yet converted).
inline uint32_t readBE32(const void* p) {
    const uint8_t* b = static_cast<const uint8_t*>(p);
    return (uint32_t(b[0]) << 24) | (uint32_t(b[1]) << 16) | (uint32_t(b[2]) << 8) | uint32_t(b[3]);
}
inline uint16_t readBE16(const void* p) {
    const uint8_t* b = static_cast<const uint8_t*>(p);
    return uint16_t((b[0] << 8) | b[1]);
}
inline uint32_t readU32(const void* p) {
    uint32_t v;
    std::memcpy(&v, p, 4);
    return v;
}
inline uint16_t readU16(const void* p) {
    uint16_t v;
    std::memcpy(&v, p, 2);
    return v;
}

}  // namespace pc
