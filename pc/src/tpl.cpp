// TPL texture palettes: parse the big-endian file into native descriptors (cached per file).
#include "pc/platform.h"
#include "pc/endian.h"

#include <revolution/tpl.h>

#include <cstring>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace {

struct FilePalette {
    BE(u32) versionNumber;
    BE(u32) numDescriptors;
    BE(u32) descriptorArray;
};
struct FileDescriptor {
    BE(u32) textureHeader;
    BE(u32) CLUTHeader;
};
struct FileHeader {
    BE(u16) height;
    BE(u16) width;
    BE(u32) format;
    BE(u32) data;
    BE(u32) wrapS;
    BE(u32) wrapT;
    BE(u32) minFilter;
    BE(u32) magFilter;
    BE(f32) LODBias;
    u8 edgeLODEnable;
    u8 minLOD;
    u8 maxLOD;
    u8 unpacked;
};
struct FileClutHeader {
    BE(u16) numEntries;
    u8 unpacked;
    u8 _4;
    BE(u32) format;
    BE(u32) data;
};

struct HostPalette {
    u8 signature[64];  // file prefix, to detect a different TPL later loaded at the same address
    std::vector<TPLDescriptor> descriptors;
    std::vector<TPLHeader> headers;
    std::vector<TPLClutHeader> cluts;
};

std::mutex sMutex;
std::unordered_map<const void*, HostPalette*> sPalettes;

HostPalette* getHost(TPLPalettePtr pal) {
    std::lock_guard lock(sMutex);
    auto it = sPalettes.find(pal);
    if (it != sPalettes.end()) {
        if (memcmp(it->second->signature, pal, sizeof(it->second->signature)) == 0) {
            return it->second;
        }
        delete it->second;
        sPalettes.erase(it);
    }
    u8* base = reinterpret_cast<u8*>(pal);
    const FilePalette* fp = reinterpret_cast<const FilePalette*>(base);
    u32 n = fp->numDescriptors;
    auto* host = new HostPalette;
    memcpy(host->signature, pal, sizeof(host->signature));
    host->descriptors.resize(n);
    host->headers.resize(n);
    host->cluts.resize(n);
    const FileDescriptor* fd = reinterpret_cast<const FileDescriptor*>(base + (u32)fp->descriptorArray);
    for (u32 i = 0; i < n; i++) {
        TPLDescriptor& d = host->descriptors[i];
        d.textureHeader = nullptr;
        d.CLUTHeader = nullptr;
        if ((u32)fd[i].textureHeader != 0) {
            const FileHeader* fh = reinterpret_cast<const FileHeader*>(base + (u32)fd[i].textureHeader);
            TPLHeader& h = host->headers[i];
            h.height = fh->height;
            h.width = fh->width;
            h.format = fh->format;
            h.data = reinterpret_cast<char*>(base + (u32)fh->data);
            h.wrapS = (GXTexWrapMode)(u32)fh->wrapS;
            h.wrapT = (GXTexWrapMode)(u32)fh->wrapT;
            h.minFilter = (GXTexFilter)(u32)fh->minFilter;
            h.magFilter = (GXTexFilter)(u32)fh->magFilter;
            h.LODBias = fh->LODBias;
            h.edgeLODEnable = fh->edgeLODEnable;
            h.minLOD = fh->minLOD;
            h.maxLOD = fh->maxLOD;
            h.unpacked = fh->unpacked;
            d.textureHeader = &h;
        }
        if ((u32)fd[i].CLUTHeader != 0) {
            const FileClutHeader* fc = reinterpret_cast<const FileClutHeader*>(base + (u32)fd[i].CLUTHeader);
            TPLClutHeader& c = host->cluts[i];
            c.numEntries = fc->numEntries;
            c.unpacked = fc->unpacked;
            c._4 = fc->_4;
            c.format = (GXTlutFmt)(u32)fc->format;
            c.data = reinterpret_cast<char*>(base + (u32)fc->data);
            d.CLUTHeader = &c;
        }
    }
    sPalettes.emplace(pal, host);
    return host;
}

}  // namespace

extern "C" {

void TPLBind(TPLPalettePtr pal) {
    if (pal != nullptr) {
        getHost(pal);
    }
}

TPLDescriptorPtr TPLGet(TPLPalettePtr pal, u32 id) {
    if (pal == nullptr) {
        return nullptr;
    }
    HostPalette* host = getHost(pal);
    if (id >= host->descriptors.size()) {
        return nullptr;
    }
    return &host->descriptors[id];
}

}  // extern "C"
