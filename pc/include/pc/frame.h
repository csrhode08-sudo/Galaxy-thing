// Per-frame hooks called from the game's main loop (GameSystem::frameLoop).
#pragma once

namespace pc {
void frameBegin();  // pump events, update input, begin the GPU frame
void frameEnd();    // draw overlays (controls menu), present, emulate the VI retrace
}  // namespace pc
