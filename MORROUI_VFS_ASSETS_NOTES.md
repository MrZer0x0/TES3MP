# MorroUI VFS assets integrated

This tree includes the non-code MorroUI assets from `data.zip` that are required by the MorroUI inventory/trade/spell layouts and widgets.

Added into `files/vfs/`:
- `textures/ui/*.dds`
- `textures/ui/gradients/fade_up.dds`
- `sound/fx/inventory hover.mp3`

Added as optional extra content under `files/morroui/`:
- `morroUI.omwaddon`
- `CREDITS.txt`

Why the addon is separate:
- `morroUI.omwaddon` defines the `inventory hover` sound record.
- The actual binary/source port now contains the sound file in VFS.
- If you want the sound to exist as a content record as well, enable `files/morroui/morroUI.omwaddon` manually in your data path / OpenMW config.

Build change:
- `files/vfs/CMakeLists.txt` now copies the MorroUI VFS textures and sound into `resources/vfs/...` during build.
