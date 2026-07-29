# TTS packaging fix

The Windows Visual Studio generator is multi-config. The previous TTS resource
copy wrote files to `MSVC2022_64/resources/tts`, while `cmake --install` reads
from `MSVC2022_64/Release/resources`. As a result, libpiper, ONNX Runtime,
espeak-ng-data, and all voice models were omitted from the client ZIP.

This revision:

- stages TTS resources through OpenMW `copy_resource_file`, which writes into
  every configuration directory, including `Release/resources/tts`;
- verifies the installed client contains the runtime, phoneme data, and four
  voice models before creating the Windows ZIP;
- preloads adjacent `onnxruntime.dll` before loading `piper.dll` on Windows,
  avoiding dependent-DLL lookup failures from `resources/tts/runtime`.
