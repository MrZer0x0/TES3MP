if (NOT DEFINED OPENMW_MYGUI_FILES_ROOT)
    return()
endif()

# Copy resource files into the build directory
set(SDIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DDIRRELATIVE resources/vfs/textures)

set(TEXTURE_FILES
    textures/crosshair.dds
    textures/ownership.dds
    textures/sneak.dds
    textures/omw_menu_scroll_down.dds
    textures/omw_menu_scroll_up.dds
    textures/omw_menu_scroll_left.dds
    textures/omw_menu_scroll_right.dds
    textures/omw_menu_scroll_center_h.dds
    textures/omw_menu_scroll_center_v.dds
)

copy_all_resource_files(${CMAKE_CURRENT_SOURCE_DIR} ${OPENMW_MYGUI_FILES_ROOT} ${DDIRRELATIVE} "${TEXTURE_FILES}")


# Built-in additional character animations. The engine mounts resources/vfs as
# its first data directory, so these KF files are available without a separate mod.
file(GLOB_RECURSE DYNAMIC_ACTOR_ANIMATION_FILES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    animations/*.kf
    animations/*.yaml
    animations/*.txt
)
foreach (f ${DYNAMIC_ACTOR_ANIMATION_FILES})
    copy_resource_file("${CMAKE_CURRENT_SOURCE_DIR}/${f}" "${OPENMW_MYGUI_FILES_ROOT}" "resources/vfs/${f}")
endforeach()
