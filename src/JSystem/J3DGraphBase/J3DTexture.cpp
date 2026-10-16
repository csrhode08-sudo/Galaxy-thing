#include "JSystem/J3DGraphBase/J3DTexture.hpp"

// Not present in the decompilation; after Dusklight (CC0), with the PC path binding prebuilt texture objects.
void J3DTexture::loadGX(u16 idx, GXTexMapID texMapID) const {
    ResTIMG* timg = getResTIMG(idx);
#ifdef TARGET_PC
    if (idx >= mNum || timg->mWidth == 0 || timg->mHeight == 0) {
        return;
    }
    if (timg->mPaletteName) {
        GXLoadTlut(&mpTlutObj[idx], (GXTlut)texMapID);
        GXInitTexObjTlut(&mpTexObj[idx], (GXTlut)texMapID);
    }
    GXLoadTexObj(&mpTexObj[idx], texMapID);
#else
    GXTexObj texObj;
    GXTlutObj tlutObj;
    if (!timg->mPaletteName) {
        GXInitTexObj(&texObj, ((u8*)timg) + timg->mImageDataOffset, timg->mWidth, timg->mHeight, (GXTexFmt)timg->mFormat,
                     (GXTexWrapMode)timg->mWrapS, (GXTexWrapMode)timg->mWrapT, timg->mMipmap);
    } else {
        GXInitTexObjCI(&texObj, ((u8*)timg) + timg->mImageDataOffset, timg->mWidth, timg->mHeight, (GXCITexFmt)timg->mFormat,
                       (GXTexWrapMode)timg->mWrapS, (GXTexWrapMode)timg->mWrapT, timg->mMipmap, (u32)texMapID);
        GXInitTlutObj(&tlutObj, ((u8*)timg) + timg->mPaletteDataOffset, (GXTlutFmt)timg->mPaletteFormat, timg->mPaletteNum);
        GXLoadTlut(&tlutObj, texMapID);
    }
    GXInitTexObjLOD(&texObj, (GXTexFilter)timg->mMinType, (GXTexFilter)timg->mMagType, (s8)timg->mMinLod / 8.0f, (s8)timg->mMaxLod / 8.0f,
                    timg->mLodBias / 100.0f, timg->mBiasClamp, timg->mDoEdgeLod, (GXAnisotropy)timg->mMaxAnisotropy);
    GXLoadTexObj(&texObj, texMapID);
#endif
}

#ifdef TARGET_PC
void J3DTexture::loadGXTexObj(u16 idx) {
    ResTIMG* timg = getResTIMG(idx);
    GXTlutObj& tlutObj = mpTlutObj[idx];
    GXTexObj& texObj = mpTexObj[idx];

    if (!timg->mPaletteName) {
        GXInitTexObj(&texObj, mpImgDataPtr[idx], timg->mWidth, timg->mHeight, (GXTexFmt)timg->mFormat, (GXTexWrapMode)timg->mWrapS,
                     (GXTexWrapMode)timg->mWrapT, timg->mMipmap);
    } else {
        GXInitTexObjCI(&texObj, mpImgDataPtr[idx], timg->mWidth, timg->mHeight, (GXCITexFmt)timg->mFormat, (GXTexWrapMode)timg->mWrapS,
                       (GXTexWrapMode)timg->mWrapT, timg->mMipmap, GX_TLUT0);
        GXInitTlutObj(&tlutObj, mpTlutDataPtr[idx], (GXTlutFmt)timg->mPaletteFormat, timg->mPaletteNum);
    }
    GXInitTexObjLOD(&texObj, (GXTexFilter)timg->mMinType, (GXTexFilter)timg->mMagType, (s8)timg->mMinLod / 8.0f, (s8)timg->mMaxLod / 8.0f,
                    timg->mLodBias / 100.0f, timg->mBiasClamp, timg->mDoEdgeLod, (GXAnisotropy)timg->mMaxAnisotropy);
}
#endif
