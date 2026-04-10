if (NOT DEFINED OPENMW_MYGUI_FILES_ROOT)
    return()
endif()

# Copy MorroUI/OpenMW VFS resource files into the build directory
set(SDIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DDIRRELATIVE resources/vfs)

set(VFS_FILES
    textures/omw_menu_scroll_down.dds
    textures/omw_menu_scroll_up.dds
    textures/omw_menu_scroll_left.dds
    textures/omw_menu_scroll_right.dds
    textures/omw_menu_scroll_center_h.dds
    textures/omw_menu_scroll_center_v.dds
    textures/crosshair.dds
    textures/sneak.dds

    textures/ui/all_idle.dds
    textures/ui/all_over.dds
    textures/ui/all_pressed.dds
    textures/ui/alteration_idle.dds
    textures/ui/alteration_over.dds
    textures/ui/alteration_pressed.dds
    textures/ui/armor_idle.dds
    textures/ui/armor_over.dds
    textures/ui/armor_pressed.dds
    textures/ui/book_idle.dds
    textures/ui/book_over.dds
    textures/ui/book_overlay.dds
    textures/ui/book_overlay_detail.dds
    textures/ui/book_pressed.dds
    textures/ui/bound.dds
    textures/ui/cloth_idle.dds
    textures/ui/cloth_over.dds
    textures/ui/cloth_pressed.dds
    textures/ui/conjuration_all.dds
    textures/ui/conjuration_idle.dds
    textures/ui/conjuration_over.dds
    textures/ui/conjuration_pressed.dds
    textures/ui/destruction_idle.dds
    textures/ui/destruction_over.dds
    textures/ui/destruction_pressed.dds
    textures/ui/enchanted.dds
    textures/ui/equipped.dds
    textures/ui/favorite.dds
    textures/ui/illusion_idle.dds
    textures/ui/illusion_over.dds
    textures/ui/illusion_pressed.dds
    textures/ui/ingredient_idle.dds
    textures/ui/ingredient_over.dds
    textures/ui/ingredient_pressed.dds
    textures/ui/item_idle.dds
    textures/ui/item_over.dds
    textures/ui/item_pressed.dds
    textures/ui/magic_idle.dds
    textures/ui/magic_over.dds
    textures/ui/magic_pressed.dds
    textures/ui/misc_idle.dds
    textures/ui/misc_over.dds
    textures/ui/misc_pressed.dds
    textures/ui/mysticism_idle.dds
    textures/ui/mysticism_over.dds
    textures/ui/mysticism_pressed.dds
    textures/ui/out2_idle.dds
    textures/ui/out2_over.dds
    textures/ui/out2_pressed.dds
    textures/ui/potion_idle.dds
    textures/ui/potion_over.dds
    textures/ui/potion_pressed.dds
    textures/ui/power_idle.dds
    textures/ui/power_over.dds
    textures/ui/power_pressed.dds
    textures/ui/restoration_idle.dds
    textures/ui/restoration_over.dds
    textures/ui/restoration_pressed.dds
    textures/ui/selected.dds
    textures/ui/spellall_idle.dds
    textures/ui/spellall_over.dds
    textures/ui/spellall_pressed.dds
    textures/ui/spellitem_idle.dds
    textures/ui/spellitem_over.dds
    textures/ui/spellitem_pressed.dds
    textures/ui/stolen.dds
    textures/ui/toggle_idle.dds
    textures/ui/toggle_over.dds
    textures/ui/toggle_pressed.dds
    textures/ui/tool_idle.dds
    textures/ui/tool_over.dds
    textures/ui/tool_pressed.dds
    textures/ui/weapon_idle.dds
    textures/ui/weapon_over.dds
    textures/ui/weapon_pressed.dds
    textures/ui/gradients/fade_up.dds

    sound/fx/inventory hover.mp3
)

copy_all_resource_files(${CMAKE_CURRENT_SOURCE_DIR} ${OPENMW_MYGUI_FILES_ROOT} ${DDIRRELATIVE} "${VFS_FILES}")
