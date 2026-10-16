#pragma once

#include "JSystem/J3DGraphBase/J3DTevs.hpp"
#include "JSystem/JUtility/JUTTexture.hpp"
#include <stdint.h>

class J3DTexture {
private:
    /* 0x0 */ u16 mNum;
    /* 0x2 */ u16 unk_0x2;
    /* 0x4 */ ResTIMG* mpRes;
#ifdef TARGET_PC
    // Textures are bound as texture objects on PC (display lists cannot carry host image pointers).
    GXTlutObj* mpTlutObj;
    GXTexObj* mpTexObj;
    u8** mpImgDataPtr;
    u8** mpTlutDataPtr;
#endif

public:
    J3DTexture(u16 num, ResTIMG* res) : mNum(num), unk_0x2(0), mpRes(res) {
#ifdef TARGET_PC
        mpTexObj = new GXTexObj[num];
        mpTlutObj = new GXTlutObj[num];
        mpImgDataPtr = new u8*[num];
        mpTlutDataPtr = new u8*[num];
        for (u16 i = 0; i < num; i++) {
            mpImgDataPtr[i] = (u8*)(&mpRes[i]) + mpRes[i].mImageDataOffset;
            mpTlutDataPtr[i] = (u8*)(&mpRes[i]) + mpRes[i].mPaletteDataOffset;
            loadGXTexObj(i);
        }
#endif
    }

    void loadGX(u16, GXTexMapID) const;
    void entryNum(u16);
    void addResTIMG(u16, ResTIMG const*);
#ifdef TARGET_PC
    void loadGXTexObj(u16);
#endif
    virtual ~J3DTexture() {
#ifdef TARGET_PC
        delete[] mpTexObj;
        delete[] mpTlutObj;
        delete[] mpImgDataPtr;
        delete[] mpTlutDataPtr;
#endif
    }

    u16 getNum() const {
        return mNum;
    }

    ResTIMG* getResTIMG(u16 index) const {
        return &mpRes[index];
    }

#ifdef TARGET_PC
    u8* getImgDataPtr(u16 index) const {
        return mpImgDataPtr[index];
    }
#endif

    void setResTIMG(u16 index, const ResTIMG& timg) {
        mpRes[index] = timg;
#ifdef TARGET_PC
        mpImgDataPtr[index] = ((u8*)&timg) + timg.mImageDataOffset;
        mpTlutDataPtr[index] = ((u8*)&timg) + timg.mPaletteDataOffset;
        loadGXTexObj(index);
#else
        mpRes[index].mImageDataOffset = ((mpRes[index].mImageDataOffset + (uintptr_t)&timg - (uintptr_t)(mpRes + index)));
        mpRes[index].mPaletteDataOffset = ((mpRes[index].mPaletteDataOffset + (uintptr_t)&timg - (uintptr_t)(mpRes + index)));
#endif
    }
};

extern J3DTexMtxInfo const j3dDefaultTexMtxInfo;

class J3DTexMtx {
public:
    J3DTexMtx() {
        mTexMtxInfo = j3dDefaultTexMtxInfo;
    }

    J3DTexMtx(const J3DTexMtxInfo& info) {
        mTexMtxInfo = info;
    }

    void load(u32) const;
    void calc(const Mtx);
    void calcTexMtx(const Mtx);
    void calcPostTexMtx(const Mtx);
    inline void loadTexMtx(u32) const;
    inline void loadPostTexMtx(u32) const;

    J3DTexMtxInfo& getTexMtxInfo() {
        return mTexMtxInfo;
    }
    Mtx& getMtx() {
        return mMtx;
    }
    void setEffectMtx(Mtx effectMtx) {
        mTexMtxInfo.setEffectMtx(effectMtx);
    }

    /* 0x00 */ J3DTexMtxInfo mTexMtxInfo;

private:
    /* 0x64 */ Mtx mMtx;
};  // Size: 0x94
