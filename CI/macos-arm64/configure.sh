#!/bin/bash -e

rm -rf build stage
mkdir -p build stage

QT_PREFIX="$(brew --prefix qt@5)"
PREFIX_PATH="${QT_PREFIX};$HOME/MyGUI/install;$(brew --prefix boost);$(brew --prefix bullet);$(brew --prefix open-scene-graph);$(brew --prefix ffmpeg);$(brew --prefix sdl2);$(brew --prefix openal-soft);$(brew --prefix lz4);$(brew --prefix tinyxml)"

cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-Release}" \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET:-12.0}" \
  -DCMAKE_PREFIX_PATH="$PREFIX_PATH" \
  -DCMAKE_INSTALL_PREFIX="$GITHUB_WORKSPACE/stage" \
  -DOPENMW_OSX_DEPLOYMENT=ON \
  -DBUILD_OPENCS=OFF \
  -DBUILD_WIZARD=OFF \
  -DUSE_SYSTEM_TINYXML=ON \
  -DOPENMW_USE_SYSTEM_MYGUI=ON \
  -DOPENMW_USE_SYSTEM_OSG=ON \
  -DOPENMW_USE_SYSTEM_BULLET=ON \
  -DRakNet_LIBRARY_RELEASE="$HOME/CrabNet/build/lib/libRakNetLibStatic.a" \
  -DRakNet_LIBRARY_DEBUG="$HOME/CrabNet/build/lib/libRakNetLibStatic.a" \
  -DRakNet_INCLUDE_DIR="$HOME/CrabNet/Source"
