#!/usr/bin/env bash
set -euo pipefail

package_root="${1:?usage: verify-package.sh PACKAGE_ROOT [windows|linux|macos]}"
platform="${2:-}"

if [[ ! -d "$package_root" ]]; then
    echo "ERROR: package root does not exist: $package_root" >&2
    exit 1
fi

find_tts_root() {
    find "$package_root" -type d -path '*/resources/tts' -print -quit
}

tts_root="$(find_tts_root)"
if [[ -z "$tts_root" ]]; then
    echo "ERROR: resources/tts is missing from package: $package_root" >&2
    exit 1
fi

required_common=(
    "espeak-ng-data/phondata"
    "voices/ru_RU-dmitri-medium.onnx"
    "voices/ru_RU-dmitri-medium.onnx.json"
    "voices/ru_RU-irina-medium.onnx"
    "voices/ru_RU-irina-medium.onnx.json"
    "voices/en_US-joe-medium.onnx"
    "voices/en_US-joe-medium.onnx.json"
    "voices/en_US-amy-medium.onnx"
    "voices/en_US-amy-medium.onnx.json"
)

for relative in "${required_common[@]}"; do
    if [[ ! -f "$tts_root/$relative" ]]; then
        echo "ERROR: missing packaged TTS file: $tts_root/$relative" >&2
        exit 1
    fi
done

case "$platform" in
    windows)
        test -f "$tts_root/runtime/piper.dll"
        test -f "$tts_root/runtime/onnxruntime.dll"
        ;;
    linux)
        compgen -G "$tts_root/runtime/libpiper.so*" >/dev/null
        compgen -G "$tts_root/runtime/libonnxruntime.so*" >/dev/null
        ;;
    macos)
        test -f "$tts_root/runtime/libpiper.dylib"
        compgen -G "$tts_root/runtime/libonnxruntime*.dylib" >/dev/null
        ;;
    *)
        echo "ERROR: unsupported platform for package verification: $platform" >&2
        exit 2
        ;;
esac

size_bytes="$(du -sk "$tts_root" | awk '{print $1 * 1024}')"
if (( size_bytes < 100000000 )); then
    echo "ERROR: packaged TTS tree is unexpectedly small: $size_bytes bytes" >&2
    exit 1
fi

printf 'Verified packaged TTS tree: %s (%s bytes)\n' "$tts_root" "$size_bytes"
