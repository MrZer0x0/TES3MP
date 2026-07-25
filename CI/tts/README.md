# Piper build integration

`prepare-piper.sh` pins `piper1-gpl` to `v1.6.0`, builds `libpiper`, stages its
runtime and `espeak-ng-data` under `files/tts`, and downloads the four configured
RU/EN male/female voice models before ArenaMP is configured.

Usage:

```bash
CI/tts/prepare-piper.sh /tmp/arenamp-piper files/tts linux
```

The voice files include their individual model cards. Review those cards before
public distribution because model/dataset licensing is separate from libpiper's
GPL-3.0 license.
