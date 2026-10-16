# Super Mario Galaxy PC port — working notes

Base: Petari (SMGCommunity/Petari, CC0) for game code; Aurora (MIT) for GX/VI/PAD/DVD/OS;
Dusklight (CC0) for reference PC fixes to the shared JSystem/nw4r code.

## Conventions
- `TARGET_PC` guards every PC-specific change; original code stays in the `#else` branch where practical.
- `BE(T)` (pc/endian.h) = big-endian field read in place from game files.
- `OFFSET_PTR_V0` = 32-bit file offset that the console patches into a pointer in place; on PC it stays an offset.
- Game `new` expressions carry `JKRHeapToken::Dummy` (tools/rewrite_new.py) so they go to JKRHeap.
- Original-game bugs are preserved and commented (MapPartsRailPosture, Sandstorm).

## Data-format status
| Format | Code | Approach |
|---|---|---|
| BCSV (JMapInfo) | src/Game/Util/JMapInfo.cpp | BE fields |
| Yaz0 | JKRDecomp | BE header reads |
| RARC archives | JKRArchive* | BE structs; loaded-file pointers in a host table (`JKRArchive::fileData`) |
| BRFNT fonts | nw4r ut_ResFont | host font info rebuilt |
| TPL | pc/src/tpl.cpp | host descriptors |
| BRLYT/BRLAN | pc/src/lyt_swap.cpp | whole file swapped in place (BOM marks it converted) |
| BMG messages | pc/src/bmg_swap.cpp | swapped in place; tag params as u16 units (see MessageEditorMessageTag) |
| U8 archives (ISBN screen) | pc/src/arc.cpp | BE reads |
| BMD/BDL models | J3DModelLoader, J3DShapeFactory, J3DJointFactory | BE headers; vertex arrays, vtx descs, joints swapped in place once (marker at file offset 0x18, pc/j3d.h) |
| J3D materials | J3DMaterialFactory | BE indices; tex-mtx/fog/ind-mtx info swapped on copy |
| J3D textures | J3DTexture | bound via GXLoadTexObj (J3DTevBlock::loadTexture), not via DL image pointers |
| J3D display lists | J3DShapeDraw | copied (and optimized) with aurora::gx::dl |
| J3D animations | J3DAnimation.hpp, J3DAnmLoader | BE pointers for most; transform keys swapped in place once |
| JPA particles | JParticle | BE headers/keys |
| KCL collision | Game/Map/KCollision.cpp | swapped in place once, host-side KCLFile header |
| JAudio (BAA/BNK/WSYS/BMS) | JAudio2 | BE structs; sequence reads BE (JASSeqReader); CPU DSP mixer + SDL3 output in pc/src/audio (from Dusklight) |

## Runtime
- Disc: a WBFS/ISO/RVZ image via nod, or a directory of extracted files (`aurora_dvd_open_directory`,
  patches/aurora-dvd-directory.patch applied to extern/aurora).
- Saves: NAND calls map to `<user dir>/save/` (pc/src/sdk_wii.cpp).
- Unavailable Wii services return "not available": Miis, WiiConnect24 messages, HOME menu module (RSO).
- Fixed-size heaps are scaled x2 (`PC_HEAP_SCALE`, HeapMemoryWatcher.cpp).
- `tools/run_cloud.sh` runs under Xvfb with Mesa llvmpipe.
- DOL data tables (galaxy info, story progress, error-message archive) are copied out of the disc's main.dol at
  startup (pc/src/dol_assets.cpp), so no game data is compiled into the executable.
- `overlay/` next to the executable (or `--overlay <dir>`) adds/replaces disc files; with `UsEnglish/` present the
  language switches to English.
- 64-bit pitfalls found at runtime: hard-coded object sizes (root heap header), intrusive-list node offsets
  (nw4r LinkList `-1` / JGadget TLinkList `1` = resolve from the real layout), alignment masks that drop the upper
  pointer bits (ALIGN_PREV), NULL `%s` (CodeWarrior printed nothing).
- Windows: clang-cl, Dawn prebuilt; built by .github/workflows/windows.yml.
