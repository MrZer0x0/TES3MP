#!/usr/bin/env bash
set -euo pipefail

export HOMEBREW_NO_AUTO_UPDATE=1
export HOMEBREW_CACHE="${HOMEBREW_CACHE:-$HOME/Library/Caches/Homebrew}"

brew install cmake ninja ccache qt@5 boost sdl2 openal-soft ffmpeg \
  bullet open-scene-graph lz4 tinyxml libunshield pkg-config luajit

echo "$(brew --prefix qt@5)/bin" >> "$GITHUB_PATH"

MYGUI_ROOT="${RUNNER_TEMP:-$HOME}/MyGUI"
if [ ! -f "$MYGUI_ROOT/install/lib/cmake/MyGUI/MyGUIConfig.cmake" ] && \
   [ ! -f "$MYGUI_ROOT/install/lib/MyGUI/cmake/MyGUIConfig.cmake" ]; then
  if [ ! -d "$MYGUI_ROOT/.git" ]; then
    git clone --depth 1 --branch MyGUI3.4.3 https://github.com/MyGUI/mygui.git "$MYGUI_ROOT"
  else
    git -C "$MYGUI_ROOT" fetch --depth 1 origin tag MyGUI3.4.3
    git -C "$MYGUI_ROOT" reset --hard MyGUI3.4.3
  fi
  cmake -S "$MYGUI_ROOT" -B "$MYGUI_ROOT/build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET:-12.0}" \
    -DCMAKE_C_COMPILER_LAUNCHER=ccache \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DCMAKE_INSTALL_PREFIX="$MYGUI_ROOT/install" \
    -DMYGUI_BUILD_DEMOS=OFF \
    -DMYGUI_BUILD_TOOLS=OFF \
    -DMYGUI_BUILD_PLUGINS=OFF \
    -DMYGUI_BUILD_UNITTESTS=OFF \
    -DMYGUI_BUILD_WRAPPER=OFF
  cmake --build "$MYGUI_ROOT/build" --parallel "$(sysctl -n hw.logicalcpu)"
  cmake --install "$MYGUI_ROOT/build"
fi
