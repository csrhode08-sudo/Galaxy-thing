// Converts NW4R layout (.brlyt) and layout animation (.brlan) files from big-endian to host order, in place.
// The file's byte-order mark is converted too, so a file is only ever converted once.
#include "pc/byteswap.h"

#include <cstdint>
#include <mutex>

namespace {

std::mutex sLock;

using pc::readU16;
using pc::readU32;
using pc::swap16;
using pc::swap32;

bool fourcc(const uint8_t* p, const char* s) {
    return p[0] == (uint8_t)s[0] && p[1] == (uint8_t)s[1] && p[2] == (uint8_t)s[2] && p[3] == (uint8_t)s[3];
}

// Returns the block count, or -1 if the file needs no conversion.
int swapFileHeader(uint8_t* f) {
    if (f[4] != 0xFE || f[5] != 0xFF) {
        return -1;  // already converted (or not a valid file; the loader rejects it)
    }
    swap16(f + 4);   // byteOrder
    swap16(f + 6);   // version
    swap32(f + 8);   // fileSize
    swap16(f + 12);  // headerSize
    swap16(f + 14);  // dataBlocks
    return readU16(f + 14);
}

void swapTexCoords(uint8_t* p, int num) {
    swap32(p, (size_t)num * 8);  // num * 4 corners * VEC2
}

void swapPane(uint8_t* b) {
    swap32(b + 0x24, 10);  // translate(3) rotate(3) scale(2) size(2)
}

void swapMaterial(uint8_t* m) {
    swap16(m + 20, 12);  // tevCols: 3 x GXColorS10
    swap32(m + 60);      // resNum bits
    uint32_t bits = readU32(m + 60);
    auto get = [bits](int pos, int len) { return (bits >> pos) & ~(0xFFFFFFFFu << len); };
    uint32_t texMapNum = get(0, 4), texSRTNum = get(4, 4), texCoordGenNum = get(8, 4);
    bool tevSwap = get(12, 1), alphaCmp = get(23, 1), blend = get(24, 1);
    uint32_t indSRTNum = get(13, 2), indStageNum = get(15, 3), tevStageNum = get(18, 5);
    uint32_t chanCtrlNum = get(25, 1), matColNum = get(27, 1);
    (void)alphaCmp;
    (void)blend;
    (void)tevStageNum;

    uint8_t* p = m + 64;
    for (uint32_t i = 0; i < texMapNum; i++, p += 4) {
        swap16(p);  // texIdx
    }
    swap32(p, texSRTNum * 5);
    p += texSRTNum * 20;
    p += texCoordGenNum * 4;
    p += chanCtrlNum * 4;
    p += matColNum * 4;
    if (tevSwap) {
        p += 16;
    }
    swap32(p, indSRTNum * 5);
    p += indSRTNum * 20;
    (void)indStageNum;  // u8-only structures follow
}

void swapLayoutBlock(uint8_t* b) {
    if (fourcc(b, "lyt1")) {
        swap32(b + 12, 2);
    } else if (fourcc(b, "txl1") || fourcc(b, "fnl1")) {
        swap16(b + 8);
        int n = readU16(b + 8);
        for (int i = 0; i < n; i++) {
            swap32(b + 12 + i * 8);
        }
    } else if (fourcc(b, "mat1")) {
        swap16(b + 8);
        int n = readU16(b + 8);
        swap32(b + 12, n);
        for (int i = 0; i < n; i++) {
            swapMaterial(b + readU32(b + 12 + i * 4));
        }
    } else if (fourcc(b, "pan1") || fourcc(b, "bnd1")) {
        swapPane(b);
    } else if (fourcc(b, "pic1")) {
        swapPane(b);
        swap32(b + 0x4C, 4);  // vtxCols
        swap16(b + 0x5C);     // materialIdx
        swapTexCoords(b + 0x60, b[0x5E]);
    } else if (fourcc(b, "txt1")) {
        swapPane(b);
        swap16(b + 0x4C, 4);   // textBufBytes textStrBytes materialIdx fontIdx
        swap32(b + 0x58, 7);  // textStrOffset textCols[2] fontSize charSpace lineSpace
        uint32_t strOffs = readU32(b + 0x58);
        uint16_t strBytes = readU16(b + 0x4E);
        if (strOffs != 0) {
            swap16(b + strOffs, strBytes / 2);
        }
    } else if (fourcc(b, "wnd1")) {
        swapPane(b);
        swap32(b + 0x4C, 4);  // inflation
        swap32(b + 0x60, 2);  // contentOffset, frameOffsetTableOffset
        uint8_t frameNum = b[0x5C];
        uint8_t* content = b + readU32(b + 0x60);
        swap32(content, 4);  // vtxCols
        swap16(content + 16);
        swapTexCoords(content + 20, content[18]);
        uint8_t* frames = b + readU32(b + 0x64);
        swap32(frames, frameNum);
        for (int i = 0; i < frameNum; i++) {
            swap16(b + readU32(frames + i * 4));
        }
    } else if (fourcc(b, "grp1")) {
        swap16(b + 24);
    } else if (fourcc(b, "usd1")) {
        swap16(b + 8);
        int n = readU16(b + 8);
        for (int i = 0; i < n; i++) {
            uint8_t* e = b + 12 + i * 12;
            swap32(e, 2);
            swap16(e + 8);
            uint8_t type = e[10];
            if (type == 1 || type == 2) {
                swap32(e + readU32(e + 4), readU16(e + 8));
            }
        }
    }
}

void swapAnimBlock(uint8_t* b) {
    if (fourcc(b, "pat1")) {
        swap16(b + 8, 2);
        swap32(b + 12, 2);
        swap16(b + 20, 2);
    } else if (fourcc(b, "pah1")) {
        swap32(b + 8);
        swap16(b + 12);
    } else if (fourcc(b, "pai1")) {
        swap16(b + 8);  // frameSize
        swap16(b + 12, 2);
        swap32(b + 16);
        int fileNum = readU16(b + 12);
        int contNum = readU16(b + 14);
        swap32(b + 20, fileNum);
        uint8_t* contOffs = b + readU32(b + 16);
        swap32(contOffs, contNum);
        for (int c = 0; c < contNum; c++) {
            uint8_t* cont = b + readU32(contOffs + c * 4);
            int infoNum = cont[20];
            swap32(cont + 24, infoNum);
            for (int i = 0; i < infoNum; i++) {
                uint8_t* info = cont + readU32(cont + 24 + i * 4);
                swap32(info);  // kind
                bool step = fourcc(info, "IVLR") || fourcc(info, "PTLR");  // RLVI / RLTP, now little-endian
                int targetNum = info[4];
                swap32(info + 8, targetNum);
                for (int t = 0; t < targetNum; t++) {
                    uint8_t* target = info + readU32(info + 8 + t * 4);
                    swap16(target + 4);  // keyNum
                    swap32(target + 8);  // keysOffset
                    uint16_t keyNum = readU16(target + 4);
                    uint8_t* keys = target + readU32(target + 8);
                    if (step) {
                        for (int k = 0; k < keyNum; k++) {
                            swap32(keys + k * 8);
                            swap16(keys + k * 8 + 4);
                        }
                    } else {
                        swap32(keys, (size_t)keyNum * 3);
                    }
                }
            }
        }
    }
}

void swapFile(void* buf, void (*blockFn)(uint8_t*)) {
    if (buf == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lk(sLock);
    uint8_t* f = static_cast<uint8_t*>(buf);
    int blocks = swapFileHeader(f);
    if (blocks < 0) {
        return;
    }
    uint8_t* b = f + readU16(f + 12);
    for (int i = 0; i < blocks; i++) {
        swap32(b + 4);  // block size
        blockFn(b);
        b += readU32(b + 4);
    }
}

}  // namespace

namespace pc::lyt {

void swapLayout(const void* buf) {
    swapFile(const_cast<void*>(buf), swapLayoutBlock);
}

void swapAnimation(const void* buf) {
    swapFile(const_cast<void*>(buf), swapAnimBlock);
}

}  // namespace pc::lyt
