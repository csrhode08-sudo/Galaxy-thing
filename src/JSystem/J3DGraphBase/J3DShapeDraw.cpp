#include "JSystem/J3DGraphBase/J3DShapeDraw.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include <cstring>
#include <revolution/gx/GXDispList.h>
#include <stdint.h>

#ifdef TARGET_PC
#include <aurora/dl.hpp>

namespace {
    // Display lists are copied out of the (shared, possibly reloaded) file data. From Dusklight (CC0).
    void set_display_list_copy(void*& displayList, u32& displayListSize, const u8* data, u32 size) {
        const u32 alignedSize = ALIGN_NEXT(size, 0x20);
        u8* newDL = new (JKRHeapToken::Dummy, 0x20) u8[alignedSize];
        if (size != 0) {
            memcpy(newDL, data, size);
        }
        for (u32 i = size; i < alignedSize; i++) {
            newDL[i] = 0;
        }
        displayList = newDL;
        displayListSize = alignedSize;
    }
}  // namespace
#endif

#ifdef TARGET_PC
u32 J3DShapeDraw::countVertex(u32 stride) {
    u32 count = 0;
    aurora::gx::dl::Reader reader{static_cast< u8* >(getDisplayList()), getDisplayListSize(), static_cast< u8 >(stride)};
    while (const auto cmd = reader.next()) {
        if (cmd->kind != aurora::gx::dl::Command::Kind::Passthrough) {
            count += cmd->draw.vtxCount;
        }
    }
    return count;
}

void J3DShapeDraw::addTexMtxIndexInDL(u32 stride, u32 attrOffs, u32 valueBase) {
    u32 byteNum = countVertex(stride);
    u32 newSize = ALIGN_NEXT(mDisplayListSize + byteNum, 0x20);
    u8* newDLStart = new (JKRHeapToken::Dummy, 0x20) u8[newSize];
    u8* oldDLStart = static_cast< u8* >(mDisplayList);
    u8* newDL = newDLStart;

    aurora::gx::dl::Reader reader{oldDLStart, mDisplayListSize, static_cast< u8 >(stride)};
    while (const auto cmd = reader.next()) {
        if (cmd->kind == aurora::gx::dl::Command::Kind::Passthrough) {
            memcpy(newDL, cmd->data, cmd->size);
            newDL += cmd->size;
            continue;
        }

        const auto& draw = cmd->draw;
        const u32 headerSize = draw.vertices - cmd->data;
        memcpy(newDL, cmd->data, headerSize);
        newDL += headerSize;

        for (u32 i = 0; i < draw.vtxCount; i++) {
            const u8* oldVtx = draw.vertices + stride * i;
            u8 pnmtxidx = oldVtx[0];
            memcpy(newDL, oldVtx, attrOffs);
            newDL += attrOffs;
            *newDL++ = valueBase + pnmtxidx;
            memcpy(newDL, oldVtx + attrOffs, stride - attrOffs);
            newDL += stride - attrOffs;
        }
    }
    if (reader.failed()) {
        memcpy(newDL, oldDLStart + reader.pos(), mDisplayListSize - reader.pos());
        newDL += mDisplayListSize - reader.pos();
    }

    u32 realSize = ALIGN_NEXT(reinterpret_cast< uintptr_t >(newDL) - reinterpret_cast< uintptr_t >(newDLStart), 0x20);
    for (; (u32)(newDL - newDLStart) < newSize; newDL++)
        *newDL = 0;

    mDisplayListSize = realSize;
    mDisplayList = newDLStart;
}

J3DShapeDraw::J3DShapeDraw(const u8* displayList, u32 displayListSize, const GXVtxDescList* vtxDesc) {
    if (const auto optimized = aurora::gx::dl::optimize(displayList, displayListSize, vtxDesc)) {
        set_display_list_copy(mDisplayList, mDisplayListSize, optimized->data(), optimized->size());
    } else {
        set_display_list_copy(mDisplayList, mDisplayListSize, displayList, displayListSize);
    }
}
#else
u32 J3DShapeDraw::countVertex(u32 stride) {
    u32 count = 0;
    u8* dlStart = static_cast< u8* >(getDisplayList());

    for (u8* dl = dlStart; (dl - dlStart) < getDisplayListSize();) {
        u8 cmd = *dl;
        dl++;
        if (cmd != GX_TRIANGLEFAN && cmd != GX_TRIANGLESTRIP)
            break;
        int vtxNum = *reinterpret_cast< u16* >(dl);
        dl += 2;
        count += vtxNum;
        dl = static_cast< u8* >(dl) + stride * vtxNum;
    }

    return count;
}

void J3DShapeDraw::addTexMtxIndexInDL(u32 stride, u32 attrOffs, u32 valueBase) {
    u32 newSize = ALIGN_NEXT(countVertex(stride) + mDisplayListSize, 0x20);
    u8* newDLStart = new (JKRHeapToken::Dummy, 0x20) u8[newSize];
    u8* oldDLStart = static_cast< u8* >(mDisplayList);
    u8* oldDL = oldDLStart;
    u8* newDL = newDLStart;

    for (; (oldDL - oldDLStart) < mDisplayListSize;) {
        // Copy command
        u8 cmd = *oldDL;
        oldDL++;
        *newDL++ = cmd;

        if (cmd != GX_TRIANGLEFAN && cmd != GX_TRIANGLESTRIP)
            break;

        // Copy count
        int vtxNum = *reinterpret_cast< u16* >(oldDL);
        oldDL += 2;
        *reinterpret_cast< u16* >(newDL) = vtxNum;
        newDL += 2;

        for (int i = 0; i < vtxNum; i++) {
            u8* oldDLVtx = &oldDL[stride * i];
            u8 pnmtxidx = *oldDLVtx;
            memcpy(newDL, oldDLVtx, static_cast< int >(attrOffs));
            newDL += attrOffs;
            *newDL++ = valueBase + pnmtxidx;
            memcpy(newDL, oldDLVtx + attrOffs, stride - attrOffs);
            newDL += (stride - attrOffs);
        }

        oldDL = static_cast< u8* >(oldDL) + stride * vtxNum;
    }

    u32 realSize = ALIGN_NEXT(reinterpret_cast< uintptr_t >(newDL) - reinterpret_cast< uintptr_t >(newDLStart), 0x20);
    for (; (newDL - newDLStart) < newSize; newDL++)
        *newDL = 0;

    mDisplayListSize = realSize;
    mDisplayList = newDLStart;
    DCStoreRange(newDLStart, mDisplayListSize);
}

#endif

J3DShapeDraw::J3DShapeDraw(const u8* displayList, u32 displayListSize) {
#ifdef TARGET_PC
    set_display_list_copy(mDisplayList, mDisplayListSize, displayList, displayListSize);
#else
    mDisplayList = const_cast< u8* >(displayList);
    mDisplayListSize = displayListSize;
#endif
}

void J3DShapeDraw::draw() const {
    GXCallDisplayList(mDisplayList, mDisplayListSize);
}

J3DShapeDraw::~J3DShapeDraw() {
}
