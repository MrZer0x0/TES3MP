#!/usr/bin/env bash
set -euo pipefail

cmake --build build --parallel "$(sysctl -n hw.logicalcpu)"
cmake --install build

mkdir -p artifacts
if cpack --config build/CPackConfig.cmake -G DragNDrop; then
  find build -maxdepth 2 -type f -name '*.dmg' -exec cp {} artifacts/ \;
else
  echo "CPack DragNDrop failed; creating tar.gz fallback." >&2
fi
ccache --show-stats || true

if ! find artifacts -type f -name '*.dmg' | grep -q .; then
  ref="${GITHUB_REF_NAME:-manual}"
  ref="${ref//\//-}"
  tar -C stage -czf "artifacts/ArenaMP-macOS-arm64-${ref}.tar.gz" .
fi
