#pragma once

#include <revolution.h>

namespace nw4r {
    namespace ut {
        struct CharWidths {
            s8 left;
            u8 glyphWidth;
            s8 charWidth;
        };

        struct FontTextureGlyph {
            u8 cellWidth;
            u8 cellHeight;
            s8 baselinePos;
            u8 maxCharWidth;
            u32 sheetSize;
            u16 sheetNum;
            u16 sheetFormat;
            u16 sheetRow;
            u16 sheetLine;
            u16 sheetWidth;
            u16 sheetHeight;
            u8* sheetImage;
        };

        struct FontWidth {
            u16 indexBegin;
            u16 indexEnd;
            FontWidth* pNext;
            CharWidths widthTable[];
        };

        struct FontCodeMap {
            u16 ccodeBegin;
            u16 ccodeEnd;
            u16 mappingMethod;
            u16 reserved;
            FontCodeMap* pNext;
            u16 mapInfo[];
        };

        struct FontInformation {
            u8 fontType;
            s8 linefeed;
            u16 alterCharIndex;
            CharWidths defaultWidth;
            u8 encoding;
            FontTextureGlyph* pGlyph;
            FontWidth* pWidth;
            FontCodeMap* pMap;
            u8 height;
            u8 width;
            u8 ascent;
            u8 padding_[1];
        };
#ifdef TARGET_PC
        // On-disc (big-endian, offset-based) layouts. On PC, ResFont builds the native structs above from these.
        namespace res {
            struct FontTextureGlyph {
                u8 cellWidth;
                u8 cellHeight;
                s8 baselinePos;
                u8 maxCharWidth;
                BE(u32) sheetSize;
                BE(u16) sheetNum;
                BE(u16) sheetFormat;
                BE(u16) sheetRow;
                BE(u16) sheetLine;
                BE(u16) sheetWidth;
                BE(u16) sheetHeight;
                BE(u32) sheetImage;
            };

            struct FontWidth {
                BE(u16) indexBegin;
                BE(u16) indexEnd;
                BE(u32) pNext;
                CharWidths widthTable[];
            };

            struct FontCodeMap {
                BE(u16) ccodeBegin;
                BE(u16) ccodeEnd;
                BE(u16) mappingMethod;
                BE(u16) reserved;
                BE(u32) pNext;
                BE(u16) mapInfo[];
            };

            struct FontInformation {
                u8 fontType;
                s8 linefeed;
                BE(u16) alterCharIndex;
                CharWidths defaultWidth;
                u8 encoding;
                BE(u32) pGlyph;
                BE(u32) pWidth;
                BE(u32) pMap;
                u8 height;
                u8 width;
                u8 ascent;
                u8 padding_[1];
            };
        }  // namespace res
#endif
    };  // namespace ut
};  // namespace nw4r
