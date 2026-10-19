#!/bin/bash
# Runs the game in the cloud dev container: software GL (llvmpipe) on a virtual X display.
# usage: run_cloud.sh [game files dir] [extra args...]
GAME_DIR=${1:-/home/claude/gamedata/files}
shift
export DISPLAY=:99
if ! pgrep -x Xvfb >/dev/null; then
    Xvfb :99 -screen 0 1280x960x24 >/dev/null 2>&1 &
    sleep 1
fi
export LD_LIBRARY_PATH=/home/claude/deps/gl:$LD_LIBRARY_PATH
export LIBGL_ALWAYS_SOFTWARE=1
export SDL_AUDIO_DRIVER=dummy
exec "$(dirname "$0")/../build/galaxy" --disc "$GAME_DIR" "$@"
