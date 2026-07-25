#!/usr/bin/env bash
set -euo pipefail

work_dir="${1:?usage: prepare-piper.sh WORK_DIR RESOURCE_TTS_DIR [windows|linux|macos]}"
resource_dir="${2:?usage: prepare-piper.sh WORK_DIR RESOURCE_TTS_DIR [windows|linux|macos]}"
platform="${3:-}"

PIPER_TAG="${PIPER_TAG:-v1.6.0}"
VOICE_REF="${PIPER_VOICE_REF:-v1.0.0}"
source_dir="$work_dir/piper1-gpl"
build_dir="$work_dir/build"
install_dir="$work_dir/install"
runtime_dir="$resource_dir/runtime"
voices_dir="$resource_dir/voices"

if [[ -z "$platform" ]]; then
    case "$(uname -s)" in
        MINGW*|MSYS*|CYGWIN*) platform=windows ;;
        Darwin*) platform=macos ;;
        *) platform=linux ;;
    esac
fi

mkdir -p "$work_dir" "$runtime_dir" "$voices_dir"

if [[ ! -d "$source_dir/.git" ]]; then
    rm -rf "$source_dir"
    git clone --depth 1 --branch "$PIPER_TAG" \
        https://github.com/OHF-Voice/piper1-gpl.git "$source_dir"
else
    git -C "$source_dir" fetch --depth 1 origin "refs/tags/$PIPER_TAG:refs/tags/$PIPER_TAG" || true
    git -C "$source_dir" checkout -f "$PIPER_TAG"
fi

rm -rf "$build_dir" "$install_dir"
mkdir -p "$build_dir" "$install_dir"

if [[ "$platform" == windows ]]; then
    cmake -S "$source_dir/libpiper" -B "$build_dir" \
        -G "Visual Studio 17 2022" -A x64 \
        -DCMAKE_INSTALL_PREFIX="$install_dir" \
        -DBUILD_TESTING=OFF
    cmake --build "$build_dir" --config Release --parallel 2
    cmake --install "$build_dir" --config Release
else
    cmake -S "$source_dir/libpiper" -B "$build_dir" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$install_dir" \
        -DBUILD_TESTING=OFF
    cmake --build "$build_dir" --parallel 2
    cmake --install "$build_dir"
fi

# Keep README files but refresh generated runtime content.
find "$runtime_dir" -mindepth 1 -maxdepth 1 \( -type f -o -type l \) ! -name 'README.txt' -delete
rm -rf "$resource_dir/espeak-ng-data"

copy_matches() {
    local pattern="$1"
    while IFS= read -r -d '' file; do
        cp -a "$file" "$runtime_dir/"
    done < <(find "$install_dir" "$build_dir" \( -type f -o -type l \) -name "$pattern" -print0 2>/dev/null)
}

case "$platform" in
    windows)
        copy_matches 'piper.dll'
        copy_matches 'onnxruntime.dll'
        [[ -f "$runtime_dir/piper.dll" && -f "$runtime_dir/onnxruntime.dll" ]] || {
            echo "ERROR: Windows libpiper runtime was not staged" >&2; exit 1; }
        ;;
    linux)
        copy_matches 'libpiper.so*'
        copy_matches 'libonnxruntime.so*'
        compgen -G "$runtime_dir/libpiper.so*" >/dev/null || {
            echo "ERROR: Linux libpiper runtime was not staged" >&2; exit 1; }
        compgen -G "$runtime_dir/libonnxruntime.so*" >/dev/null || {
            echo "ERROR: Linux ONNX Runtime was not staged" >&2; exit 1; }
        ;;
    macos)
        copy_matches 'libpiper.dylib'
        copy_matches 'libonnxruntime*.dylib'
        [[ -f "$runtime_dir/libpiper.dylib" ]] || {
            echo "ERROR: macOS libpiper runtime was not staged" >&2; exit 1; }
        compgen -G "$runtime_dir/libonnxruntime*.dylib" >/dev/null || {
            echo "ERROR: macOS ONNX Runtime was not staged" >&2; exit 1; }
        ;;
    *)
        echo "Unsupported platform: $platform" >&2
        exit 2
        ;;
esac

espeak_dir="$(find "$install_dir" "$build_dir" -type d -name espeak-ng-data -print -quit 2>/dev/null || true)"
if [[ -z "$espeak_dir" ]]; then
    echo "ERROR: espeak-ng-data was not produced by libpiper build" >&2
    exit 1
fi
cp -a "$espeak_dir" "$resource_dir/espeak-ng-data"

# Make adjacent dependencies discoverable for the resource-local Linux fallback.
if [[ "$platform" == linux ]] && command -v patchelf >/dev/null 2>&1; then
    while IFS= read -r -d '' library; do
        patchelf --set-rpath '$ORIGIN' "$library" || true
    done < <(find "$runtime_dir" -maxdepth 1 -type f -name 'libpiper.so*' -print0)
fi

if [[ "$platform" == macos ]] && command -v install_name_tool >/dev/null 2>&1; then
    piper_dylib="$runtime_dir/libpiper.dylib"
    if [[ -f "$piper_dylib" ]]; then
        install_name_tool -id '@loader_path/libpiper.dylib' "$piper_dylib" || true
        while IFS= read -r dependency; do
            [[ "$dependency" == *onnxruntime*.dylib* ]] || continue
            install_name_tool -change "$dependency" "@loader_path/$(basename "$dependency")" "$piper_dylib" || true
        done < <(otool -L "$piper_dylib" | tail -n +2 | awk '{print $1}')
    fi
fi

# Keep license/version information beside the runtime.
for license in "$source_dir/LICENSE.md" "$source_dir/LICENSE"; do
    if [[ -f "$license" ]]; then
        cp "$license" "$runtime_dir/libpiper-LICENSE.${license##*.}"
        break
    fi
done
printf 'libpiper=%s\nvoices=%s\n' "$PIPER_TAG" "$VOICE_REF" > "$runtime_dir/VERSIONS.txt"

base_url="https://huggingface.co/rhasspy/piper-voices/resolve/$VOICE_REF"
download_voice() {
    local relative="$1"
    local name="$2"
    curl --fail --location --retry 4 --retry-delay 3 \
        "$base_url/$relative/$name.onnx" -o "$voices_dir/$name.onnx"
    curl --fail --location --retry 4 --retry-delay 3 \
        "$base_url/$relative/$name.onnx.json" -o "$voices_dir/$name.onnx.json"
    curl --fail --location --retry 4 --retry-delay 3 \
        "$base_url/$relative/MODEL_CARD" -o "$voices_dir/$name.MODEL_CARD"
}

download_voice 'ru/ru_RU/dmitri/medium' 'ru_RU-dmitri-medium'
download_voice 'ru/ru_RU/irina/medium' 'ru_RU-irina-medium'
download_voice 'en/en_US/joe/medium' 'en_US-joe-medium'
download_voice 'en/en_US/amy/medium' 'en_US-amy-medium'

# Known binary checksums from the published voice repository. Amy is retained
# with its model card but is not checksum-pinned because the upstream pointer
# has changed between repository revisions.
if command -v sha256sum >/dev/null 2>&1; then
    printf '%s  %s\n' \
      'f073356ebc4bd0f80c5af58df2953a5988bd5bdab1eb38635ce960b071fbefcb' "$voices_dir/ru_RU-dmitri-medium.onnx" \
      '8ff38212d23da300bbe3705c645e6e5b9475f0bfde01558eb17813e22acaaaaa' "$voices_dir/ru_RU-irina-medium.onnx" \
      '58afce0321b8d9c46d7cdf9c16500cc55a793b4220212dba6b70fb788b3baf06' "$voices_dir/en_US-joe-medium.onnx" \
      | sha256sum --check --strict
fi

printf 'Prepared ArenaMP Piper runtime and voices in %s\n' "$resource_dir"
