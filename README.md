# ArenaMP

ArenaMP is a TES3MP/OpenMW 0.47 based multiplayer engine distribution that combines the complete EncoreMP 0.92 gameplay overhaul with ArenaMP rendering work: enhanced water, compatibility PBR lighting, occlusion/shadow optimisations, and runtime graphics controls.

## Main additions

- Complete EncoreMP gameplay and multiplayer codebase.
- Enhanced water and PBR-compatible object/terrain lighting.
- Authoritative interior and inventory-preview guards for caustics.
- Occlusion-culling and shadow sampling optimisations.
- MyGUI GUI-scale control with immediate application.
- Runtime controls for caustics intensity, underwater tint, wave strength, reflection quality, texture quality, and surface roughness.

## Compatibility

ArenaMP remains based on TES3MP 0.8.1 and OpenMW 0.47. EncoreMP companion ESP files keep their original filenames so existing server configurations and mod lists do not break. Full original EncoreMP documentation is preserved in `ENCOREMP_UPSTREAM_NOTES.md`.

## Important build note

The source archive requires all original TES3MP submodules, including CrabNet/RakNet, before CMake can complete configuration.
