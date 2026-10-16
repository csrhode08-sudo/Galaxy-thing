#include "nw4r/ut/ResFont.h"
#include "nw4r/ut/binaryFileFormat.h"

namespace nw4r {
    namespace ut {
#ifndef TARGET_PC
        namespace {
            template < typename T >
            inline void ResolveOffset(T*& rpPtr, void* pBase) {
                rpPtr = reinterpret_cast< T* >(static_cast< char* >(pBase) + reinterpret_cast< s32 >(rpPtr));
            }

        }  // namespace
#endif

#ifdef TARGET_PC
        // Converts a BRFNT's big-endian, offset-linked blocks into native structs with real pointers.
        static FontInformation* BuildHostFontInfo(BinaryFileHeader* fileHeader) {
            u8* base = reinterpret_cast< u8* >(fileHeader);
            const res::FontInformation* finf = nullptr;
            BinaryBlockHeader* blockHeader = reinterpret_cast< BinaryBlockHeader* >(base + fileHeader->headerSize);
            for (int n = 0; n < fileHeader->dataBlocks; n++) {
                if (blockHeader->kind == 'FINF') {
                    finf = reinterpret_cast< const res::FontInformation* >(reinterpret_cast< u8* >(blockHeader) + sizeof(*blockHeader));
                    break;
                }
                blockHeader = reinterpret_cast< BinaryBlockHeader* >(reinterpret_cast< u8* >(blockHeader) + blockHeader->size);
            }
            if (finf == nullptr) {
                return nullptr;
            }

            FontInformation* info = new (JKRHeapToken::Dummy) FontInformation;
            info->fontType = finf->fontType;
            info->linefeed = finf->linefeed;
            info->alterCharIndex = finf->alterCharIndex;
            info->defaultWidth = finf->defaultWidth;
            info->encoding = finf->encoding;
            info->height = finf->height;
            info->width = finf->width;
            info->ascent = finf->ascent;

            const res::FontTextureGlyph* tglp = reinterpret_cast< const res::FontTextureGlyph* >(base + finf->pGlyph);
            FontTextureGlyph* glyph = new (JKRHeapToken::Dummy) FontTextureGlyph;
            glyph->cellWidth = tglp->cellWidth;
            glyph->cellHeight = tglp->cellHeight;
            glyph->baselinePos = tglp->baselinePos;
            glyph->maxCharWidth = tglp->maxCharWidth;
            glyph->sheetSize = tglp->sheetSize;
            glyph->sheetNum = tglp->sheetNum;
            glyph->sheetFormat = tglp->sheetFormat;
            glyph->sheetRow = tglp->sheetRow;
            glyph->sheetLine = tglp->sheetLine;
            glyph->sheetWidth = tglp->sheetWidth;
            glyph->sheetHeight = tglp->sheetHeight;
            glyph->sheetImage = base + tglp->sheetImage;  // texture data stays big-endian for the GPU
            info->pGlyph = glyph;

            FontWidth** widthLink = &info->pWidth;
            for (u32 off = finf->pWidth; off != 0;) {
                const res::FontWidth* src = reinterpret_cast< const res::FontWidth* >(base + off);
                u32 count = (u32)src->indexEnd - (u32)src->indexBegin + 1;
                FontWidth* w = static_cast< FontWidth* >(::operator new(sizeof(FontWidth) + count * sizeof(CharWidths), JKRHeapToken::Dummy, 4));
                w->indexBegin = src->indexBegin;
                w->indexEnd = src->indexEnd;
                w->pNext = nullptr;
                memcpy(w->widthTable, src->widthTable, count * sizeof(CharWidths));
                *widthLink = w;
                widthLink = &w->pNext;
                off = src->pNext;
            }
            *widthLink = nullptr;

            FontCodeMap** mapLink = &info->pMap;
            for (u32 off = finf->pMap; off != 0;) {
                const res::FontCodeMap* src = reinterpret_cast< const res::FontCodeMap* >(base + off);
                u32 count;
                switch ((u16)src->mappingMethod) {
                case 0:
                    count = 1;
                    break;
                case 1:
                    count = (u32)src->ccodeEnd - (u32)src->ccodeBegin + 1;
                    break;
                default:
                    count = 1 + 2 * (u32)src->mapInfo[0];
                    break;
                }
                FontCodeMap* m = static_cast< FontCodeMap* >(::operator new(sizeof(FontCodeMap) + count * sizeof(u16), JKRHeapToken::Dummy, 4));
                m->ccodeBegin = src->ccodeBegin;
                m->ccodeEnd = src->ccodeEnd;
                m->mappingMethod = src->mappingMethod;
                m->reserved = src->reserved;
                m->pNext = nullptr;
                for (u32 i = 0; i < count; i++) {
                    m->mapInfo[i] = src->mapInfo[i];
                }
                *mapLink = m;
                mapLink = &m->pNext;
                off = src->pNext;
            }
            *mapLink = nullptr;
            return info;
        }
#endif

        ResFont::ResFont() {
        }

        ResFont::~ResFont() {
        }

        bool ResFont::SetResource(void* brfnt) {
            FontInformation* pFontInfo = NULL;
            BinaryFileHeader* fileHeader = reinterpret_cast< BinaryFileHeader* >(brfnt);

            if (!IsManaging(NULL)) {
                return false;
            }
#ifdef TARGET_PC
            if (fileHeader->version == 0x104) {
                if (!IsValidBinaryFile(fileHeader, 'RFNT', 0x104, 2)) {
                    return false;
                }
            } else if (!IsValidBinaryFile(fileHeader, 'RFNT', 0x102, 2)) {
                return false;
            }
            pFontInfo = BuildHostFontInfo(fileHeader);
            if (false) {
#else
            if (fileHeader->signature == 'RFNU') {
#endif
                BinaryBlockHeader* blockHeader;
                int nBlocks = 0;

                blockHeader = reinterpret_cast< BinaryBlockHeader* >(reinterpret_cast< u8* >(fileHeader) + fileHeader->headerSize);

                while (nBlocks < fileHeader->dataBlocks) {
                    if (blockHeader->kind == 'FINF') {
                        pFontInfo = reinterpret_cast< FontInformation* >(reinterpret_cast< u8* >(blockHeader) + sizeof(*blockHeader));
                        break;
                    }

                    blockHeader = reinterpret_cast< BinaryBlockHeader* >(reinterpret_cast< u8* >(blockHeader) + blockHeader->size);
                    nBlocks++;
                }
            } else {
                if (fileHeader->version == 0x104) {
                    if (!IsValidBinaryFile(fileHeader, 'RFNT', 0x104, 2)) {
                        return false;
                    }
                } else {
                    if (!IsValidBinaryFile(fileHeader, 'RFNT', 0x102, 2)) {
                        return false;
                    }
                }
                pFontInfo = Rebuild(fileHeader);
            }

            if (pFontInfo == NULL) {
                return false;
            }

            SetResourceBuffer(brfnt, pFontInfo);
            InitReaderFunc(GetEncoding());

            return true;
        }

        void ResFont::RemoveResource() {
            RemoveResourceBuffer();
        }

#ifdef TARGET_PC
        FontInformation* ResFont::Rebuild(BinaryFileHeader* fileHeader) {
            return BuildHostFontInfo(fileHeader);
        }
#else
        FontInformation* ResFont::Rebuild(BinaryFileHeader* fileHeader) {
            BinaryBlockHeader* blockHeader;
            FontInformation* info = nullptr;
            int nBlocks = 0;

            blockHeader = reinterpret_cast< BinaryBlockHeader* >(reinterpret_cast< u8* >(fileHeader) + fileHeader->headerSize);

            while (nBlocks < fileHeader->dataBlocks) {
                switch (blockHeader->kind) {
                case 'FINF': {
                    info = reinterpret_cast< FontInformation* >(reinterpret_cast< u8* >(blockHeader) + sizeof(*blockHeader));
                    ResolveOffset(info->pGlyph, fileHeader);

                    if (info->pWidth != nullptr) {
                        ResolveOffset(info->pWidth, fileHeader);
                    }
                    if (info->pMap != nullptr) {
                        ResolveOffset(info->pMap, fileHeader);
                    }
                } break;

                case 'TGLP': {
                    FontTextureGlyph* glyph = reinterpret_cast< FontTextureGlyph* >(reinterpret_cast< u8* >(blockHeader) + sizeof(*blockHeader));
                    ResolveOffset(glyph->sheetImage, fileHeader);
                } break;

                case 'CWDH': {
                    FontWidth* width = reinterpret_cast< FontWidth* >(reinterpret_cast< u8* >(blockHeader) + sizeof(*blockHeader));

                    if (width->pNext != nullptr) {
                        ResolveOffset(width->pNext, fileHeader);
                    }
                } break;

                case 'CMAP': {
                    FontCodeMap* map = reinterpret_cast< FontCodeMap* >(reinterpret_cast< u8* >(blockHeader) + sizeof(*blockHeader));

                    if (map->pNext != nullptr) {
                        ResolveOffset(map->pNext, fileHeader);
                    }
                } break;

                case 'GLGR': {
                } break;
                default:
                    return nullptr;
                }

                blockHeader = reinterpret_cast< BinaryBlockHeader* >(reinterpret_cast< u8* >(blockHeader) + blockHeader->size);
                nBlocks++;
            }

            fileHeader->signature = 'RFNU';

            return info;
        };
#endif
    };  // namespace ut
};  // namespace nw4r
