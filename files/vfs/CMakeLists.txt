if (NOT DEFINED OPENMW_MYGUI_FILES_ROOT)
    return()
endif()

# Copy resource files into the build directory
set(SDIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DDIRRELATIVE resources/vfs/textures)

set(TEXTURE_FILES
    textures/omw_menu_scroll_down.dds
    textures/omw_menu_scroll_up.dds
    textures/omw_menu_scroll_left.dds
    textures/omw_menu_scroll_right.dds
    textures/omw_menu_scroll_center_h.dds
    textures/omw_menu_scroll_center_v.dds
	textures/crosshair.dds
	textures/sneak.dds
    textures/ui/breathbarfull.dds
    textures/ui/compass.dds
    textures/ui/compass_hidden.dds
    textures/ui/compass_needle.dds
    textures/ui/compass_ring.dds
    textures/ui/crosshair.dds
    textures/ui/enemybar.dds
    textures/ui/healthbar.dds
    textures/ui/magickabar.dds
    textures/ui/sneak.dds
    textures/ui/staminabar.dds
)

copy_all_resource_files(${CMAKE_CURRENT_SOURCE_DIR} ${OPENMW_MYGUI_FILES_ROOT} ${DDIRRELATIVE} "${TEXTURE_FILES}")
