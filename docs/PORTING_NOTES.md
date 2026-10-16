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
| Format | Code | Status |
|---|---|---|
| BCSV (JMapInfo) | src/Game/Util/JMapInfo.cpp | BE fields done |
| Yaz0 | JKRDecomp | header BE done |
| BRFNT fonts | nw4r ut_ResFont | host rebuild done |
| TPL | pc/src/tpl.cpp | host descriptors done |
| SpkRes / ME params / chord tables | Game/Speaker, RhythmLib | done |
| J3D animations (BCK/BTK/BRK/BTP/BVA/BPK) | J3DAnimation.hpp, J3DAnmLoader.cpp | headers BE; TODO swap value arrays in loader |
| RARC archives | JKRArchive* | TODO |
| BMD/BDL models | J3DModelLoader & co | TODO |
| BRLYT/BRLAN | nw4r lyt | TODO |
| JPA particles | JParticle | TODO |
| JAudio (BAA/BMS/AW) | JAudio2 | TODO; DSP emulation from Dusklight |
| BMG messages | Game/Screen/MessageHolder? | TODO |
| KCL collision | Game/Map/KCollision | TODO |
