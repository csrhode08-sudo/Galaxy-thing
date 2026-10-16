// Converts a Super Mario Galaxy message file (MESGbmg1: INF1/DAT1/FLW1/FLI1) to host byte order, in place.
//
// Text is UTF-16. Every 16-bit unit is swapped, including the parameters of inline tags
// (0x1A, size:u8, group:u8, tag:u16, params...), except the (size, group) byte pair, which is kept
// in file order so MessageEditorMessageTag can keep reading it byte-wise. See
// MessageEditorMessageTag::getParam8/getParam32 for how 8- and 32-bit parameters are read back.
#include "pc/byteswap.h"

#include <cstdint>
#include <mutex>

namespace {

std::mutex sLock;

using pc::readU16;
using pc::readU32;
using pc::swap16;
using pc::swap32;

void swapText(uint8_t* p, uint8_t* end) {
    while (p + 2 <= end) {
        uint16_t c = pc::readBE16(p);
        swap16(p);
        p += 2;
        if (c == 0x1A && p + 2 <= end) {
            uint8_t size = p[0];  // whole tag in bytes, including the 0x1A
            p += 2;               // (size, group) stays as is
            uint8_t* tagEnd = p - 4 + size;
            if (size < 4 || tagEnd > end) {
                continue;
            }
            for (; p + 2 <= tagEnd; p += 2) {
                swap16(p);
            }
            p = tagEnd;
        }
    }
}

}  // namespace

namespace pc::bmg {

void swapMessageFile(void* buf) {
    if (buf == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lk(sLock);
    uint8_t* f = static_cast<uint8_t*>(buf);
    // The block count's top byte is zero in a big-endian file and (for any sane count) nonzero once swapped.
    if (f[0xC] != 0 || f[0xF] == 0) {
        return;
    }
    swap32(f + 8, 2);  // fileSize, numBlocks
    uint32_t numBlocks = readU32(f + 0xC);

    uint8_t* b = f + 0x20;
    for (uint32_t i = 0; i < numBlocks; i++) {
        swap32(b, 2);  // magic, size
        uint32_t magic = readU32(b);
        uint32_t size = readU32(b + 4);
        if (magic == 'INF1') {
            swap16(b + 8, 2);  // count, itemSize
            swap32(b + 12);
            uint16_t count = readU16(b + 8), itemSize = readU16(b + 10);
            for (uint32_t k = 0; k < count; k++) {
                uint8_t* e = b + 16 + k * itemSize;
                swap32(e);      // text offset
                swap16(e + 4);  // camera set id
            }
        } else if (magic == 'DAT1') {
            swapText(b + 8, b + size);
        } else if (magic == 'FLW1') {
            swap16(b + 8, 2);  // nodeCount, branchCount
            swap32(b + 12);
            uint16_t nodeCount = readU16(b + 8), branchCount = readU16(b + 10);
            uint8_t* n = b + 16;
            for (uint32_t k = 0; k < nodeCount; k++, n += 8) {
                if (n[0] == 3) {  // event: u8 type, u8 event, u16 branch, u32 arg
                    swap16(n + 2);
                    swap32(n + 4);
                } else {  // message / branch: u8 type, u8 group, u16 x3
                    swap16(n + 2, 3);
                }
            }
            swap16(n, branchCount);
        } else if (magic == 'FLI1') {
            swap16(b + 8, 2);
            uint16_t count = readU16(b + 8), itemSize = readU16(b + 10);
            for (uint32_t k = 0; k < count && itemSize >= 6; k++) {
                uint8_t* e = b + 16 + k * itemSize;
                swap32(e);
                swap16(e + 4);
            }
        }
        b += size;
    }
}

}  // namespace pc::bmg

#include "pc/msgarg.h"
#include <cstring>

namespace pc::msgarg {

namespace {
constexpr uint32_t kSlots = 4096;
const wchar_t* sSlots[kSlots];
uint32_t sNext = 0;
std::mutex sArgLock;
}  // namespace

uint32_t store(const wchar_t* str) {
    if (str == nullptr) {
        return 0;
    }
    std::lock_guard<std::mutex> lk(sArgLock);
    for (uint32_t i = 0; i < kSlots; i++) {  // reuse a slot already holding this string
        if (sSlots[i] == str) {
            return (i << 8) | 1;
        }
    }
    uint32_t i = sNext++ % kSlots;
    sSlots[i] = str;
    return (i << 8) | 1;
}

const wchar_t* load(const void* param) {
    uint32_t h;
    std::memcpy(&h, param, sizeof(h));
    if ((h & 0xFF) != 1) {
        return nullptr;
    }
    uint32_t i = h >> 8;
    return i < kSlots ? sSlots[i] : nullptr;
}

}  // namespace pc::msgarg
