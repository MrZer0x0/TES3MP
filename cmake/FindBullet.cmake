# Delegate to CMake's built-in FindBullet.
# This overrides the openmw-morro_ui custom finder which broke TES3MP builds.
# TES3MP uses -DBULLET_ROOT to locate Bullet; CMake's built-in handles this correctly.
include(${CMAKE_ROOT}/Modules/FindBullet.cmake)
