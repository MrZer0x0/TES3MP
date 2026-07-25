#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[2]
checks = {
    "chat hook": (root / "apps/openmw/mwmp/processors/player/ProcessorChatMessage.hpp", "getChatTtsManager()->enqueue"),
    "main lifecycle": (root / "apps/openmw/mwmp/Main.cpp", "mChatTtsManager->initialize"),
    "custom speech decoder": (root / "apps/openmw/mwsound/soundmanagerimp.cpp", "const DecoderPtr& decoder"),
    "TTS sources in CMake": (root / "apps/openmw/CMakeLists.txt", "add_openmw_dir (mwmp/tts"),
    "TTS resources in CMake": (root / "files/CMakeLists.txt", "add_subdirectory(tts)"),
    "TTS defaults": (root / "files/settings-default.cfg", "[Chat TTS]"),
    "release CI": (root / ".github/workflows/release-builds.yml", "prepare-piper.sh"),
}

failed = False
for label, (path, marker) in checks.items():
    text = path.read_text(encoding="utf-8")
    if marker not in text:
        print(f"FAIL: {label}: {marker!r} missing from {path}")
        failed = True
    else:
        print(f"OK: {label}")

required = [
    root / "apps/openmw/mwmp/tts/ChatTtsManager.cpp",
    root / "apps/openmw/mwmp/tts/PiperApi.cpp",
    root / "apps/openmw/mwmp/tts/PcmDecoder.cpp",
    root / "apps/openmw/mwmp/tts/LanguageDetector.cpp",
    root / "CI/tts/prepare-piper.sh",
]
for path in required:
    if not path.is_file():
        print(f"FAIL: missing {path}")
        failed = True

raise SystemExit(1 if failed else 0)
