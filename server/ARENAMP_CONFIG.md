# ArenaMP server mechanics

The bundled CoreScripts are the authoritative server core for the Windows,
GNU/Linux and macOS builds. The launcher edits `scripts/config.lua`; the server
then sends the mapped OpenMW `[Game]` settings to every connected player.

## NPC combat and pursuit

- `arenaTacticalCombat` enables ArenaMP tactical combat behaviour.
- `arenaCombatWeaponSheatheDelay` controls the post-combat sheathe delay.
- `arenaCombatPursuitThroughDoors` enables pursuit through teleport doors.
- `arenaCombatPursuitGuaranteedDistance` is the distance where door pursuit is guaranteed.
- `arenaCombatPursuitDoorMaxDistance` is the maximum distance considered for door pursuit.
- `arenaCombatPursuitMinimumChance` is the pursuit chance at the maximum distance.
- `arenaCombatPursuitMaxActors` limits the number of NPCs crossing one door transition.
- `arenaCombatPursuitMaxDistance` is the same-cell pursuit leash; `0` means unlimited.

## Other ArenaMP mechanics

The same section controls follower aggression, collision avoidance, giving way,
following over water, actor processing range, looting during death animations,
weapon/shield sheathing, graphic herbalism, long-blade Agility scaling,
two-handed accuracy, staff accuracy, skill-book level limits, constant-effect
difficulty and the global XP multiplier.

Values are clamped to launcher-supported ranges when the server starts. The
server-side values override corresponding local client settings while connected.
