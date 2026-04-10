# TES3MP MorroUI GMST-first localization changes

This source tree patches the built-in TES3MP UI so that interface text uses:

1. **GMST first** through layout tags like `#{gmst=sRaceMenu1,Appearance}` for strings that should follow Morrowind/OpenMW localization.
2. **`[UI]` settings keys** through `#{setting=UI,...}` for TES3MP- or MorroUI-specific strings that do not exist in GMST.

## Main code changes

- `apps/openmw/mwgui/windowmanagerimp.cpp/.hpp`
  - Added support for `#{gmst=GMST_ID,Fallback}` tag resolution in MyGUI layouts.
- `apps/openmw/mwgui/savegamedialog.cpp`
  - Localized "Select Character ..." placeholder.
- `apps/openmw/mwgui/waitdialog.cpp`
  - Localized TES3MP wait restriction message.
- `apps/openmw/mwgui/settingswindow.cpp`
  - Localized texture filtering combo captions.
- `apps/openmw/mwmp/GUI/GUIChat.cpp`
  - Localized chat window title.

## Main layout changes

Localized hardcoded captions in:

- `files/mygui/openmw_savegame_dialog.layout`
- `files/mygui/openmw_settings_window.layout`
- `files/mygui/tes3mp_login.layout`
- `files/mygui/openmw_enchanting_dialog.layout`
- `files/mygui/openmw_chargen_birth.layout`
- `files/mygui/openmw_chargen_class_description.layout`
- `files/mygui/openmw_chargen_generate_class_result.layout`
- `files/mygui/openmw_chargen_race.layout`
- `files/mygui/openmw_debug_window.layout`
- `files/mygui/openmw_text_input.layout`
- `files/mygui/tes3mp_dialog_list.layout`
- `files/mygui/tes3mp_text_input.layout`

## String storage

Custom strings were added to `files/settings-default.cfg` under a new `[UI]` section.
They can now be overridden in user/server localization configs without editing layouts.
