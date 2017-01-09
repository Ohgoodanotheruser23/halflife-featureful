/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/*

===== items.cpp ========================================================

  functions governing the selection/use of weapons for players

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "game.h"
#include "skill.h"
#include "items.h"
#include "gamerules.h"
#include "spawnitems.h"

extern int gmsgItemPickup;

class CWorldItem : public CBaseEntity
{
public:
	void KeyValue( KeyValueData *pkvd ); 
	void Spawn( void );
	int m_iType;
};

LINK_ENTITY_TO_CLASS( world_items, CWorldItem )

void CWorldItem::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "type" ) )
	{
		m_iType = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CWorldItem::Spawn( void )
{
	CBaseEntity *pEntity = NULL;

	switch( m_iType ) 
	{
	case 44: // ITEM_BATTERY:
		pEntity = CBaseEntity::Create( "item_battery", pev->origin, pev->angles );
		break;
	case 42: // ITEM_ANTIDOTE:
		pEntity = CBaseEntity::Create( "item_antidote", pev->origin, pev->angles );
		break;
	case 43: // ITEM_SECURITY:
		pEntity = CBaseEntity::Create( "item_security", pev->origin, pev->angles );
		break;
	case 45: // ITEM_SUIT:
		pEntity = CBaseEntity::Create( "item_suit", pev->origin, pev->angles );
		break;
	}

	if( !pEntity )
	{
		ALERT( at_console, "unable to create world_item %d\n", m_iType );
	}
	else
	{
		pEntity->pev->target = pev->target;
		pEntity->pev->targetname = pev->targetname;
		pEntity->pev->spawnflags = pev->spawnflags;
	}

	REMOVE_ENTITY( edict() );
}

#define ITEM_RANDOM_MAX_COUNT 9

class CItemRandom : public CBaseEntity
{
public:
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	void KeyValue( KeyValueData *pkvd ); 
	void Spawn( void );
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void SpawnItem(int itemType);
	void SpawnItem();
	
	int ItemCount() const;
	
	int m_items[ITEM_RANDOM_MAX_COUNT];
	
	static TYPEDESCRIPTION m_SaveData[];
};

LINK_ENTITY_TO_CLASS( item_random, CItemRandom )

void CItemRandom::KeyValue( KeyValueData *pkvd )
{
	if ( strncmp(pkvd->szKeyName, "item", 4) == 0 && isdigit(pkvd->szKeyName[4])) {
		pkvd->fHandled = FALSE;
		
		int itemIndex = atoi(pkvd->szKeyName + 4);
		if (itemIndex > 0 && itemIndex <= ITEM_RANDOM_MAX_COUNT) {
			int itemType = atoi(pkvd->szValue);
			if (itemType >= ARRAYSIZE(gSpawnItems)) {
				itemType = 0;
			}
			
			m_items[itemIndex-1] = itemType;
			pkvd->fHandled = TRUE;
		}
		if (pkvd->fHandled == FALSE) {
			CBaseEntity::KeyValue( pkvd );
			return;
		}
	} else {
		CBaseEntity::KeyValue( pkvd );
	}
}

int CItemRandom::ItemCount() const
{
	return CountSpawnItems(m_items, ITEM_RANDOM_MAX_COUNT);
}

void CItemRandom::Spawn( void )
{
	if (FStringNull(pev->targetname)) {
		SpawnItem();
	}
}

TYPEDESCRIPTION CItemRandom::m_SaveData[] =
{
	DEFINE_ARRAY( CItemRandom, m_items, FIELD_INTEGER, ITEM_RANDOM_MAX_COUNT ),
};

IMPLEMENT_SAVERESTORE( CItemRandom, CBaseEntity )

void CItemRandom::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	SpawnItem();
}

void CItemRandom::SpawnItem()
{
	if (ItemCount()) {
		int chosenItemIndex = RANDOM_LONG(0, ItemCount()-1);
		int itemType = m_items[chosenItemIndex];		
		SpawnItem(itemType);
	} else {
		SpawnItem(0);
	}
}

void CItemRandom::SpawnItem(int itemType)
{
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = pev->ltime + 0.1;
	
	if (itemType && itemType < ARRAYSIZE(gSpawnItems)) {
		UTIL_PrecacheOther(gSpawnItems[itemType].name);
		CBaseEntity *pEntity = CBaseEntity::Create( (char*)gSpawnItems[itemType].name, pev->origin, pev->angles, edict() );
		if( pEntity )
		{
			pEntity->pev->target = pev->target;
			pEntity->pev->spawnflags = pev->spawnflags;
		}
	}
}

#define SF_INFOITEMRANDOM_STARTSPAWNED 1
#define SF_INFOITEMRANDOM_SUPPORTPLAYERS 4096

class CInfoItemRandom : public CItemRandom
{
public:
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	void KeyValue( KeyValueData *pkvd );
	void Spawn();
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void SpawnItems();
	void EXPORT SpawnThink();
	float m_maxPoints;
	
	static TYPEDESCRIPTION m_SaveData[];
};

LINK_ENTITY_TO_CLASS( info_item_random, CInfoItemRandom )

void CInfoItemRandom::KeyValue(KeyValueData *pkvd)
{
	if ( FStrEq( pkvd->szKeyName, "maxpoints" ) ) {
		m_maxPoints = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	} else {
		CItemRandom::KeyValue(pkvd);
	}
}

void CInfoItemRandom::Spawn()
{
	if (pev->spawnflags & SF_INFOITEMRANDOM_STARTSPAWNED) {
		SetThink(&CInfoItemRandom::SpawnThink);
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CInfoItemRandom::SpawnThink()
{
	SpawnItems();
	pev->nextthink = -1;
}

TYPEDESCRIPTION CInfoItemRandom::m_SaveData[] =
{
	DEFINE_FIELD( CInfoItemRandom, m_maxPoints, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CInfoItemRandom, CItemRandom )

void CInfoItemRandom::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	SpawnItems();
}

static void ItemRandomShuffle(CItemRandom** first, CItemRandom** last)
{
	int n = (last-first);
	for (int i=n-1; i>0; --i) {
		CItemRandom* temp = first[i];
		int randomIndex = RANDOM_LONG(0,i);
		first[i] = first[randomIndex];
		first[randomIndex] = temp;
	}
}

void CInfoItemRandom::SpawnItems()
{
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = pev->ltime + 0.1;
	
	if (FStringNull(pev->target)) {
		return;
	}
	
	float pointsLeft = m_maxPoints;
	const float* pPointsLeft = m_maxPoints > 0 ? &pointsLeft : NULL;
	const int myItemCount = ItemCount();
	
	CBaseEntity* pEntity = NULL;
	CItemRandom* itemRandomVec[100];
	int itemRandomCount = 0;
	while((pEntity = UTIL_FindEntityByTargetname(pEntity, STRING(pev->target))) && itemRandomCount < ARRAYSIZE(itemRandomVec)) {
		if (FClassnameIs(pEntity->pev, "item_random")) {
			CItemRandom* itemRandom = (CItemRandom*)pEntity;
			itemRandomVec[itemRandomCount++] = itemRandom;
		}
	}
	
	// So the spawn order was not defined by order of entities in map
	ItemRandomShuffle(itemRandomVec, itemRandomVec + itemRandomCount);
	
	float playerNeeds[ARRAYSIZE(gSpawnItems)];
	float* pPlayerNeeds = NULL;
	if (pev->spawnflags & SF_INFOITEMRANDOM_SUPPORTPLAYERS) {
		EvaluatePlayersNeeds(playerNeeds);
		pPlayerNeeds = playerNeeds;
	}
	
	for (int i = 0; i < itemRandomCount; ++i) {
		CItemRandom* itemRandom = itemRandomVec[i];
		int itemType = 0;
		if (itemRandom->ItemCount()) {
			itemType = ChooseRandomSpawnItem(itemRandom->m_items, itemRandom->ItemCount(), pPointsLeft, pPlayerNeeds);
		} else if (myItemCount) {
			itemType = ChooseRandomSpawnItem(m_items, myItemCount, pPointsLeft, pPlayerNeeds);
		}
		itemRandom->SpawnItem(itemType);
		if (pPointsLeft) {
			pointsLeft -= gSpawnItems[itemType].value;
		}
	}
}

void CItem::Spawn( void )
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 16 ) );
	SetTouch( &CItem::ItemTouch );

	if( DROP_TO_FLOOR(ENT( pev ) ) == 0 )
	{
		ALERT(at_error, "Item %s fell out of level at %f,%f,%f\n", STRING( pev->classname ), pev->origin.x, pev->origin.y, pev->origin.z);
		UTIL_Remove( this );
		return;
	}
}

extern int gEvilImpulse101;

void CItem::ItemTouch( CBaseEntity *pOther )
{
	if (!use_to_take.value) {
		TouchOrUse(pOther);
	}
}

int CItem::ObjectCaps()
{
	if (use_to_take.value && !(pev->effects & EF_NODRAW)) {
		return CBaseEntity::ObjectCaps() | FCAP_IMPULSE_USE;
	} else {
		return CBaseEntity::ObjectCaps();
	}
}

void CItem::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (use_to_take.value && !(pev->effects & EF_NODRAW)) {
		TouchOrUse(pActivator);
	}
}

void CItem::TouchOrUse(CBaseEntity *pOther)
{
	// if it's not a player, ignore
	if( !pOther->IsPlayer() )
	{
		return;
	}

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	// ok, a player is touching this item, but can he have it?
	if( !g_pGameRules->CanHaveItem( pPlayer, this ) )
	{
		// no? Ignore the touch.
		return;
	}

	if( MyTouch( pPlayer ) )
	{
		SUB_UseTargets( pOther, USE_TOGGLE, 0 );
		SetTouch( NULL );
		
		// player grabbed the item. 
		g_pGameRules->PlayerGotItem( pPlayer, this );
		if( g_pGameRules->ItemShouldRespawn( this ) == GR_ITEM_RESPAWN_YES )
		{
			Respawn(); 
		}
		else
		{
			UTIL_Remove( this );
		}
	}
	else if( gEvilImpulse101 )
	{
		UTIL_Remove( this );
	}
}

CBaseEntity* CItem::Respawn( void )
{
	SetTouch( NULL );
	pev->effects |= EF_NODRAW;

	UTIL_SetOrigin( pev, g_pGameRules->VecItemRespawnSpot( this ) );// blip to whereever you should respawn.

	SetThink( &CItem::Materialize );
	pev->nextthink = g_pGameRules->FlItemRespawnTime( this ); 
	return this;
}

void CItem::Materialize( void )
{
	if( pev->effects & EF_NODRAW )
	{
		// changing from invisible state to visible.
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150 );
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	SetTouch( &CItem::ItemTouch );
}

#define SF_SUIT_SHORTLOGON		0x0001

class CItemSuit : public CItem
{
	void Spawn( void )
	{ 
		Precache();
		SET_MODEL( ENT( pev ), "models/w_suit.mdl" );
		CItem::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_suit.mdl" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if( pPlayer->pev->weapons & ( 1<<WEAPON_SUIT ) )
			return FALSE;

		if( pev->spawnflags & SF_SUIT_SHORTLOGON )
			EMIT_SOUND_SUIT( pPlayer->edict(), "!HEV_A0" );		// short version of suit logon,
		else
			EMIT_SOUND_SUIT( pPlayer->edict(), "!HEV_AAx" );	// long version of suit logon

		pPlayer->pev->weapons |= ( 1 << WEAPON_SUIT );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( item_suit, CItemSuit )

class CItemBattery : public CItem
{
	void Spawn( void )
	{ 
		Precache();
		SET_MODEL( ENT( pev ), "models/w_battery.mdl" );
		CItem::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_battery.mdl" );
		PRECACHE_SOUND( "items/gunpickup2.wav" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if( pPlayer->pev->deadflag != DEAD_NO )
		{
			return FALSE;
		}

		if( ( pPlayer->pev->armorvalue < MAX_NORMAL_BATTERY ) &&
			( pPlayer->pev->weapons & ( 1 << WEAPON_SUIT ) ) )
		{
			int pct;
			char szcharge[64];

			pPlayer->pev->armorvalue += gSkillData.batteryCapacity;
			pPlayer->pev->armorvalue = min( pPlayer->pev->armorvalue, MAX_NORMAL_BATTERY );

			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );

			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
				WRITE_STRING( STRING( pev->classname ) );
			MESSAGE_END();

			// Suit reports new power level
			// For some reason this wasn't working in release build -- round it.
			pct = (int)( (float)( pPlayer->pev->armorvalue * 100.0 ) * ( 1.0 / MAX_NORMAL_BATTERY ) + 0.5 );
			pct = ( pct / 5 );
			if( pct > 0 )
				pct--;

			sprintf( szcharge,"!HEV_%1dP", pct );

			//EMIT_SOUND_SUIT( ENT( pev ), szcharge );
			pPlayer->SetSuitUpdate( szcharge, FALSE, SUIT_NEXT_IN_30SEC);
			return TRUE;
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( item_battery, CItemBattery )

class CItemAntidote : public CItem
{
	void Spawn( void )
	{ 
		Precache();
		SET_MODEL( ENT( pev ), "models/w_antidote.mdl" );
		CItem::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_antidote.mdl" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		pPlayer->SetSuitUpdate( "!HEV_DET4", FALSE, SUIT_NEXT_IN_1MIN );

		pPlayer->m_rgItems[ITEM_ANTIDOTE] += 1;
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( item_antidote, CItemAntidote )

class CItemSecurity : public CItem
{
	void Spawn( void )
	{ 
		Precache();
		SET_MODEL( ENT( pev ), "models/w_security.mdl" );
		CItem::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_security.mdl" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		pPlayer->m_rgItems[ITEM_SECURITY] += 1;
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( item_security, CItemSecurity )

class CItemLongJump : public CItem
{
	void Spawn( void )
	{ 
		Precache();
		SET_MODEL( ENT( pev ), "models/w_longjump.mdl" );
		CItem::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_longjump.mdl" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if( pPlayer->m_fLongJump )
		{
			return FALSE;
		}

		if( ( pPlayer->pev->weapons & ( 1 << WEAPON_SUIT ) ) )
		{
			pPlayer->m_fLongJump = TRUE;// player now has longjump module

			g_engfuncs.pfnSetPhysicsKeyValue( pPlayer->edict(), "slj", "1" );

			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
				WRITE_STRING( STRING( pev->classname ) );
			MESSAGE_END();

			EMIT_SOUND_SUIT( pPlayer->edict(), "!HEV_A1" );	// Play the longjump sound UNDONE: Kelly? correct sound?
			return TRUE;		
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( item_longjump, CItemLongJump )
