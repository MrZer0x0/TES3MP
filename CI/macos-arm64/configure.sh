#!/usr/bin/env bash
set -euo pipefail

mkdir -p build stage

QT_PREFIX="$(brew --prefix qt@5)"
MYGUI_PREFIX="${RUNNER_TEMP:-$HOME}/MyGUI/install"
CRABNET_DIR="${RUNNER_TEMP:-$HOME}/CrabNet"
PREFIX_PATH="${QT_PREFIX};${MYGUI_PREFIX};$(brew --prefix boost);$(brew --prefix bullet);$(brew --prefix open-scene-graph);$(brew --prefix ffmpeg);$(brew --prefix sdl2);$(brew --prefix openal-soft);$(brew --prefix lz4);$(brew --prefix tinyxml);$(brew --prefix luajit)"

cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-Release}" \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET:-12.0}" \
  -DCMAKE_PREFIX_PATH="$PREFIX_PATH" \
  -DCMAKE_INSTALL_PREFIX="$GITHUB_WORKSPACE/stage" \
  -DCMAKE_C_COMPILER_LAUNCHER=ccache \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  -DOPENMW_OSX_DEPLOYMENT=ON \
  -DBUILD_OPENCS=OFF \
  -DBUILD_WIZARD=OFF \
  -DUSE_SYSTEM_TINYXML=ON \
  -DOPENMW_USE_SYSTEM_MYGUI=ON \
  -DOPENMW_USE_SYSTEM_OSG=ON \
  -DOPENMW_USE_SYSTEM_BULLET=ON \
  -DLuaJit_INCLUDE_DIR="$(brew --prefix luajit)/include/luajit-2.1" \
  -DLuaJit_LIBRARY="$(brew --prefix luajit)/lib/libluajit-5.1.dylib" \
  -DRakNet_LIBRARY_RELEASE="$CRABNET_DIR/build/lib/libRakNetLibStatic.a" \
  -DRakNet_LIBRARY_DEBUG="$CRABNET_DIR/build/lib/libRakNetLibStatic.a" \
  -DRakNet_INCLUDE_DIR="$CRABNET_DIR/Source"
