# TES3MP + MorroUI port

This source tree contains a best-effort MorroUI integration ported into the uploaded TES3MP source tree.

## Included
- MorroUI list-style inventory, trade, and spell windows
- MorroUI list widgets and headers registered in WindowManager
- TES3MP multiplayer hooks preserved for:
  - item use
  - selected spell updates
  - spell deletion updates
  - trade gold synchronization
  - object deletion on pickup
- GMST-first localization support via `#{gmst=GMST_ID,Fallback}`
- TES3MP/MorroUI-specific fallback strings moved to `[UI]` in `files/settings-default.cfg`
- MorroUI setting added:
  - `[MorroUI]`
  - `leftclick activates = false`

## Main files touched
- `apps/openmw/mwgui/inventorywindow.*`
- `apps/openmw/mwgui/tradewindow.*`
- `apps/openmw/mwgui/spellwindow.*`
- `apps/openmw/mwgui/itemlistwidget*`
- `apps/openmw/mwgui/spelllistwidget*`
- `apps/openmw/mwgui/itemview.*`
- `apps/openmw/mwgui/spellview.*`
- `apps/openmw/mwgui/spellmodel.*`
- `apps/openmw/mwgui/sortfilteritemmodel.*`
- `files/mygui/openmw_inventory_window.layout`
- `files/mygui/openmw_trade_window.layout`
- `files/mygui/openmw_spell_window.layout`
- `files/mygui/openmw_list.skin.xml`
- `files/mygui/openmw_resources.xml`

## Caveat
This is a source-level port prepared from the uploaded archives and adapted for TES3MP hooks, but it was not fully built and runtime-tested in this environment.
