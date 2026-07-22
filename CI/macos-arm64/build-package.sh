#!/usr/bin/env bash
set -euo pipefail

cmake --build build --parallel "$(sysctl -n hw.logicalcpu)"
cmake --install build
cpack --config build/CPackConfig.cmake -G DragNDrop
ccache --show-stats || true

mkdir -p artifacts
find build -maxdepth 2 -type f -name '*.dmg' -exec cp {} artifacts/ \;

if ! find artifacts -type f -name '*.dmg' | grep -q .; then
  tar -C stage -czf "artifacts/ArenaMP-macOS-arm64-${GITHUB_REF_NAME:-manual}.tar.gz" .
fi
