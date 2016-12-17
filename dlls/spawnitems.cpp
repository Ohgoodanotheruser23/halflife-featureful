#include "spawnitems.h"
#include <cstddef>
#include "extdll.h"
#include "util.h"
#include "cbase.h"


//Items spawned by func_breakable and item_random
SpawnItem gSpawnItems[] = 
{
	{NULL, 0},			// 0
	{"item_battery", 1.5},		// 1
	{"item_healthkit", 1},	// 2
	{"weapon_9mmhandgun",2},	// 3
	{"ammo_9mmclip",1.5},		// 4
	{"weapon_9mmAR",3},		// 5
	{"ammo_9mmAR", 2},		// 6
	{"ammo_ARgrenades",3.5},	// 7
	{"weapon_shotgun",3},	// 8
	{"ammo_buckshot",2.5},	// 9
	{"weapon_crossbow",4},	// 10
	{"ammo_crossbow",3},	// 11
	{"weapon_357",3},		// 12
	{"ammo_357",2.5},		// 13
	{"weapon_rpg",4},		// 14
	{"ammo_rpgclip",2.5},		// 15
	{"ammo_gaussclip",3.5},	// 16
	{"weapon_handgrenade",2},	// 17
	{"weapon_tripmine",2},	// 18
	{"weapon_satchel",2.5},	// 19
	{"weapon_snark",2.5},		// 20
	{"weapon_hornetgun",4},	// 21
};

int ChooseRandomSpawnItem(const int items[], int itemCount, const float* pointsLeft)
{
	float probabilities[ARRAYSIZE(gSpawnItems)];
	for (int i=0; i<ARRAYSIZE(probabilities); ++i) {
		probabilities[i] = 0;
	}
	float probSum = 0;
	for (int i=0; i<itemCount; ++i) {
		int itemType = items[i];
		if (!pointsLeft || gSpawnItems[i].value <= *pointsLeft) {
			probabilities[itemType] += 1;
			probSum += 1;
		}
	}
	
	float random = RANDOM_FLOAT(0, probSum);
	float currentProb = 0;
	for (int i=0; i<ARRAYSIZE(gSpawnItems); ++i) {
		if (random >= currentProb && random <= (currentProb + probabilities[i])) {
			return i;
		}
		currentProb += probabilities[i];
	}
	return 0;
}

int CountSpawnItems(const int items[], int maxCount)
{
	for (int i=maxCount-1; i>=0; --i) {
		if (items[i]) {
			return i+1;
		}
	}
	return 0;
}
