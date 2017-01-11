/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// Alien slave monster
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"squadmonster.h"
#include	"schedule.h"
#include	"effects.h"
#include	"weapons.h"
#include	"soundent.h"

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_ISLAVE_COVER_AND_SUMMON_FAMILIAR = LAST_COMMON_SCHEDULE + 1,
	SCHED_ISLAVE_SUMMON_FAMILIAR
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_ISLAVE_SUMMON_FAMILIAR = LAST_COMMON_TASK + 1,
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		ISLAVE_AE_CLAW			( 1 )
#define		ISLAVE_AE_CLAWRAKE		( 2 )
#define		ISLAVE_AE_ZAP_POWERUP		( 3 )
#define		ISLAVE_AE_ZAP_SHOOT		( 4 )
#define		ISLAVE_AE_ZAP_DONE		( 5 )

#define		ISLAVE_MAX_BEAMS		8

#define ISLAVE_HAND_SPRITE_NAME "sprites/glow02.spr"

#define ISLAVE_ZAP_RED 180
#define ISLAVE_ZAP_GREEN 255
#define ISLAVE_ZAP_BLUE 96

#define ISLAVE_ARMBEAM_RED 96
#define ISLAVE_ARMBEAM_GREEN 128
#define ISLAVE_ARMBEAM_BLUE 16

#define ISLAVE_ELECTROONLY	(1 << 0)
#define ISLAVE_SNARKS		(1 << 1)
#define ISLAVE_HEADCRABS	(1 << 2)

#define ISLAVE_COIL_ATTACK_RADIUS 196

static bool IsVortWounded(CBaseEntity* pEntity)
{
	return pEntity->pev->health < 20;
}

class CISlave : public CSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int ISoundMask( void );
	int Classify( void );
	int IRelationship( CBaseEntity *pTarget );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL CheckRangeAttack1( float flDot, float flDist );
	BOOL CheckRangeAttack2( float flDot, float flDist );
	void CallForHelp( char *szClassname, float flDist, EHANDLE hEnemy, Vector &vecLocation );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );

	void DeathSound( void );
	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );

	void Killed( entvars_t *pevAttacker, int iGib );

	void StartTask( Task_t *pTask );
	void RunTask( Task_t *pTask );
	void SpawnFamiliar(char* entityName, const Vector& origin, const Vector& vecEnd, int hullType);
	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType( int Type );
	CUSTOM_SCHEDULES

	int Save( CSave &save ); 
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	void ClearBeams();
	void ArmBeam(int side );
	void WackBeam( int side, CBaseEntity *pEntity );
	void ZapBeam( int side );
	void BeamGlow( void );
	void HandsGlowOn(int brightness = 224);
	void HandGlowOn(CSprite* handGlow, int brightness, int side);
	void HandsGlowOff();
	void CreateSummonBeams();
	void RemoveSummonBeams();
	void CoilBeam();
	void MakeDynamicLight(const Vector& vecSrc, int radius, int t);
	
	Vector HandPosition(int side);

	CSprite* CreateHandGlow(int attachment);
	CBeam* CreateSummonBeam(const Vector& vecEnd, int attachment);
	
	float HealPower();
	float HealSacrifice();
	void GiveSacrifice();
	bool CanShareHealth();
	bool CanRevive();
	int HealOther(CBaseEntity* pEntity);
	bool CanSpawnFamiliar();
	
	int m_iBravery;

	CBeam *m_pBeam[ISLAVE_MAX_BEAMS];

	int m_iBeams;
	float m_flNextAttack;

	int m_voicePitch;

	EHANDLE m_hDead;
	EHANDLE m_hWounded;
	
	int m_iSpriteTexture;
	
	CSprite	*m_handGlow1;
	CSprite	*m_handGlow2;
	CBeam *m_handsBeam1;
	CBeam *m_handsBeam2;

	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
	static const char *pPainSounds[];
	static const char *pDeathSounds[];
};

LINK_ENTITY_TO_CLASS( monster_alien_slave, CISlave )
LINK_ENTITY_TO_CLASS( monster_vortigaunt, CISlave )

TYPEDESCRIPTION	CISlave::m_SaveData[] =
{
	DEFINE_FIELD( CISlave, m_iBravery, FIELD_INTEGER ),

	DEFINE_ARRAY( CISlave, m_pBeam, FIELD_CLASSPTR, ISLAVE_MAX_BEAMS ),
	DEFINE_FIELD( CISlave, m_iBeams, FIELD_INTEGER ),
	DEFINE_FIELD( CISlave, m_flNextAttack, FIELD_TIME ),

	DEFINE_FIELD( CISlave, m_voicePitch, FIELD_INTEGER ),

	DEFINE_FIELD( CISlave, m_hDead, FIELD_EHANDLE ),

};

IMPLEMENT_SAVERESTORE( CISlave, CSquadMonster )

const char *CISlave::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CISlave::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CISlave::pPainSounds[] =
{
	"aslave/slv_pain1.wav",
	"aslave/slv_pain2.wav",
};

const char *CISlave::pDeathSounds[] =
{
	"aslave/slv_die1.wav",
	"aslave/slv_die2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CISlave::Classify( void )
{
	return CLASS_ALIEN_MILITARY;
}

int CISlave::IRelationship( CBaseEntity *pTarget )
{
	if( ( pTarget->IsPlayer() ) )
		if( ( pev->spawnflags & SF_MONSTER_WAIT_UNTIL_PROVOKED ) && ! ( m_afMemory & bits_MEMORY_PROVOKED ) )
			return R_NO;
	return CBaseMonster::IRelationship( pTarget );
}

void CISlave::CallForHelp( char *szClassname, float flDist, EHANDLE hEnemy, Vector &vecLocation )
{
	// ALERT( at_aiconsole, "help " );

	// skip ones not on my netname
	if( FStringNull( pev->netname ) )
		return;

	CBaseEntity *pEntity = NULL;

	while( ( pEntity = UTIL_FindEntityByString( pEntity, "netname", STRING( pev->netname ) ) ) != NULL)
	{
		float d = ( pev->origin - pEntity->pev->origin ).Length();
		if( d < flDist )
		{
			CBaseMonster *pMonster = pEntity->MyMonsterPointer();
			if( pMonster )
			{
				pMonster->m_afMemory |= bits_MEMORY_PROVOKED;
				pMonster->PushEnemy( hEnemy, vecLocation );
			}
		}
	}
}

//=========================================================
// ALertSound - scream
//=========================================================
void CISlave::AlertSound( void )
{
	if( m_hEnemy != NULL )
	{
		SENTENCEG_PlayRndSz( ENT( pev ), "SLV_ALERT", 0.85, ATTN_NORM, 0, m_voicePitch );

		CallForHelp( "monster_alien_slave", 512, m_hEnemy, m_vecEnemyLKP );
	}
}

//=========================================================
// IdleSound
//=========================================================
void CISlave::IdleSound( void )
{
	if( RANDOM_LONG( 0, 2 ) == 0 )
	{
		SENTENCEG_PlayRndSz( ENT( pev ), "SLV_IDLE", 0.85, ATTN_NORM, 0, m_voicePitch );
	}
#if 1
	int side = RANDOM_LONG( 0, 1 ) * 2 - 1;

	ClearBeams();
	ArmBeam( side );

	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = pev->origin + gpGlobals->v_right * 2 * side;
	MakeDynamicLight(vecSrc, 8, 10);

	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "debris/zap1.wav", 1, ATTN_NORM, 0, 100 );
#endif
}

//=========================================================
// PainSound
//=========================================================
void CISlave::PainSound( void )
{
	if( RANDOM_LONG( 0, 2 ) == 0 )
	{
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pPainSounds[RANDOM_LONG( 0, ARRAYSIZE( pPainSounds ) - 1 )], 1.0, ATTN_NORM, 0, m_voicePitch );
	}
}

//=========================================================
// DieSound
//=========================================================
void CISlave::DeathSound( void )
{
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pDeathSounds[RANDOM_LONG( 0, ARRAYSIZE( pDeathSounds ) - 1 )], 1.0, ATTN_NORM, 0, m_voicePitch );
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CISlave::ISoundMask( void )
{
	return bits_SOUND_WORLD |
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER |
		bits_SOUND_PLAYER;
}

void CISlave::Killed( entvars_t *pevAttacker, int iGib )
{
	ClearBeams();
	UTIL_Remove(m_handGlow1);
	m_handGlow1 = NULL;
	UTIL_Remove(m_handGlow2);
	m_handGlow2 = NULL;
	CSquadMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CISlave::SetYawSpeed( void )
{
	int ys;

	switch( m_Activity )
	{
	case ACT_WALK:		
		ys = 50;	
		break;
	case ACT_RUN:		
		ys = 70;
		break;
	case ACT_IDLE:		
		ys = 50;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CISlave::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	// ALERT( at_console, "event %d : %f\n", pEvent->event, pev->frame );
	switch( pEvent->event )
	{
		case ISLAVE_AE_CLAW:
		{
			// SOUND HERE!
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.slaveDmgClaw, DMG_SLASH );
			if( pHurt )
			{
				if( pHurt->pev->flags & ( FL_MONSTER | FL_CLIENT ) )
				{
					pHurt->pev->punchangle.z = -18;
					pHurt->pev->punchangle.x = 5;
				}
				// Play a random attack hit sound
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG( 0, ARRAYSIZE( pAttackHitSounds ) - 1 )], 1.0, ATTN_NORM, 0, m_voicePitch );
			}
			else
			{
				// Play a random attack miss sound
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG( 0, ARRAYSIZE( pAttackMissSounds ) - 1 )], 1.0, ATTN_NORM, 0, m_voicePitch );
			}
		}
			break;
		case ISLAVE_AE_CLAWRAKE:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.slaveDmgClawrake, DMG_SLASH );
			if( pHurt )
			{
				if( pHurt->pev->flags & ( FL_MONSTER | FL_CLIENT ) )
				{
					pHurt->pev->punchangle.z = -18;
					pHurt->pev->punchangle.x = 5;
				}
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG( 0, ARRAYSIZE( pAttackHitSounds ) - 1 )], 1.0, ATTN_NORM, 0, m_voicePitch );
			}
			else
			{
				EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG( 0, ARRAYSIZE( pAttackMissSounds ) - 1 )], 1.0, ATTN_NORM, 0, m_voicePitch );
			}
		}
			break;
		case ISLAVE_AE_ZAP_POWERUP:
		{
			// speed up attack when on hard
			pev->framerate = gSkillData.slaveZapRate;

			UTIL_MakeAimVectors( pev->angles );

			if( m_iBeams == 0 )
			{
				Vector vecSrc = pev->origin + gpGlobals->v_forward * 2;
				MakeDynamicLight(vecSrc, 12, (int)(20/pev->framerate));
			}
			if( CanRevive() )
			{
				WackBeam( -1, m_hDead );
				WackBeam( 1, m_hDead );
			}
			else
			{
				ArmBeam( -1 );
				ArmBeam( 1 );
				BeamGlow();
			}

			EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "debris/zap4.wav", 1, ATTN_NORM, 0, 100 + m_iBeams * 10 );
			pev->skin = m_iBeams / 2;
		}
			break;
		case ISLAVE_AE_ZAP_SHOOT:
		{
			ClearBeams();

			if( CanRevive() )
			{
				Vector vecDest = m_hDead->pev->origin + Vector( 0, 0, 38 );
				TraceResult trace;
				UTIL_TraceHull( vecDest, vecDest, dont_ignore_monsters, human_hull, m_hDead->edict(), &trace );

				if( !trace.fStartSolid )
				{
					CBaseEntity *pNew = Create( "monster_alien_slave", m_hDead->pev->origin, m_hDead->pev->angles );
					
					if (m_hDead->pev->weapons) {
						pNew->pev->weapons = m_hDead->pev->weapons;
					}
					
					CBaseMonster *pNewMonster = pNew->MyMonsterPointer( );
					//pNew->pev->spawnflags |= 1;
					WackBeam( -1, pNew );
					WackBeam( 1, pNew );
					UTIL_Remove( m_hDead );
					EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "hassault/hw_shoot1.wav", 1, ATTN_NORM, 0, RANDOM_LONG( 130, 160 ) );
					
					GiveSacrifice();
					
					/*
					CBaseEntity *pEffect = Create( "test_effect", pNew->Center(), pev->angles );
					pEffect->Use( this, this, USE_ON, 1 );
					*/
					break;
				}
			}
			ClearMultiDamage();

			bool coilAttack = false;
			if (m_hWounded && CanShareHealth() && IsVortWounded(m_hWounded) && m_hWounded->IsAlive() && (pev->origin - m_hWounded->pev->origin).Length() <= ISLAVE_COIL_ATTACK_RADIUS) {
				coilAttack = true;
				ALERT(at_aiconsole, "Vort makes coil attack to heal friend\n");
			} else if ((pev->origin - m_vecEnemyLKP).Length() <= ISLAVE_COIL_ATTACK_RADIUS) {
				coilAttack = true;
			}
			
			if (coilAttack) {
				CoilBeam();
				
				float flAdjustedDamage = gSkillData.slaveDmgZap*3;
				CBaseEntity *pEntity = NULL;
				while( ( pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, ISLAVE_COIL_ATTACK_RADIUS ) ) != NULL )
				{
					if( pEntity != this && pEntity->pev->takedamage != DAMAGE_NO && pEntity->MyMonsterPointer() != NULL ) {
						if (IRelationship(pEntity) >= R_DL) {
							if( !FVisible( pEntity ) ) {
								flAdjustedDamage *= 0.5;
							}
			
							if( flAdjustedDamage > 0 ) {
								pEntity->TakeDamage( pev, pev, flAdjustedDamage, DMG_SHOCK );
							}
						} else {
							if (FClassnameIs(pEntity->pev, STRING(pev->classname))) {
								if (HealOther(pEntity)) {
									ALERT(at_aiconsole, "Vortigaunt healed friend with coil attack\n");
								}
							}
						}
					}
				}
				UTIL_EmitAmbientSound( ENT( pev ), pev->origin, "weapons/electro4.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG( 140, 160 ) );
			} else {
				UTIL_MakeAimVectors( pev->angles );
	
				ZapBeam( -1 );
				ZapBeam( 1 );
				
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "hassault/hw_shoot1.wav", 1, ATTN_NORM, 0, RANDOM_LONG( 130, 160 ) );
				// STOP_SOUND( ENT( pev ), CHAN_WEAPON, "debris/zap4.wav" );
				ApplyMultiDamage( pev, pev );
			}

			m_flNextAttack = gpGlobals->time + RANDOM_FLOAT( 0.5, 4.0 );
		}
			break;
		case ISLAVE_AE_ZAP_DONE:
		{
			ClearBeams();
		}
			break;
		default:
			CSquadMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// CheckRangeAttack1 - normal beam attack 
//=========================================================
BOOL CISlave::CheckRangeAttack1( float flDot, float flDist )
{
	if( m_flNextAttack > gpGlobals->time )
	{
		return FALSE;
	}
	
	if( flDist > 32 && flDist <= ISLAVE_COIL_ATTACK_RADIUS )
	{
		return TRUE;
	}

	return CSquadMonster::CheckRangeAttack1( flDot, flDist );
}

//=========================================================
// CheckRangeAttack2 - try to resurect dead comrades or heal wounded ones
//=========================================================
BOOL CISlave::CheckRangeAttack2( float flDot, float flDist )
{
	if( m_flNextAttack > gpGlobals->time )
	{
		return FALSE;
	}

	m_hDead = NULL;
	m_hWounded = NULL;

	CBaseEntity *pEntity = NULL;
	while( ( pEntity = UTIL_FindEntityByClassname( pEntity, "monster_alien_slave" ) ) != NULL )
	{
		TraceResult tr;

		UTIL_TraceLine( EyePosition(), pEntity->EyePosition(), ignore_monsters, ENT( pev ), &tr );
		if( tr.flFraction == 1.0 || tr.pHit == pEntity->edict() )
		{
			if( pEntity->pev->deadflag == DEAD_DEAD )
			{
				float d = ( pev->origin - pEntity->pev->origin ).Length();
				if( d < flDist )
				{
					m_hDead = pEntity;
					flDist = d;
				}
			}
			else if ( pEntity != this && pEntity->IsAlive() && IsVortWounded(pEntity) ) 
			{
				float d = ( pev->origin - pEntity->pev->origin ).Length();
				if( d < flDist )
				{
					m_hWounded = pEntity;
					flDist = d;
				}
			}
		}
	}
	if( m_hDead != NULL || m_hWounded != NULL )
		return TRUE;
	else
		return FALSE;
}

//=========================================================
// StartTask
//=========================================================
void CISlave::StartTask( Task_t *pTask )
{
	ClearBeams();
	
	if (pTask->iTask == TASK_ISLAVE_SUMMON_FAMILIAR) {
		ALERT(at_aiconsole, "start TASK_ISLAVE_FAMILIAR\n");
		m_IdealActivity = ACT_CROUCH;
		EMIT_SOUND( edict(), CHAN_BODY, "debris/beamstart1.wav", 1, ATTN_NORM );
		UTIL_MakeAimVectors( pev->angles );
		Vector vecSrc = pev->origin + gpGlobals->v_forward * 8;
		MakeDynamicLight(vecSrc, 10, 15);
		HandsGlowOn();
		CreateSummonBeams();
	} else {
		CSquadMonster::StartTask( pTask );
	}
}

void CISlave::RunTask(Task_t *pTask)
{
	if (pTask->iTask == TASK_ISLAVE_SUMMON_FAMILIAR) {
		if( m_fSequenceFinished )
		{
			UTIL_MakeVectors( pev->angles );
			if (pev->weapons & ISLAVE_HEADCRABS) {
				Vector headcrabOrigin = pev->origin + gpGlobals->v_forward * 42;
				Vector headcrabVecEnd =  pev->origin + gpGlobals->v_forward * 52;
				SpawnFamiliar("monster_headcrab", headcrabOrigin, headcrabVecEnd, human_hull);
			} else if (pev->weapons & ISLAVE_SNARKS) {
				Vector snarkOrigin = pev->origin + gpGlobals->v_forward * 30;
				Vector snarkVecEnd =  pev->origin + gpGlobals->v_forward * 40;
				SpawnFamiliar("monster_snark", snarkOrigin, snarkVecEnd, head_hull);
			}
			HandsGlowOff();
			TaskComplete();
			RemoveSummonBeams();
		}
	} else {
		CSquadMonster::RunTask( pTask );
	}
}

void CISlave::SpawnFamiliar(char *entityName, const Vector &origin, const Vector &vecEnd, int hullType)
{	
	TraceResult tr;
	UTIL_TraceHull( pev->origin, vecEnd, dont_ignore_monsters, hullType, ENT( pev ), &tr );
	if (tr.flFraction == 1.0) {
		CBaseEntity *pNew = Create( entityName, origin, pev->angles );
		CBaseMonster *pNewMonster = pNew->MyMonsterPointer( );
	
		if(pNew) {
			CSprite *pSpr = CSprite::SpriteCreate( "sprites/bexplo.spr", origin, TRUE );
			pSpr->AnimateAndDie( 20 );
			pSpr->SetTransparency( kRenderGlow,  77, 210, 130,  255, kRenderFxNoDissipation );
			EMIT_SOUND( pNew->edict(), CHAN_BODY, "debris/beamstart7.wav", 0.9, ATTN_NORM );
			
			SetBits( pNew->pev->spawnflags, SF_MONSTER_FALL_TO_GROUND );
			if (pNewMonster) {
				pNewMonster->m_hEnemy = m_hEnemy;
				pNewMonster->m_vecEnemyLKP = m_vecEnemyLKP;
				pNewMonster->Remember(bits_MEMORY_PROVOKED);
				pNewMonster->m_IdealMonsterState = m_IdealMonsterState;
			}
		}
	} else {
		ALERT(at_aiconsole, "Not enough room to create %s\n", entityName);
	}
}

CSprite* CISlave::CreateHandGlow(int attachment)
{
	CSprite* handSprite = CSprite::SpriteCreate( ISLAVE_HAND_SPRITE_NAME, pev->origin, FALSE );
	handSprite->SetTransparency( kRenderGlow, ISLAVE_ZAP_RED, ISLAVE_ZAP_GREEN, ISLAVE_ZAP_BLUE, 224, kRenderFxNoDissipation );
	handSprite->SetAttachment( edict(), attachment );
	handSprite->SetScale(0.25);
	return handSprite;
}

//=========================================================
// Spawn
//=========================================================
void CISlave::Spawn()
{
	Precache();

	SET_MODEL( ENT( pev ), "models/islave.mdl" );
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid		= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->effects		= 0;
	pev->health		= gSkillData.slaveHealth;
	pev->view_ofs		= Vector( 0, 0, 64 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_RANGE_ATTACK2 | bits_CAP_DOORS_GROUP;

	m_voicePitch		= RANDOM_LONG( 85, 110 );
	
	m_handGlow1 = CreateHandGlow(1);
	m_handGlow2 = CreateHandGlow(2);
	
	HandsGlowOff();
	
	if (!pev->weapons) {
		pev->weapons = ISLAVE_SNARKS;
	}

	MonsterInit();
	
	pev->max_health = gSkillData.slaveHealth * 3;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CISlave::Precache()
{
	int i;

	PRECACHE_MODEL( "models/islave.mdl" );
	m_iSpriteTexture = PRECACHE_MODEL( "sprites/lgtning.spr" );
	PRECACHE_MODEL( ISLAVE_HAND_SPRITE_NAME );
	PRECACHE_MODEL( "sprites/bexplo.spr" );
	PRECACHE_SOUND( "debris/zap1.wav" );
	PRECACHE_SOUND( "debris/zap4.wav" );
	PRECACHE_SOUND( "debris/beamstart1.wav" );
	PRECACHE_SOUND( "debris/beamstart7.wav" );
	PRECACHE_SOUND( "weapons/electro4.wav" );
	PRECACHE_SOUND( "hassault/hw_shoot1.wav" );
	PRECACHE_SOUND( "zombie/zo_pain2.wav" );
	PRECACHE_SOUND( "headcrab/hc_headbite.wav" );
	PRECACHE_SOUND( "weapons/cbar_miss1.wav" );

	for( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND( (char *)pAttackHitSounds[i] );

	for( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND( (char *)pAttackMissSounds[i] );

	for( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i] );

	for( i = 0; i < ARRAYSIZE( pDeathSounds ); i++ )
		PRECACHE_SOUND( (char *)pDeathSounds[i] );

	UTIL_PrecacheOther( "test_effect" );
	UTIL_PrecacheOther( "monster_snark" );
	UTIL_PrecacheOther( "monster_headcrab" );
}

//=========================================================
// TakeDamage - get provoked when injured
//=========================================================

int CISlave::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// don't slash one of your own
	if( ( bitsDamageType & DMG_SLASH ) && pevAttacker && IRelationship( Instance( pevAttacker ) ) < R_DL )
		return 0;

	m_afMemory |= bits_MEMORY_PROVOKED;
	return CSquadMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CISlave::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if( bitsDamageType & DMG_SHOCK )
		return;

	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

// primary range attack
Task_t	tlSlaveAttack1[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slSlaveAttack1[] =
{
	{ 
		tlSlaveAttack1,
		ARRAYSIZE ( tlSlaveAttack1 ), 
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_HEAR_SOUND |
		bits_COND_HEAVY_DAMAGE, 

		bits_SOUND_DANGER,
		"Slave Range Attack1"
	},
};

Task_t tlSlaveCoverAndSummon[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_WAIT, (float)0.1 },
	{ TASK_FIND_COVER_FROM_ENEMY, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_REMEMBER, (float)bits_MEMORY_INCOVER },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_ISLAVE_SUMMON_FAMILIAR, (float)0 },
};

Schedule_t slSlaveCoverAndSummon[] =
{
	{
		tlSlaveCoverAndSummon,
		ARRAYSIZE( tlSlaveCoverAndSummon ),
		bits_COND_NEW_ENEMY,
		0,
		"Slave Cover and Summon"
	},
};

Task_t tlSlaveSummon[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_ISLAVE_SUMMON_FAMILIAR, (float)0 }
};

Schedule_t slSlaveSummon[] =
{
	{
		tlSlaveSummon,
		ARRAYSIZE( tlSlaveSummon ),
		bits_COND_NEW_ENEMY,
		0,
		"Slave Summon"
	}
};

DEFINE_CUSTOM_SCHEDULES( CISlave )
{
	slSlaveAttack1,
	slSlaveCoverAndSummon,
	slSlaveSummon
};

IMPLEMENT_CUSTOM_SCHEDULES( CISlave, CSquadMonster )

//=========================================================
//=========================================================
Schedule_t *CISlave::GetSchedule( void )
{
	ClearBeams();
/*
	if( pev->spawnflags )
	{
		pev->spawnflags = 0;
		return GetScheduleOfType( SCHED_RELOAD );
	}
*/
	if( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );

		if( pSound && ( pSound->m_iType & bits_SOUND_DANGER ) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
		if( pSound->m_iType & bits_SOUND_COMBAT )
			m_afMemory |= bits_MEMORY_PROVOKED;
	}

	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		// dead enemy
		if( HasConditions( bits_COND_ENEMY_DEAD ) )
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster::GetSchedule();
		}

		if( IsVortWounded(this) )
		{
			if( !HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				m_failSchedule = SCHED_CHASE_ENEMY;
				int sched = SCHED_TAKE_COVER_FROM_ENEMY;
				if (CanSpawnFamiliar()) {
					sched = SCHED_ISLAVE_COVER_AND_SUMMON_FAMILIAR;
					m_failSchedule = SCHED_ISLAVE_SUMMON_FAMILIAR;
				}
				
				if( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
				{
					return GetScheduleOfType( sched );
				}
				if( HasConditions( bits_COND_SEE_ENEMY ) && HasConditions( bits_COND_ENEMY_FACING_ME ) 
						&& RANDOM_LONG(0,1) ) // give chance to use electro attack to restore health
				{
					return GetScheduleOfType( sched );
				}
			}
		}
		break;
	default:
		break;
	}
	return CSquadMonster::GetSchedule();
}

Schedule_t *CISlave::GetScheduleOfType( int Type ) 
{
	switch( Type )
	{
	case SCHED_FAIL:
		if( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
		{
			return CSquadMonster::GetScheduleOfType( SCHED_MELEE_ATTACK1 );
		}
		else if (CanSpawnFamiliar())
		{
			ALERT(at_aiconsole, "Vort gonna spawn a familiar\n");
			return GetScheduleOfType( SCHED_ISLAVE_SUMMON_FAMILIAR );
		}
		else if ( (m_hDead || m_hWounded) && HasConditions( bits_COND_CAN_RANGE_ATTACK2) )
		{
			ALERT(at_aiconsole, "Vort gonna heal friends after sched fail\n");
			return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
		}
		break;
	case SCHED_RANGE_ATTACK1:
		return slSlaveAttack1;
	case SCHED_RANGE_ATTACK2:
		return slSlaveAttack1;
	case SCHED_ISLAVE_COVER_AND_SUMMON_FAMILIAR:
		return slSlaveCoverAndSummon;
	case SCHED_ISLAVE_SUMMON_FAMILIAR:
		return slSlaveSummon;
	}
	return CSquadMonster::GetScheduleOfType( Type );
}

//=========================================================
// ArmBeam - small beam from arm to nearby geometry
//=========================================================
void CISlave::ArmBeam( int side )
{
	TraceResult tr;
	float flDist = 1.0;

	if( m_iBeams >= ISLAVE_MAX_BEAMS )
		return;

	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = HandPosition(side);

	for( int i = 0; i < 3; i++ )
	{
		Vector vecAim = gpGlobals->v_right * side * RANDOM_FLOAT( 0, 1 ) + gpGlobals->v_up * RANDOM_FLOAT( -1, 1 );
		TraceResult tr1;
		UTIL_TraceLine( vecSrc, vecSrc + vecAim * 512, dont_ignore_monsters, ENT( pev ), &tr1 );
		if( flDist > tr1.flFraction )
		{
			tr = tr1;
			flDist = tr.flFraction;
		}
	}

	// Couldn't find anything close enough
	if( flDist == 1.0 )
		return;

	DecalGunshot( &tr, BULLET_PLAYER_CROWBAR );

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );
	if( !m_pBeam[m_iBeams] )
		return;

	m_pBeam[m_iBeams]->PointEntInit( tr.vecEndPos, entindex() );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? 2 : 1 );
	// m_pBeam[m_iBeams]->SetColor( 180, 255, 96 );
	m_pBeam[m_iBeams]->SetColor( ISLAVE_ARMBEAM_RED, ISLAVE_ARMBEAM_GREEN, ISLAVE_ARMBEAM_BLUE );
	m_pBeam[m_iBeams]->SetBrightness( 64 );
	m_pBeam[m_iBeams]->SetNoise( 80 );
	m_iBeams++;
}

//=========================================================
// BeamGlow - brighten all beams
//=========================================================
void CISlave::BeamGlow()
{
	int b = m_iBeams * 32;
	if( b > 255 )
		b = 255;
	
	HandsGlowOn(b);

	for( int i = 0; i < m_iBeams; i++ )
	{
		if( m_pBeam[i]->GetBrightness() != 255 )
		{
			m_pBeam[i]->SetBrightness( b );
		}
	}
}

//=========================================================
// WackBeam - regenerate dead colleagues
//=========================================================
void CISlave::WackBeam( int side, CBaseEntity *pEntity )
{
	Vector vecDest;
	float flDist = 1.0;

	if( m_iBeams >= ISLAVE_MAX_BEAMS )
		return;

	if( pEntity == NULL )
		return;

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );
	if( !m_pBeam[m_iBeams] )
		return;

	m_pBeam[m_iBeams]->PointEntInit( pEntity->Center(), entindex() );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? 2 : 1 );
	m_pBeam[m_iBeams]->SetColor( ISLAVE_ZAP_RED, ISLAVE_ZAP_GREEN, ISLAVE_ZAP_BLUE );
	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 80 );
	m_iBeams++;
}

//=========================================================
// ZapBeam - heavy damage directly forward
//=========================================================
void CISlave::ZapBeam( int side )
{
	Vector vecSrc, vecAim;
	TraceResult tr;
	CBaseEntity *pEntity;

	if( m_iBeams >= ISLAVE_MAX_BEAMS )
		return;

	vecSrc = pev->origin + gpGlobals->v_up * 36;
	
	if (m_hWounded && m_hWounded->IsAlive()) {
		vecAim = ( ( m_hWounded->BodyTarget( vecSrc ) ) - vecSrc ).Normalize();
		ALERT(at_aiconsole, "Vort shoot friend on purpose to heal\n");
	} else {
		vecAim = ShootAtEnemy( vecSrc );
	}
	
	float deflection = 0.01;
	vecAim = vecAim + side * gpGlobals->v_right * RANDOM_FLOAT( 0, deflection ) + gpGlobals->v_up * RANDOM_FLOAT( -deflection, deflection );
	UTIL_TraceLine( vecSrc, vecSrc + vecAim * 1024, dont_ignore_monsters, ENT( pev ), &tr );

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 50 );
	if( !m_pBeam[m_iBeams] )
		return;

	m_pBeam[m_iBeams]->PointEntInit( tr.vecEndPos, entindex() );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? 2 : 1 );
	m_pBeam[m_iBeams]->SetColor( ISLAVE_ZAP_RED, ISLAVE_ZAP_GREEN, ISLAVE_ZAP_BLUE );
	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 20 );
	m_iBeams++;

	pEntity = CBaseEntity::Instance( tr.pHit );
	if( pEntity != NULL && pEntity->pev->takedamage )
	{
		if (IRelationship(pEntity) < R_DL && FClassnameIs(pEntity->pev, STRING(pev->classname))) {
			if (HealOther(pEntity)) {
				ALERT(at_aiconsole, "Vortigaunt healed friend with zap attack\n");
			}
		} else {
			pEntity->TraceAttack( pev, gSkillData.slaveDmgZap, vecAim, &tr, DMG_SHOCK );
			if (pEntity->pev->flags & (FL_CLIENT | FL_MONSTER)) {
				//TODO: check that target is actually a living creature, not machine
				if (TakeHealth(gSkillData.slaveDmgZap/2, DMG_GENERIC)) // give some health to vortigaunt like in Decay bonus mission
				{
					ALERT(at_aiconsole, "Vortigaunt get health from enemy\n");
				}
			}
		}
	}
	UTIL_EmitAmbientSound( ENT( pev ), tr.vecEndPos, "weapons/electro4.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG( 140, 160 ) );
}

//=========================================================
// ClearBeams - remove all beams
//=========================================================
void CISlave::ClearBeams()
{
	for( int i = 0; i < ISLAVE_MAX_BEAMS; i++ )
	{
		if( m_pBeam[i] )
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}
	m_iBeams = 0;
	pev->skin = 0;
	
	HandsGlowOff();
	RemoveSummonBeams();

	STOP_SOUND( ENT( pev ), CHAN_WEAPON, "debris/zap4.wav" );
}

void CISlave::CoilBeam()
{
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z + 16 );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z + 16 + ISLAVE_COIL_ATTACK_RADIUS*5 ); 
		WRITE_SHORT( m_iSpriteTexture );
		WRITE_BYTE( 0 ); // startframe
		WRITE_BYTE( 10 ); // framerate
		WRITE_BYTE( 2 ); // life
		WRITE_BYTE( 128 );  // width
		WRITE_BYTE( 20 );   // noise

		WRITE_BYTE( ISLAVE_ZAP_RED );
		WRITE_BYTE( ISLAVE_ZAP_GREEN );
		WRITE_BYTE( ISLAVE_ZAP_BLUE );

		WRITE_BYTE( 255 ); //brightness
		WRITE_BYTE( 0 );		// speed
	MESSAGE_END();
	
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z + 48 );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z + 48 + ISLAVE_COIL_ATTACK_RADIUS*2 ); 
		WRITE_SHORT( m_iSpriteTexture );
		WRITE_BYTE( 0 ); // startframe
		WRITE_BYTE( 10 ); // framerate
		WRITE_BYTE( 2 ); // life
		WRITE_BYTE( 128 );  // width
		WRITE_BYTE( 25 );   // noise

		WRITE_BYTE( ISLAVE_ZAP_RED );
		WRITE_BYTE( ISLAVE_ZAP_GREEN );
		WRITE_BYTE( ISLAVE_ZAP_BLUE );

		WRITE_BYTE( 255 ); //brightness
		WRITE_BYTE( 0 );		// speed
	MESSAGE_END();
}

void CISlave::MakeDynamicLight(const Vector &vecSrc, int radius, int t)
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
		WRITE_BYTE( TE_DLIGHT );
		WRITE_COORD( vecSrc.x );	// X
		WRITE_COORD( vecSrc.y );	// Y
		WRITE_COORD( vecSrc.z );	// Z
		WRITE_BYTE( radius );		// radius * 0.1
		WRITE_BYTE( 255 );		// r
		WRITE_BYTE( 180 );		// g
		WRITE_BYTE( 96 );		// b
		WRITE_BYTE( t );		// time * 10
		WRITE_BYTE( 0 );		// decay * 0.1
	MESSAGE_END();
}

void CISlave::HandsGlowOff()
{
	if (m_handGlow1) {
		m_handGlow1->pev->effects |= EF_NODRAW;
	}
	if (m_handGlow2) {
		m_handGlow2->pev->effects |= EF_NODRAW;
	}
}

void CISlave::HandsGlowOn(int brightness)
{
	HandGlowOn(m_handGlow1, brightness, -1);
	HandGlowOn(m_handGlow2, brightness, 1);
}

void CISlave::HandGlowOn(CSprite *handGlow, int brightness, int side)
{
	if (handGlow) {
		UTIL_SetOrigin(handGlow->pev, HandPosition(side));
		handGlow->SetBrightness(brightness);
		handGlow->SetScale(brightness / (float)255 * 0.3);
		handGlow->pev->effects &= ~EF_NODRAW;
	}
}

Vector CISlave::HandPosition(int side)
{
	UTIL_MakeAimVectors( pev->angles );
	return pev->origin + gpGlobals->v_up * 36 + gpGlobals->v_right * side * 16 + gpGlobals->v_forward * 32;
}

void CISlave::CreateSummonBeams()
{
	UTIL_MakeVectors(pev->angles);
	Vector vecEnd = pev->origin + gpGlobals->v_forward * 30;
	if (m_handGlow1) {
		m_handsBeam1 = CreateSummonBeam(vecEnd, 1);
	}
	if (m_handGlow2) {
		m_handsBeam2 = CreateSummonBeam(vecEnd, 2);
	}
}

CBeam* CISlave::CreateSummonBeam(const Vector& vecEnd, int attachment)
{
	CBeam* beam = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );
	if( !beam )
		return beam;

	beam->PointEntInit(vecEnd, entindex());
	beam->SetEndAttachment(attachment);
	beam->SetColor( ISLAVE_ZAP_RED, ISLAVE_ZAP_GREEN, ISLAVE_ZAP_BLUE );
	beam->SetBrightness( 192 );
	beam->SetNoise( 80 );
	return beam;
}

void CISlave::RemoveSummonBeams()
{
	UTIL_Remove(m_handsBeam1);
	m_handsBeam1 = NULL;
	UTIL_Remove(m_handsBeam2);
	m_handsBeam2 = NULL;
}

float CISlave::HealPower()
{
	return gSkillData.slaveDmgZap;
}

float CISlave::HealSacrifice()
{
	//vortigaunt heals more health than loses, because friendship is magic
	return gSkillData.slaveDmgZap / 2; 
}

void CISlave::GiveSacrifice()
{
	if (CanShareHealth()) {
		pev->health -= HealSacrifice();
	}
}

bool CISlave::CanShareHealth()
{
	return pev->health >= gSkillData.slaveHealth + HealSacrifice();
}

bool CISlave::CanRevive()
{
	return m_hDead != NULL && CanShareHealth();
}

int CISlave::HealOther(CBaseEntity *pEntity)
{
	int result = 0;
	if (pEntity->IsAlive()) {
		if (CanShareHealth()) {
			result = pEntity->TakeHealth(HealPower(), DMG_GENERIC);
			if (result) {
				GiveSacrifice();
			}
		} else {
			// vort was damaged to this moment, still give some health to friend
			result = pEntity->TakeHealth(HealPower() / 2, DMG_GENERIC);
		}
	}
	return result;
}

bool CISlave::CanSpawnFamiliar()
{
	return (pev->weapons & (ISLAVE_SNARKS | ISLAVE_HEADCRABS)) != 0;
}

