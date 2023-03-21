#pragma once
#ifndef MOD_FEATURES_H
#define MOD_FEATURES_H

/*
 * 0 to disable feature
 * 1 to enable feature
 */

// Enable opfor specific changes, like more weapon slots, green hud, etc.
// Follow the symbol to see what it actually changes
#define FEATURE_OPFOR_SPECIFIC 0

#define FEATURE_OPFOR_WEAPON_SLOTS (0 || FEATURE_OPFOR_SPECIFIC)
#define FEATURE_OPFOR_SCIENTIST_BODIES (0 || FEATURE_OPFOR_SPECIFIC)

// Fast way to enable/disable entities that require extra content to be added in mod
#define FEATURE_OPFOR_WEAPONS 1
#define FEATURE_OPFOR_MONSTERS 1
#define FEATURE_BSHIFT_MONSTERS 1
#define FEATURE_SVENCOOP_MONSTERS 1

// New monsters
#define FEATURE_ZOMBIE_BARNEY (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_ZOMBIE_SOLDIER (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_GONOME (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_PITDRONE (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_SHOCKTROOPER (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_VOLTIFORE (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_MASSN (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_BLACK_OSPREY (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_BLACK_APACHE (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_OTIS (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_CLEANSUIT_SCIENTIST (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_BABYGARG (0 || FEATURE_SVENCOOP_MONSTERS)
#define FEATURE_OPFOR_GRUNT (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_PITWORM (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_GENEWORM (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_DRILLSERGEANT (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_RECRUIT (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_ROBOGRUNT (0 || FEATURE_SVENCOOP_MONSTERS)
#define FEATURE_HWGRUNT (0 || FEATURE_SVENCOOP_MONSTERS)
#define FEATURE_ROSENBERG (0 || FEATURE_BSHIFT_MONSTERS)
#define FEATURE_GUS 0
#define FEATURE_KELLER 1
#define FEATURE_BARNIEL 1
#define FEATURE_KATE 1

#define FEATURE_ISLAVE_DEAD (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_HOUNDEYE_DEAD (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_OPFOR_DEADHAZ (0 || FEATURE_OPFOR_SPECIFIC)
#define FEATURE_SKELETON (0 || FEATURE_OPFOR_MONSTERS)

// enable reverse relationship models, like barnabus
#define FEATURE_REVERSE_RELATIONSHIP_MODELS 0

// monsters who carry hand grenades will drop one hand grenade upon death
#define FEATURE_MONSTERS_DROP_HANDGRENADES 0

#define FEATURE_DYING_MONSTERS_DONT_COLLIDE_WITH_PLAYER 0

// New weapons
#define FEATURE_PIPEWRENCH (0 || FEATURE_OPFOR_WEAPONS)
#define FEATURE_KNIFE (0 || FEATURE_OPFOR_WEAPONS)
#define FEATURE_GRAPPLE (0 || FEATURE_OPFOR_WEAPONS)
#define FEATURE_DESERT_EAGLE (0 || FEATURE_OPFOR_WEAPONS)
#define FEATURE_PENGUIN (0 || FEATURE_OPFOR_WEAPONS)
#define FEATURE_M249 (0 || FEATURE_OPFOR_WEAPONS)
#define FEATURE_SNIPERRIFLE (0 || FEATURE_OPFOR_WEAPONS)
#define FEATURE_DISPLACER (0 || FEATURE_OPFOR_WEAPONS)
#define FEATURE_SHOCKRIFLE (0 || FEATURE_OPFOR_WEAPONS)
#define FEATURE_SPORELAUNCHER (0 || FEATURE_OPFOR_WEAPONS)
#define FEATURE_MEDKIT 1
#define FEATURE_UZI 1

// Weapon features
#define FEATURE_PREDICTABLE_LASER_SPOT 0

// Dependent features
#define FEATURE_SHOCKBEAM (FEATURE_SHOCKTROOPER || FEATURE_SHOCKRIFLE)
#define FEATURE_SPOREGRENADE (FEATURE_SHOCKTROOPER || FEATURE_SPORELAUNCHER)

// Brush entities features
// Open func_door_rotating in the direction of player's movement, not facing
#define FEATURE_OPEN_ROTATING_DOOR_IN_MOVE_DIRECTION 0

// Misc features
#define FEATURE_OPFOR_NIGHTVISION (1 || FEATURE_OPFOR_SPECIFIC)
#define FEATURE_CS_NIGHTVISION (1 || FEATURE_OPFOR_SPECIFIC)
#define FEATURE_MOVE_MODE 1
#define FEATURE_CLIENTSIDE_HUDSOUND 0
#define FEATURE_GEIGER_SOUNDS_FIX 1

#define FEATURE_NIGHTVISION (FEATURE_OPFOR_NIGHTVISION || FEATURE_CS_NIGHTVISION)
#define FEATURE_NIGHTVISION_STYLES (FEATURE_OPFOR_NIGHTVISION + FEATURE_CS_NIGHTVISION > 1)

#define FEATURE_ROPE (1 || FEATURE_OPFOR_SPECIFIC)

// Counter Strike effects
#define FEATURE_WALLPUFF_CS 0

// Experimental Cvars
#define FEATURE_EXPERIMENTAL_CVARS 1

#define FEATURE_USE_THROUGH_WALLS_CVAR (0 || FEATURE_EXPERIMENTAL_CVARS)
#define FEATURE_NPC_NEAREST_CVAR (0 || FEATURE_EXPERIMENTAL_CVARS)
#define FEATURE_GRENADE_JUMP_CVAR (0 || FEATURE_EXPERIMENTAL_CVARS)
#define FEATURE_NPC_FIX_MELEE_DISTANCE_CVAR (0 || FEATURE_EXPERIMENTAL_CVARS)
#define FEATURE_NPC_FOLLOW_OUT_OF_PVS_CVAR (0 || FEATURE_EXPERIMENTAL_CVARS)

#endif
