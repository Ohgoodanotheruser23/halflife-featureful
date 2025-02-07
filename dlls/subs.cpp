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

===== subs.cpp ========================================================

  frequently used global functions

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "nodes.h"
#include "doors.h"

extern BOOL FEntIsVisible( entvars_t *pev, entvars_t *pevTarget );

extern DLL_GLOBAL int g_iSkillLevel;

// Landmark class
void CPointEntity::Spawn( void )
{
	pev->solid = SOLID_NOT;
	//UTIL_SetSize( pev, g_vecZero, g_vecZero );
}

class CNullEntity : public CBaseEntity
{
public:
	void Spawn( void );
};

// Null Entity, remove on startup
void CNullEntity::Spawn( void )
{
	REMOVE_ENTITY( ENT( pev ) );
}

LINK_ENTITY_TO_CLASS( info_null, CNullEntity )

class CSpawnPoint : public CPointEntity
{
public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

#define SF_SPAWNPOINT_OFF 2

void CSpawnPoint::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (ShouldToggle(useType, !FBitSet(pev->spawnflags, SF_SPAWNPOINT_OFF)))
	{
		if (FBitSet(pev->spawnflags, SF_SPAWNPOINT_OFF))
			ClearBits(pev->spawnflags, SF_SPAWNPOINT_OFF);
		else
			SetBits(pev->spawnflags, SF_SPAWNPOINT_OFF);
	}
}

class CBaseDMStart : public CSpawnPoint
{
public:
	void KeyValue( KeyValueData *pkvd );
	bool IsTriggered( CBaseEntity *pEntity );

private:
};

// These are the new entry points to entities. 
LINK_ENTITY_TO_CLASS( info_player_deathmatch, CBaseDMStart )
LINK_ENTITY_TO_CLASS( info_player_start, CSpawnPoint )
LINK_ENTITY_TO_CLASS( info_player_coop, CSpawnPoint )
LINK_ENTITY_TO_CLASS( info_landmark, CPointEntity )

void CBaseDMStart::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "master" ) )
	{
		pev->netname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CSpawnPoint::KeyValue( pkvd );
}

bool CBaseDMStart::IsTriggered( CBaseEntity *pEntity )
{
	bool master = UTIL_IsMasterTriggered( pev->netname, pEntity );

	return master;
}

// This updates global tables that need to know about entities being removed
void CBaseEntity::UpdateOnRemove( void )
{
	int i;

	if( FBitSet( pev->flags, FL_GRAPHED ) )
	{
		// this entity was a LinkEnt in the world node graph, so we must remove it from
		// the graph since we are removing it from the world.
		for( i = 0; i < WorldGraph.m_cLinks; i++ )
		{
			if( WorldGraph.m_pLinkPool[i].m_pLinkEnt == pev )
			{
				// if this link has a link ent which is the same ent that is removing itself, remove it!
				WorldGraph.m_pLinkPool[i].m_pLinkEnt = NULL;
			}
		}
	}

	if( pev->globalname )
		gGlobalState.EntitySetState( pev->globalname, GLOBAL_DEAD );
}

// Convenient way to delay removing oneself
void CBaseEntity::SUB_Remove( void )
{
	UpdateOnRemove();
	if( pev->health > 0 )
	{
		// this situation can screw up monsters who can't tell their entity pointers are invalid.
		pev->health = 0;
		// This message is useless as it's fine to remove entity with positive amount of health.
		//ALERT( at_aiconsole, "SUB_Remove called on entity with health > 0\n" );
	}

	REMOVE_ENTITY( ENT( pev ) );
}

// Convenient way to explicitly do nothing (passed to functions that require a method)
void CBaseEntity::SUB_DoNothing( void )
{
}

// Global Savedata for Delay
TYPEDESCRIPTION	CBaseDelay::m_SaveData[] =
{
	DEFINE_FIELD( CBaseDelay, m_flDelay, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseDelay, m_iszKillTarget, FIELD_STRING ),
	DEFINE_FIELD( CBaseDelay, m_hActivator, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CBaseDelay, CBaseEntity )

void CBaseDelay::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "delay" ) )
	{
		m_flDelay = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "killtarget" ) )
	{
		m_iszKillTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseEntity::KeyValue( pkvd );
	}
}

/*
==============================
SUB_UseTargets

If self.delay is set, a DelayedUse entity will be created that will actually
do the SUB_UseTargets after that many seconds have passed.

Removes all entities with a targetname that match self.killtarget,
and removes them, so some events can remove other triggers.

Search for (string)targetname in all entities that
match (string)self.target and call their .use function (if they have one)

==============================
*/
void CBaseEntity::SUB_UseTargets( CBaseEntity *pActivator, USE_TYPE useType, float value )
{
	//
	// fire targets
	//
	if( !FStringNull( pev->target ) )
	{
		FireTargets( STRING( pev->target ), pActivator, this, useType, value );
	}
}

const char* UseTypeToString(USE_TYPE useType)
{
	switch (useType) {
	case USE_OFF:
		return "USE_OFF";
	case USE_ON:
		return "USE_ON";
	case USE_TOGGLE:
		return "USE_TOGGLE";
	case USE_SET:
		return "USE_SET";
	default:
		return "USE_UNKNOWN";
	}
}

void FireTargets( const char *targetName, CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity* pTarget = NULL;
	if( !targetName || *targetName == '\0' )
		return;

	const char* useTypeString = UseTypeToString(useType);
	ALERT( at_aiconsole, "Firing: (%s)\n", targetName );
	const char* callerClassname = pCaller ? STRING(pCaller->pev->classname) : "";
	const char* activatorClassname = pActivator ? STRING(pActivator->pev->classname) : "";

	for( ; ; )
	{
		pTarget = UTIL_FindEntityByTargetname(pTarget, targetName, pActivator);
		if( !pTarget )
			break;

		if( pTarget && !( pTarget->pev->flags & FL_KILLME ) )	// Don't use dying ents
		{
			ALERT( at_aiconsole, "Found: %s, firing (%s, %s, called by '%s', activated by '%s')\n", STRING( pTarget->pev->classname ), targetName, useTypeString, callerClassname, activatorClassname );
			pTarget->Use( pActivator, pCaller, useType, value );
		}
	}
}

void KillTargets(const char* targetName)
{
	edict_t *pentKillTarget = NULL;

	pentKillTarget = FIND_ENTITY_BY_TARGETNAME( NULL, targetName );
	while( !FNullEnt(pentKillTarget) )
	{
		UTIL_Remove( CBaseEntity::Instance( pentKillTarget ) );

		ALERT( at_aiconsole, "killing %s (%s)\n", STRING( pentKillTarget->v.classname ), targetName );
		pentKillTarget = FIND_ENTITY_BY_TARGETNAME( pentKillTarget, targetName );
	}
}

LINK_ENTITY_TO_CLASS( DelayedUse, CBaseDelay )

void CBaseDelay::DelayedUse(float delay, CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, string_t target, string_t killTarget, float value)
{
	//
	// exit immediatly if we don't have a target or kill target
	//
	if( FStringNull( target ) && !killTarget )
		return;

	//
	// check for a delay
	//
	if( delay != 0 )
	{
		// create a temp object to fire at a later time
		CBaseDelay *pTemp = GetClassPtr( (CBaseDelay *)NULL );
		pTemp->pev->classname = MAKE_STRING( "DelayedUse" );

		pTemp->pev->nextthink = gpGlobals->time + delay;

		pTemp->SetThink( &CBaseDelay::DelayThink );

		// Save the useType
		pTemp->pev->button = (int)useType;
		pTemp->m_iszKillTarget = killTarget;
		pTemp->m_flDelay = 0.0f; // prevent "recursion"
		pTemp->pev->target = target;

		pTemp->m_hActivator = pActivator;

		return;
	}

	//
	// kill the killtargets
	//
	if( killTarget )
	{
		ALERT( at_aiconsole, "KillTarget: %s\n", STRING( killTarget ) );
		KillTargets(STRING( killTarget ));
	}

	//
	// fire targets
	//
	if( !FStringNull( target ) )
	{
		FireTargets( STRING( target ), pActivator, pCaller, useType, value );
	}
}

void CBaseDelay::SUB_UseTargets( CBaseEntity *pActivator, USE_TYPE useType, float value )
{
	DelayedUse( m_flDelay, pActivator, this, useType, pev->target, m_iszKillTarget, value );
}

/*
void CBaseDelay::SUB_UseTargetsEntMethod( void )
{
	SUB_UseTargets( pev );
}
*/

/*
QuakeEd only writes a single float for angles (bad idea), so up and down are
just constant angles.
*/
void SetMovedir( entvars_t *pev )
{
	if( pev->angles == Vector( 0, -1, 0 ) )
	{
		pev->movedir = Vector( 0, 0, 1 );
	}
	else if (pev->angles == Vector( 0, -2, 0 ) )
	{
		pev->movedir = Vector( 0, 0, -1 );
	}
	else
	{
		UTIL_MakeVectors( pev->angles );
		pev->movedir = gpGlobals->v_forward;
	}

	pev->angles = g_vecZero;
}

void CBaseDelay::DelayThink( void )
{
	// The use type is cached (and stashed) in pev->button
	SUB_UseTargets( m_hActivator, (USE_TYPE)pev->button );
	REMOVE_ENTITY( ENT( pev ) );
}

// Global Savedata for Toggle
TYPEDESCRIPTION	CBaseToggle::m_SaveData[] =
{
	DEFINE_FIELD( CBaseToggle, m_toggle_state, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseToggle, m_flActivateFinished, FIELD_TIME ),
	DEFINE_FIELD( CBaseToggle, m_flMoveDistance, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseToggle, m_flWait, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseToggle, m_flLip, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseToggle, m_flTWidth, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseToggle, m_flTLength, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseToggle, m_vecPosition1, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBaseToggle, m_vecPosition2, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBaseToggle, m_vecAngle1, FIELD_VECTOR ),		// UNDONE: Position could go through transition, but also angle?
	DEFINE_FIELD( CBaseToggle, m_vecAngle2, FIELD_VECTOR ),		// UNDONE: Position could go through transition, but also angle?
	DEFINE_FIELD( CBaseToggle, m_flHeight, FIELD_FLOAT ),
	//DEFINE_FIELD( CBaseToggle, m_hActivator, FIELD_EHANDLE ), // now in CBaseDelay
	DEFINE_FIELD( CBaseToggle, m_pfnCallWhenMoveDone, FIELD_FUNCTION ),
	DEFINE_FIELD( CBaseToggle, m_vecFinalDest, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBaseToggle, m_vecFinalAngle, FIELD_VECTOR ),
	DEFINE_FIELD( CBaseToggle, m_sMaster, FIELD_STRING),
	DEFINE_FIELD( CBaseToggle, m_bitsDamageInflict, FIELD_INTEGER ),	// damage type inflicted
};

IMPLEMENT_SAVERESTORE( CBaseToggle, CBaseAnimating )

void CBaseToggle::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq(pkvd->szKeyName, "lip" ) )
	{
		m_flLip = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "wait" ) )
	{
		m_flWait = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "master" ) )
	{
		m_sMaster = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "distance" ) )
	{
		m_flMoveDistance = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}

/*
=============
LinearMove

calculate pev->velocity and pev->nextthink to reach vecDest from
pev->origin traveling at flSpeed
===============
*/
void CBaseToggle::LinearMove( Vector vecDest, float flSpeed )
{
	ASSERTSZ( flSpeed != 0, "LinearMove:  no speed is defined!" );
	//ASSERTSZ( m_pfnCallWhenMoveDone != NULL, "LinearMove: no post-move function defined" );

	m_vecFinalDest = vecDest;

	// Already there?
	if( vecDest == pev->origin )
	{
		LinearMoveDone();
		return;
	}

	// set destdelta to the vector needed to move
	Vector vecDestDelta = vecDest - pev->origin;

	// divide vector length by speed to get time to reach dest
	float flTravelTime = vecDestDelta.Length() / flSpeed;

	// set nextthink to trigger a call to LinearMoveDone when dest is reached
	pev->nextthink = pev->ltime + flTravelTime;
	SetThink( &CBaseToggle::LinearMoveDone );

	// scale the destdelta vector by the time spent traveling to get velocity
	pev->velocity = vecDestDelta / flTravelTime;
}

/*
============
After moving, set origin to exact final destination, call "move done" function
============
*/
void CBaseToggle::LinearMoveDone( void )
{
	Vector delta = m_vecFinalDest - pev->origin;
	float error = delta.Length();
	if( error > 0.03125f )
	{
		LinearMove( m_vecFinalDest, 100 );
		return;
	}

	UTIL_SetOrigin( pev, m_vecFinalDest );
	pev->velocity = g_vecZero;
	pev->nextthink = -1;
	if( m_pfnCallWhenMoveDone )
		( this->*m_pfnCallWhenMoveDone )();
}

BOOL CBaseToggle::IsLockedByMaster( void )
{
	if( m_sMaster && !UTIL_IsMasterTriggered( m_sMaster, m_hActivator ) )
		return TRUE;
	else
		return FALSE;
}

/*
=============
AngularMove

calculate pev->velocity and pev->nextthink to reach vecDest from
pev->origin traveling at flSpeed
Just like LinearMove, but rotational.
===============
*/
void CBaseToggle::AngularMove( Vector vecDestAngle, float flSpeed )
{
	ASSERTSZ( flSpeed != 0, "AngularMove:  no speed is defined!" );
	//ASSERTSZ( m_pfnCallWhenMoveDone != NULL, "AngularMove: no post-move function defined" );

	m_vecFinalAngle = vecDestAngle;

	// Already there?
	if( vecDestAngle == pev->angles )
	{
		AngularMoveDone();
		return;
	}

	// set destdelta to the vector needed to move
	Vector vecDestDelta = vecDestAngle - pev->angles;

	// divide by speed to get time to reach dest
	float flTravelTime = vecDestDelta.Length() / flSpeed;

	// set nextthink to trigger a call to AngularMoveDone when dest is reached
	pev->nextthink = pev->ltime + flTravelTime;
	SetThink( &CBaseToggle::AngularMoveDone );

	// scale the destdelta vector by the time spent traveling to get velocity
	pev->avelocity = vecDestDelta / flTravelTime;
}

/*
============
After rotating, set angle to exact final angle, call "move done" function
============
*/
void CBaseToggle::AngularMoveDone( void )
{
	pev->angles = m_vecFinalAngle;
	pev->avelocity = g_vecZero;
	pev->nextthink = -1;
	if( m_pfnCallWhenMoveDone )
		( this->*m_pfnCallWhenMoveDone )();
}

float CBaseToggle::AxisValue( int flags, const Vector &angles )
{
	if( FBitSet( flags, SF_DOOR_ROTATE_Z ) )
		return angles.z;
	if( FBitSet( flags, SF_DOOR_ROTATE_X ) )
		return angles.x;

	return angles.y;
}

void CBaseToggle::AxisDir( entvars_t *pev )
{
	if( FBitSet( pev->spawnflags, SF_DOOR_ROTATE_Z ) )
		pev->movedir = Vector( 0, 0, 1 );	// around z-axis
	else if( FBitSet( pev->spawnflags, SF_DOOR_ROTATE_X ) )
		pev->movedir = Vector( 1, 0, 0 );	// around x-axis
	else
		pev->movedir = Vector( 0, 1, 0 );		// around y-axis
}

float CBaseToggle::AxisDelta( int flags, const Vector &angle1, const Vector &angle2 )
{
	if( FBitSet( flags, SF_DOOR_ROTATE_Z ) )
		return angle1.z - angle2.z;

	if( FBitSet( flags, SF_DOOR_ROTATE_X ) )
		return angle1.x - angle2.x;

	return angle1.y - angle2.y;
}

bool CBaseToggle::PlaySentence(const char *pszSentence, float duration, float volume, float attenuation , bool subtitle)
{
	if( pszSentence && IsAllowedToSpeak() )
	{
		if( pszSentence[0] == '!' )
		{
			if (subtitle)
				return EMIT_SOUND_DYN_SUB( edict(), CHAN_VOICE, pszSentence, volume, attenuation, 0, PITCH_NORM, ceil(duration)+1 );
			else
				return EMIT_SOUND_DYN( edict(), CHAN_VOICE, pszSentence, volume, attenuation, 0, PITCH_NORM );
		}
		else
		{
			if (subtitle)
				return SENTENCEG_PlayRndSzSub( edict(), pszSentence, volume, attenuation, 0, PITCH_NORM, ceil(duration)+1 ) >= 0;
			else
				return SENTENCEG_PlayRndSz( edict(), pszSentence, volume, attenuation, 0, PITCH_NORM ) >= 0;
		}
	}
	return false;
}

void CBaseToggle::PlayScriptedSentence( const char *pszSentence, float duration, float volume, float attenuation, BOOL bConcurrent, CBaseEntity *pListener )
{
	PlaySentence( pszSentence, duration, volume, attenuation, true );
}

void CBaseToggle::SentenceStop( void )
{
	EMIT_SOUND( edict(), CHAN_VOICE, "common/null.wav", 1.0, ATTN_IDLE );
}

/*
=============
FEntIsVisible

returns TRUE if the passed entity is visible to caller, even if not infront ()
=============
*/
BOOL FEntIsVisible( entvars_t *pev, entvars_t *pevTarget)
{
	Vector vecSpot1 = pev->origin + pev->view_ofs;
	Vector vecSpot2 = pevTarget->origin + pevTarget->view_ofs;
	TraceResult tr;

	UTIL_TraceLine( vecSpot1, vecSpot2, ignore_monsters, ENT( pev ), &tr );

	if( tr.fInOpen && tr.fInWater )
		return FALSE;                   // sight line crossed contents

	if( tr.flFraction == 1 )
		return TRUE;

	return FALSE;
}
