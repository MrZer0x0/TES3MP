#!/usr/bin/env bash
set -euo pipefail

export HOMEBREW_NO_AUTO_UPDATE=1
export HOMEBREW_CACHE="${HOMEBREW_CACHE:-$HOME/Library/Caches/Homebrew}"

brew install cmake ninja ccache qt@5 boost sdl2 openal-soft ffmpeg \
  bullet open-scene-graph lz4 unshield pkg-config luajit

echo "$(brew --prefix qt@5)/bin" >> "$GITHUB_PATH"

TINYXML_ROOT="${RUNNER_TEMP:-$HOME}/TinyXML"
TINYXML_PREFIX="$TINYXML_ROOT/install"
if [ ! -f "$TINYXML_PREFIX/lib/libtinyxml.a" ] || [ ! -f "$TINYXML_PREFIX/include/tinyxml.h" ]; then
  rm -rf "$TINYXML_ROOT/source" "$TINYXML_ROOT/build"
  mkdir -p "$TINYXML_ROOT" "$TINYXML_ROOT/build" "$TINYXML_PREFIX/lib" "$TINYXML_PREFIX/include"
  git clone --depth 1 --branch 2.6.2 https://github.com/robotology-dependencies/tinyxml.git "$TINYXML_ROOT/source"

  CXX_BIN="${CXX:-clang++}"
  for src in tinyxml.cpp tinyxmlerror.cpp tinyxmlparser.cpp tinystr.cpp; do
    ccache "$CXX_BIN" -std=c++11 -O2 -DNDEBUG -fPIC -arch arm64 \
      -mmacosx-version-min="${MACOSX_DEPLOYMENT_TARGET:-12.0}" \
      -I"$TINYXML_ROOT/source" -c "$TINYXML_ROOT/source/$src" \
      -o "$TINYXML_ROOT/build/${src%.cpp}.o"
  done
  libtool -static -o "$TINYXML_PREFIX/lib/libtinyxml.a" "$TINYXML_ROOT/build"/*.o
  cp "$TINYXML_ROOT/source/tinyxml.h" "$TINYXML_ROOT/source/tinystr.h" "$TINYXML_PREFIX/include/"
fi

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
