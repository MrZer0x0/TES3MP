# ArenaMP chat TTS runtime

ArenaMP loads `libpiper` dynamically and synthesizes incoming player chat on the
client. Generated mono PCM is passed to the normal actor speech stream, so it is
positioned at the character's head and participates in the existing speech
loudness/lip-animation path.

Expected installed layout:

```
resources/tts/
├── espeak-ng-data/
├── runtime/                 # optional resource-local libpiper fallback
└── voices/
    ├── ru_RU-dmitri-medium.onnx
    ├── ru_RU-dmitri-medium.onnx.json
    ├── ru_RU-irina-medium.onnx          # optional female voice
    ├── ru_RU-irina-medium.onnx.json
    ├── en_US-joe-medium.onnx
    ├── en_US-joe-medium.onnx.json
    ├── en_US-amy-medium.onnx            # female voice
    └── en_US-amy-medium.onnx.json
```

The female model automatically falls back to the corresponding male model when
it is absent. The CI helper downloads all four configured voices and preserves
each `MODEL_CARD`; review those model/dataset terms before public distribution.

Runtime library lookup order:

1. `[Chat TTS] library path`
2. `resources/tts/runtime/<platform library>`
3. the normal operating-system library path (`piper.dll`, `libpiper.so`, or
   `libpiper.dylib`)
