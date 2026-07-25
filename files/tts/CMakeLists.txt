# Copy the optional libpiper runtime, espeak data and voice models into the
# normal OpenMW resources tree. GLOB is intentional here: CI downloads voice
# assets before configure, while source archives may contain only README files.
if(APPLE AND NOT DEFINED OPENMW_MYGUI_FILES_ROOT)
    return()
endif()

if(DEFINED OPENMW_MYGUI_FILES_ROOT)
    set(ARENAMP_TTS_DESTINATION_ROOT "${OPENMW_MYGUI_FILES_ROOT}")
else()
    set(ARENAMP_TTS_DESTINATION_ROOT "${OpenMW_BINARY_DIR}")
endif()

file(GLOB_RECURSE ARENAMP_TTS_RESOURCE_FILES
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/*")
list(REMOVE_ITEM ARENAMP_TTS_RESOURCE_FILES "CMakeLists.txt")

foreach(TTS_FILE ${ARENAMP_TTS_RESOURCE_FILES})
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${TTS_FILE}")
        continue()
    endif()

    # Visual Studio and other multi-config generators install from
    # <binary>/<CONFIG>. Use OpenMW's helper so TTS resources are copied into
    # Release/resources/tts instead of the non-installed binary root.
    if(APPLE)
        get_filename_component(TTS_FILE_DIRECTORY "${TTS_FILE}" DIRECTORY)
        file(MAKE_DIRECTORY
            "${ARENAMP_TTS_DESTINATION_ROOT}/resources/tts/${TTS_FILE_DIRECTORY}")
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/${TTS_FILE}"
            "${ARENAMP_TTS_DESTINATION_ROOT}/resources/tts/${TTS_FILE}"
            COPYONLY)
    else()
        copy_resource_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/${TTS_FILE}"
            "${ARENAMP_TTS_DESTINATION_ROOT}"
            "resources/tts/${TTS_FILE}")
    endif()
endforeach()
