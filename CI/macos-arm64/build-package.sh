#!/usr/bin/env bash
set -euo pipefail

JOBS="$(sysctl -n hw.logicalcpu)"
QT_PREFIX="$(brew --prefix qt@5)"

rm -rf stage artifacts/dmg-root
mkdir -p stage artifacts

cmake --build build --parallel "$JOBS"
cmake --install build

APP_PATH=""
for candidate in stage/*.app; do
  if [[ -d "$candidate" ]]; then
    APP_PATH="$candidate"
    break
  fi
done
if [[ -z "$APP_PATH" ]]; then
  echo "ERROR: cmake --install did not produce an application bundle in stage/." >&2
  find stage -print >&2 || true
  exit 1
fi

# The framework under Contents/lib is only a staging copy used by
# BundleUtilities to resolve its original @executable_path/../lib install name.
# fixup_bundle must copy and rewrite the final framework into Contents/Frameworks,
# after which the staging copy is removed to avoid shipping Homebrew references.
if [[ -e "$APP_PATH/Contents/lib/MyGUIEngine.framework" ]]; then
  echo "ERROR: temporary MyGUI staging framework remains after cmake --install." >&2
  exit 1
fi
if ! find "$APP_PATH/Contents/Frameworks/MyGUIEngine.framework" -type f \
    -path '*/Versions/*/MyGUIEngine' -print -quit | grep -q .; then
  echo "ERROR: fixed MyGUI framework is missing from Contents/Frameworks." >&2
  exit 1
fi

# CMake BundleUtilities handles non-Qt dependencies. macdeployqt then performs
# the Qt-specific deployment pass before the final signature is created. The
# bundle contains more than one Qt executable, so explicitly include the
# launcher and browser in the deployment scan.
MACDEPLOYQT_ARGS=(
  "$APP_PATH"
  -always-overwrite
  -verbose=2
  "-executable=$APP_PATH/Contents/MacOS/tes3mp-browser"
)
"$QT_PREFIX/bin/macdeployqt" "${MACDEPLOYQT_ARGS[@]}"

# Homebrew libraries may retain absolute LC_RPATH entries even after their
# dependency install names have been rewritten into the bundle.
bash CI/macos-arm64/sanitize-bundle.sh "$APP_PATH"
bash CI/macos-arm64/validate-bundle.sh "$APP_PATH" unsigned artifacts/macos-bundle-pre-sign.txt

# Sign after every fixup/deployment mutation and before creating the DMG.
# Ad-hoc signing is suitable for CI artifacts; notarization can be added later
# when an Apple Developer identity is available. Sign leaf code explicitly
# instead of using --deep because osgPlugins-* is a plain plugin directory, not
# a macOS bundle.
bash CI/macos-arm64/sign-bundle.sh "$APP_PATH"
bash CI/macos-arm64/validate-bundle.sh "$APP_PATH" signed artifacts/macos-bundle-validation.txt

DMG_ROOT="artifacts/dmg-root"
rm -rf "$DMG_ROOT"
mkdir -p "$DMG_ROOT"
ditto "$APP_PATH" "$DMG_ROOT/ArenaMP.app"
ln -s /Applications "$DMG_ROOT/Applications"

ref="${GITHUB_REF_NAME:-manual}"
ref="${ref//\//-}"
DMG_PATH="artifacts/ArenaMP-macOS-arm64-${ref}.dmg"
rm -f "$DMG_PATH"
hdiutil create -volname "ArenaMP" -srcfolder "$DMG_ROOT" -ov -format UDZO -fs HFS+ "$DMG_PATH"
hdiutil verify "$DMG_PATH"

# Mount the final image once and verify the exact application users receive.
MOUNT_DIR="$(mktemp -d /tmp/arenamp-dmg.XXXXXX)"
cleanup() {
  hdiutil detach "$MOUNT_DIR" -force >/dev/null 2>&1 || true
  rmdir "$MOUNT_DIR" >/dev/null 2>&1 || true
}
trap cleanup EXIT
hdiutil attach "$DMG_PATH" -nobrowse -readonly -mountpoint "$MOUNT_DIR" >/dev/null
bash CI/macos-arm64/validate-bundle.sh "$MOUNT_DIR/ArenaMP.app" signed artifacts/macos-dmg-validation.txt
hdiutil detach "$MOUNT_DIR" >/dev/null
rmdir "$MOUNT_DIR"
trap - EXIT

rm -rf "$DMG_ROOT"
shasum -a 256 "$DMG_PATH" > "$DMG_PATH.sha256"
ccache --show-stats | tee artifacts/ccache-stats.txt || true

echo "ArenaMP macOS package created: $DMG_PATH"
