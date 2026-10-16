#include "nw4r/ut/RomFont.h"

#include <cstring>

namespace nw4r {
    namespace ut {
        RomFont::RomFont() : mFontHeader(NULL), mAlternateChar('?') {
            mDefaultWidths.left = 0;
            mDefaultWidths.glyphWidth = 0;
            mDefaultWidths.charWidth = 0;
        }

        RomFont::~RomFont() {
        }

#ifdef TARGET_PC
        // The Wii's IPL ROM font is not available on PC. Super Mario Galaxy draws its text with the fonts in its
        // own archives; RomFont only needs to exist (and report an empty font) for nw4r's code to link.
        bool RomFont::Load(void*) {
            return false;
        }
        void* RomFont::Unload() {
            return NULL;
        }
        u32 RomFont::GetRequireBufferSize() {
            return 0;
        }
        int RomFont::GetWidth() const {
            return 0;
        }
        int RomFont::GetHeight() const {
            return 0;
        }
        int RomFont::GetAscent() const {
            return 0;
        }
        int RomFont::GetDescent() const {
            return 0;
        }
        int RomFont::GetBaselinePos() const {
            return 0;
        }
        int RomFont::GetCellHeight() const {
            return 0;
        }
        int RomFont::GetCellWidth() const {
            return 0;
        }
        int RomFont::GetMaxCharWidth() const {
            return 0;
        }
        Font::Type RomFont::GetType() const {
            return Font::TYPE_ROM;
        }
        GXTexFmt RomFont::GetTextureFormat() const {
            return GX_TF_I4;
        }
        int RomFont::GetLineFeed() const {
            return 0;
        }
        const CharWidths RomFont::GetDefaultCharWidths() const {
            return mDefaultWidths;
        }
        void RomFont::SetDefaultCharWidths(const CharWidths& widths) {
            mDefaultWidths = widths;
        }
        bool RomFont::SetAlternateChar(CharCode c) {
            mAlternateChar = c;
            return true;
        }
        void RomFont::SetLineFeed(int) {
        }
        int RomFont::GetCharWidth(CharCode) const {
            return mDefaultWidths.charWidth;
        }
        const CharWidths RomFont::GetCharWidths(CharCode) const {
            return mDefaultWidths;
        }
        void RomFont::GetGlyph(Glyph* glyph, CharCode) const {
            std::memset(glyph, 0, sizeof(*glyph));
        }
        bool RomFont::HasGlyph(CharCode) const {
            return false;
        }
        FontEncoding RomFont::GetEncoding() const {
            return FONT_ENCODING_UTF16;
        }
#endif
    }  // namespace ut
}  // namespace nw4r
