#pragma once
#ifndef BULLET_TYPES_H
#define BULLET_TYPES_H

// bullet types
typedef	enum
{
	BULLET_NONE = 0,
	BULLET_PLAYER_9MM, // glock
	BULLET_PLAYER_MP5, // mp5
	BULLET_PLAYER_357, // python
	BULLET_PLAYER_BUCKSHOT, // shotgun
	BULLET_PLAYER_CROWBAR, // crowbar swipe
	BULLET_PLAYER_556, // m249
	BULLET_PLAYER_762, // sniperrifle
	BULLET_PLAYER_EAGLE, // desert eagle

	BULLET_MONSTER_9MM,
	BULLET_MONSTER_MP5,
	BULLET_MONSTER_12MM,
	BULLET_MONSTER_357,
	BULLET_MONSTER_556,
	BULLET_MONSTER_762
} Bullet;

#endif
