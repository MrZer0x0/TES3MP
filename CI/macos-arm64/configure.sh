#!/usr/bin/env bash
set -euo pipefail

rm -rf build stage
mkdir -p build stage

QT_PREFIX="$(brew --prefix qt@5)"
MYGUI_PREFIX="${RUNNER_TEMP:-$HOME}/MyGUI/install"
if SDL_PREFIX="$(brew --prefix sdl2 2>/dev/null)"; then
  :
else
  SDL_PREFIX="$(brew --prefix sdl2-compat)"
fi
PREFIX_PATH="${QT_PREFIX};${MYGUI_PREFIX};$(brew --prefix boost);$(brew --prefix open-scene-graph);$(brew --prefix ffmpeg);${SDL_PREFIX};$(brew --prefix openal-soft);$(brew --prefix lz4);$(brew --prefix luajit)"
CRABNET_PREFIX="${RUNNER_TEMP:-$HOME}/CrabNet"

cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-Release}" \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET:-12.0}" \
  -DCMAKE_PREFIX_PATH="$PREFIX_PATH" \
  -DCMAKE_INSTALL_PREFIX="${GITHUB_WORKSPACE:-$PWD}/stage" \
  -DCMAKE_C_COMPILER_LAUNCHER=ccache \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  -DOPENMW_OSX_DEPLOYMENT=ON \
  -DBUILD_OPENCS=OFF \
  -DBUILD_WIZARD=OFF \
  -DBUILD_UNITTESTS=OFF \
  -DUSE_SYSTEM_TINYXML=OFF \
  -DOPENMW_USE_SYSTEM_MYGUI=ON \
  -DOPENMW_USE_SYSTEM_OSG=ON \
  -DOPENMW_USE_SYSTEM_BULLET:BOOL=OFF \
  -DBULLET_STATIC:BOOL=ON \
  -DUSE_DOUBLE_PRECISION:BOOL=ON \
  -DRakNet_INCLUDE_DIR="$CRABNET_PREFIX/include/raknet" \
  -DRakNet_LIBRARY_RELEASE="$CRABNET_PREFIX/build/lib/libRakNetLibStatic.a" \
  -DRakNet_LIBRARY_DEBUG="$CRABNET_PREFIX/build/lib/libRakNetLibStatic.a" \
  -DLuaJit_INCLUDE_DIR="$(brew --prefix luajit)/include/luajit-2.1" \
  -DLuaJit_LIBRARY="$(brew --prefix luajit)/lib/libluajit-5.1.dylib"

# Fail immediately if CMake ignored the bundled double-precision Bullet mode.
grep -qx 'OPENMW_USE_SYSTEM_BULLET:BOOL=OFF' build/CMakeCache.txt || {
  echo 'ERROR: macOS configuration selected a system Bullet build.' >&2
  exit 1
}
grep -qx 'USE_DOUBLE_PRECISION:BOOL=ON' build/CMakeCache.txt || {
  echo 'ERROR: bundled Bullet is not configured for double precision.' >&2
  exit 1
}
echo 'ArenaMP macOS: bundled Bullet double-precision configuration verified.'
