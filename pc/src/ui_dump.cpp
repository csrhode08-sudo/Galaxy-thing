// Saves every menu/HUD image (the TPL textures inside the layout archives) as PNG files named for texture
// replacement, so they can be edited and dropped into the textures folder. Run from the F1 > Graphics page.
#include "pc/ui_dump.h"
#include "pc/platform.h"

#include <revolution/dvd.h>
#include <revolution/gx.h>

#include <aurora/texture.hpp>

#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace pc {
namespace {

u32 be32(const u8* p) {
    return (u32(p[0]) << 24) | (u32(p[1]) << 16) | (u32(p[2]) << 8) | p[3];
}
u16 be16(const u8* p) {
    return u16((p[0] << 8) | p[1]);
}

bool readDiscFile(const std::string& path, std::vector<u8>& out) {
    DVDFileInfo fi;
    if (!DVDOpen(path.c_str(), &fi)) {
        return false;
    }
    const u32 len = fi.length;
    const u32 rounded = (len + 31) & ~31u;
    std::vector<u8> raw(rounded + 64);
    u8* buf = reinterpret_cast<u8*>((reinterpret_cast<uintptr_t>(raw.data()) + 31) & ~uintptr_t(31));
    const s32 n = DVDReadPrio(&fi, buf, rounded, 0, 2);
    DVDClose(&fi);
    if (n < 0) {
        return false;
    }
    out.assign(buf, buf + len);
    return true;
}

bool yaz0Decode(const std::vector<u8>& in, std::vector<u8>& out) {
    if (in.size() < 16 || std::memcmp(in.data(), "Yaz0", 4) != 0) {
        return false;
    }
    const u32 size = be32(in.data() + 4);
    out.assign(size, 0);
    size_t src = 16, dst = 0;
    u8 header = 0;
    int bits = 0;
    while (dst < size && src < in.size()) {
        if (bits == 0) {
            header = in[src++];
            bits = 8;
        }
        if (header & 0x80) {
            out[dst++] = in[src++];
        } else {
            if (src + 1 >= in.size()) break;
            const u8 b1 = in[src++], b2 = in[src++];
            size_t dist = (((b1 & 0x0F) << 8) | b2) + 1;
            size_t count = b1 >> 4;
            if (count == 0) {
                if (src >= in.size()) break;
                count = in[src++] + 0x12;
            } else {
                count += 2;
            }
            if (dist > dst) return false;
            for (size_t i = 0; i < count && dst < size; i++, dst++) {
                out[dst] = out[dst - dist];
            }
        }
        header <<= 1;
        bits--;
    }
    return dst == size;
}

struct DumpStats {
    int archives = 0;
    int images = 0;
    int failed = 0;
};

void dumpTpl(const u8* tpl, size_t size, const std::filesystem::path& dir, DumpStats& stats) {
    if (size < 12 || be32(tpl) != 0x0020AF30) {
        return;
    }
    const u32 count = be32(tpl + 4), table = be32(tpl + 8);
    if (count > 256 || table + count * 8 > size) {
        return;
    }
    for (u32 i = 0; i < count; i++) {
        const u32 imgOfs = be32(tpl + table + i * 8), palOfs = be32(tpl + table + i * 8 + 4);
        if (imgOfs + 0x24 > size) continue;
        const u8* ih = tpl + imgOfs;
        const u16 height = be16(ih), width = be16(ih + 2);
        const u32 format = be32(ih + 4), dataOfs = be32(ih + 8);
        if (width == 0 || height == 0 || dataOfs >= size) continue;
        GXTexObj obj;
        GXTlutObj tlut;
        const bool ci = format == GX_TF_C4 || format == GX_TF_C8 || format == GX_TF_C14X2;
        if (ci) {
            if (palOfs == 0 || palOfs + 12 > size) continue;
            const u8* ph = tpl + palOfs;
            const u16 entries = be16(ph);
            const u32 palFmt = be32(ph + 4), palData = be32(ph + 8);
            if (palData >= size) continue;
            GXInitTlutObj(&tlut, const_cast<u8*>(tpl + palData), static_cast<GXTlutFmt>(palFmt), entries);
            GXInitTexObjCI(&obj, const_cast<u8*>(tpl + dataOfs), width, height, static_cast<GXCITexFmt>(format), GX_CLAMP,
                           GX_CLAMP, GX_FALSE, GX_TLUT0);
        } else {
            GXInitTexObj(&obj, const_cast<u8*>(tpl + dataOfs), width, height, static_cast<GXTexFmt>(format), GX_CLAMP,
                         GX_CLAMP, GX_FALSE);
        }
        if (aurora::texture::dump_texture_png(&obj, ci ? &tlut : nullptr, dir).empty()) {
            stats.failed++;
        } else {
            stats.images++;
        }
    }
}

// Walks a RARC archive and dumps each .tpl file into <dir>/<tpl name>/.
void dumpArchive(const std::vector<u8>& arc, const std::filesystem::path& dir, DumpStats& stats) {
    if (arc.size() < 0x40 || std::memcmp(arc.data(), "RARC", 4) != 0) {
        return;
    }
    const u8* d = arc.data();
    const u32 dataStart = 0x20 + be32(d + 0x0C);
    const u8* info = d + 0x20;
    const u32 numEntries = be32(info + 0x08), entriesOfs = 0x20 + be32(info + 0x0C);
    const u32 stringsOfs = 0x20 + be32(info + 0x14);
    if (entriesOfs + numEntries * 0x14 > arc.size() || stringsOfs >= arc.size()) {
        return;
    }
    stats.archives++;
    for (u32 i = 0; i < numEntries; i++) {
        const u8* e = d + entriesOfs + i * 0x14;
        const u32 typeName = be32(e + 4);
        const u8 flags = typeName >> 24;
        if ((flags & 0x01) == 0) continue;  // not a file
        const u32 nameOfs = stringsOfs + (typeName & 0xFFFFFF);
        if (nameOfs >= arc.size()) continue;
        const std::string name(reinterpret_cast<const char*>(d + nameOfs));
        if (name.size() < 4 || name.compare(name.size() - 4, 4, ".tpl") != 0) continue;
        const u32 ofs = dataStart + be32(e + 8), size = be32(e + 12);
        if (ofs + size > arc.size()) continue;
        std::vector<u8> tpl;
        const u8* p = d + ofs;
        size_t n = size;
        if (size >= 4 && std::memcmp(p, "Yaz0", 4) == 0) {
            if (!yaz0Decode(std::vector<u8>(p, p + size), tpl)) continue;
            p = tpl.data();
            n = tpl.size();
        }
        dumpTpl(p, n, dir / name.substr(0, name.size() - 4), stats);
    }
}

void dumpDirectory(const std::string& discDir, const std::filesystem::path& outDir, DumpStats& stats) {
    DVDDir dir;
    if (!DVDOpenDir(discDir.c_str(), &dir)) {
        return;
    }
    DVDDirEntry entry;
    std::vector<std::pair<std::string, bool>> children;
    while (DVDReadDir(&dir, &entry)) {
        children.emplace_back(entry.name, entry.isDir != 0);
    }
    DVDCloseDir(&dir);
    for (const auto& [name, isDir] : children) {
        const std::string path = discDir + "/" + name;
        if (isDir) {
            dumpDirectory(path, outDir / name, stats);
            continue;
        }
        if (name.size() < 4 || name.compare(name.size() - 4, 4, ".arc") != 0) continue;
        std::vector<u8> raw, arc;
        if (!readDiscFile(path, raw)) continue;
        const std::vector<u8>* data = &raw;
        if (yaz0Decode(raw, arc)) {
            data = &arc;
        }
        dumpArchive(*data, outDir / name.substr(0, name.size() - 4), stats);
    }
}

}  // namespace

std::string dumpUiTextures(const std::string& outRoot) {
    DumpStats stats;
    const std::filesystem::path root = std::filesystem::u8path(outRoot);
    dumpDirectory("/LayoutData", root / "LayoutData", stats);
    const bool english = DVDConvertPathToEntrynum("/UsEnglish/LayoutData") >= 0;
    const std::string lang = english ? "/UsEnglish/LayoutData" : "/KrKorean/LayoutData";
    dumpDirectory(lang, root / (english ? "UsEnglish" : "KrKorean"), stats);
    char msg[256];
    std::snprintf(msg, sizeof(msg), "Saved %d images from %d archives%s.", stats.images, stats.archives,
                  stats.failed ? " (some could not be converted)" : "");
    pc_log("[ui_dump] %s -> %s\n", msg, outRoot.c_str());
    return msg;
}

}  // namespace pc
