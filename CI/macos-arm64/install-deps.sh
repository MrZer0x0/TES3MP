#!/bin/bash -e

export HOMEBREW_NO_AUTO_UPDATE=1

brew install cmake ninja ccache qt@5 boost sdl2 openal-soft ffmpeg \
  bullet open-scene-graph lz4 tinyxml libunshield pkg-config

echo "$(brew --prefix qt@5)/bin" >> "$GITHUB_PATH"

# Homebrew no longer ships MyGUI. Build the last stable 3.x release natively.
git clone --depth 1 --branch MyGUI3.4.3 https://github.com/MyGUI/mygui.git "$HOME/MyGUI"
cmake -S "$HOME/MyGUI" -B "$HOME/MyGUI/build" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET:-12.0}" \
  -DCMAKE_INSTALL_PREFIX="$HOME/MyGUI/install" \
  -DMYGUI_BUILD_DEMOS=OFF \
  -DMYGUI_BUILD_TOOLS=OFF \
  -DMYGUI_BUILD_PLUGINS=OFF \
  -DMYGUI_BUILD_UNITTESTS=OFF \
  -DMYGUI_BUILD_WRAPPER=OFF
cmake --build "$HOME/MyGUI/build" --parallel "$(sysctl -n hw.logicalcpu)"
cmake --install "$HOME/MyGUI/build"
