#include "spawnitems.h"

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"

//Items spawned by func_breakable and item_random
SpawnItem gSpawnItems[] = 
{
	{0, 0},			// 0
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
	{"weapon_crowbar", 1.5},	// 22
	{"weapon_pipewrench", 1.5},	// 23
	{"weapon_sniperrifle", 4},	// 24
	{"ammo_762", 2.5},		// 25
	{"weapon_knife", 1.5},		// 26
	{"weapon_m249",4},		// 27
	{"weapon_penguin",3.5},	// 28
	{"ammo_556", 3.5},		// 29
	{"weapon_sporelauncher", 4},	// 30
	{"weapon_displacer", 4},		// 31
	{"ammo_9mmbox",3.5},	// 32
	{0,0},		// 33
	{0,0},		// 34
	{"weapon_eagle",3},		// 35
	{"weapon_grapple",3},		// 36
	{"weapon_medkit",2},		// 37
	{"item_suit",1},		// 38
};

int ChooseRandomSpawnItem(const int items[], int itemCount, const float* pointsLeft, const float* playerNeeds)
{
	int i;
	float probabilities[ARRAYSIZE(gSpawnItems)];
	for (i=0; i<ARRAYSIZE(probabilities); ++i) {
		probabilities[i] = 0;
	}
	float probSum = 0;
	for (i=0; i<itemCount; ++i) {
		int itemType = items[i];
		if (!pointsLeft || gSpawnItems[i].value <= *pointsLeft) {
			probabilities[itemType] += 1;
			if (playerNeeds) {
				probabilities[itemType] += playerNeeds[itemType];
			}
			probSum += probabilities[itemType];
		}
	}
	
	float random = RANDOM_FLOAT(0, probSum);
	float currentProb = 0;
	for (i=0; i<ARRAYSIZE(gSpawnItems); ++i) {
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

// Ugly magic numbers here are indices to gSpawnItems array
static int GetSpawnItemIndexByWeaponId(int IId) {
	switch(IId) {
	case WEAPON_GLOCK:
		return 4;
	case WEAPON_PYTHON:
	case WEAPON_EAGLE:
		return 13;
	case WEAPON_MP5:
		return 6;
	case WEAPON_SHOTGUN:
		return 9;
	case WEAPON_CROSSBOW:
		return 11;
	case WEAPON_RPG:
		return 15;
	case WEAPON_GAUSS:
	case WEAPON_EGON:
		return 16;
	}
	return 0;
}

void EvaluatePlayersNeeds(float* playerNeeds)
{	
	int i;
	for (i=0; i<ARRAYSIZE(gSpawnItems); ++i) {
		playerNeeds[i] = 0;
	}
	
	int playerCount = 0;
	for(i = 1; i <= gpGlobals->maxClients; i++ ) {
		CBasePlayer* player = (CBasePlayer*)UTIL_PlayerByIndex(i);
		if (player && player->IsPlayer() && player->IsAlive()) {
			playerCount++;
			
			// player need healkits
			if (player->pev->health < 25) {
				playerNeeds[2] += 2;
			} else if (player->pev->health < 50) {
				playerNeeds[2] += 1;
			}
			
			// player is in good health and need armor
			if (player->pev->health > 90 || player->pev->armorvalue < 20) {
				playerNeeds[1] += 1;
			}
			
			for(int j = 0; j < MAX_WEAPONS; j++ )
			{
				CBasePlayerWeapon *pPlayerItem = player->m_rgpPlayerWeapons[j];
				if( pPlayerItem )
				{
					int ammoAmount = player->AmmoInventory(player->GetAmmoIndex(pPlayerItem->pszAmmo1()));
					if (ammoAmount >= 0) {
						int spawnItemIndex = GetSpawnItemIndexByWeaponId(pPlayerItem->m_iId);
						if (spawnItemIndex) {
							int maxAmmo = pPlayerItem->iMaxAmmo1();
							if (ammoAmount <= maxAmmo / 4) {
								playerNeeds[spawnItemIndex] += 2;
							} else if (ammoAmount <= maxAmmo / 2) {
								playerNeeds[spawnItemIndex] += 1;
							}
						}
					}
				}
			}
		}
	}
	
	if (playerCount) {
		for (i=0; i<ARRAYSIZE(gSpawnItems); ++i) {
			playerNeeds[i] /= playerCount;
		}
	}
}
