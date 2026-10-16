// Data tables the game links into main.dol (galaxy info, story progress, the error message archive).
// They are copied out of the disc's main.dol at startup instead of being compiled into the executable.
#include "pc/platform.h"

#include <revolution/types.h>

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

extern u8 cGalaxyInfoTable[];
extern u8 cStoryProgressTable[];
extern u8 cErrorMessageArchive[];

namespace {
    struct Asset {
        const char* name;
        u8* dst;
        u32 address;  // RMGK01 virtual address
        u32 size;
    };

    const Asset kAssets[] = {
        {"cStoryProgressTable", cStoryProgressTable, 0x8053DC20, 0x1E0},
        {"cGalaxyInfoTable", cGalaxyInfoTable, 0x8053DE00, 0xD20},
        {"cErrorMessageArchive", cErrorMessageArchive, 0x8053F5E0, 0x1CAC0},
    };

    u32 be32(const u8* p) {
        return (u32(p[0]) << 24) | (u32(p[1]) << 16) | (u32(p[2]) << 8) | u32(p[3]);
    }
}  // namespace

namespace pc {
    bool loadDolAssets(const std::string& discPath) {
        std::vector<u8> fileData;
        const u8* dol = nullptr;
        s32 size = 0;
        dol = DVDGetDOLLocation(&size);
        if (dol == nullptr || size <= 0x100) {
            // Extracted disc: <root>/files is the disc root, the DOL lives in <root>/sys/main.dol.
            namespace fs = std::filesystem;
            for (const fs::path& p : {fs::path(discPath) / ".." / "sys" / "main.dol", fs::path(discPath) / "sys" / "main.dol"}) {
                std::ifstream in(p, std::ios::binary);
                if (in) {
                    fileData.assign(std::istreambuf_iterator<char>(in), {});
                    break;
                }
            }
            dol = fileData.data();
            size = (s32)fileData.size();
        }
        if (dol == nullptr || size <= 0x100) {
            std::fprintf(stderr, "[assets] main.dol not found\n");
            return false;
        }

        bool ok = true;
        for (const Asset& a : kAssets) {
            bool found = false;
            for (int i = 0; i < 18; i++) {
                const u32 off = be32(dol + i * 4);
                const u32 addr = be32(dol + 0x48 + i * 4);
                const u32 len = be32(dol + 0x90 + i * 4);
                if (len != 0 && a.address >= addr && a.address + a.size <= addr + len && off + (a.address - addr) + a.size <= (u32)size) {
                    std::memcpy(a.dst, dol + off + (a.address - addr), a.size);
                    found = true;
                    break;
                }
            }
            if (!found) {
                std::fprintf(stderr, "[assets] %s not found in main.dol (is this the Korean RMGK01 disc?)\n", a.name);
                ok = false;
            }
        }
        return ok;
    }
}  // namespace pc
