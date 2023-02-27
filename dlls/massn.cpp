#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"weapons.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"hgrunt.h"
#include	"mod_features.h"
#include	"gamerules.h"
#include	"game.h"
#include	"shake.h"

#if FEATURE_MASSN

extern BOOL g_hasFlashbangModel;

class CFlashGrenade : public CBaseMonster
{
public:
	void Precache();
	void Spawn( void );

	static CFlashGrenade *ShootTimed( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time );

	void EXPORT BounceTouch( CBaseEntity *pOther );
	void EXPORT TumbleThink( void );
	void EXPORT Detonate( void );
	void EXPORT Smoke( void );
};

LINK_ENTITY_TO_CLASS( fgrenade, CFlashGrenade )

void CFlashGrenade::Precache()
{
	PRECACHE_MODEL("models/w_fgrenade.mdl");
	PRECACHE_SOUND("debris/metal6.wav");
}

void CFlashGrenade::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_BOUNCE;
	pev->classname = MAKE_STRING( "fgrenade" );
	pev->scale = 1.5f;

	pev->solid = SOLID_BBOX;

	SET_MODEL( ENT( pev ), "models/w_fgrenade.mdl" );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
}

CFlashGrenade *CFlashGrenade::ShootTimed( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time )
{
	CFlashGrenade *pGrenade = GetClassPtr( (CFlashGrenade *)NULL );
	pGrenade->Spawn();
	UTIL_SetOrigin( pGrenade->pev, vecStart );
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = Vector(270, 0, 0);
	pGrenade->pev->owner = ENT( pevOwner );

	pGrenade->SetTouch( &CFlashGrenade::BounceTouch );

	pGrenade->pev->dmgtime = gpGlobals->time + time;
	pGrenade->SetThink( &CFlashGrenade::TumbleThink );
	pGrenade->pev->nextthink = gpGlobals->time + 0.1f;
	if( time < 0.1f )
	{
		pGrenade->pev->nextthink = gpGlobals->time;
		pGrenade->pev->velocity = Vector( 0, 0, 0 );
	}

	//pGrenade->pev->sequence = RANDOM_LONG( 3, 6 );
	pGrenade->pev->framerate = 1.0f;

	pGrenade->pev->gravity = 0.5f;
	pGrenade->pev->friction = 0.8f;

	return pGrenade;
}

void CFlashGrenade::BounceTouch( CBaseEntity *pOther )
{
	// don't hit the guy that launched this grenade
	if( pOther->edict() == pev->owner )
		return;

	// only do damage if we're moving fairly fast
	if( m_flNextAttack < gpGlobals->time && pev->velocity.Length() > 100 )
	{
		entvars_t *pevOwner = VARS( pev->owner );
		if( pevOwner && pOther->pev->takedamage )
		{
			TraceResult tr = UTIL_GetGlobalTrace();
			pOther->ApplyTraceAttack( pev, pevOwner, 1, gpGlobals->v_forward, &tr, DMG_CLUB );
		}
		m_flNextAttack = gpGlobals->time + 1.0f; // debounce
	}

	Vector vecTestVelocity;
	// pev->avelocity = Vector( 300, 300, 300 );

	// this is my heuristic for modulating the grenade velocity because grenades dropped purely vertical
	// or thrown very far tend to slow down too quickly for me to always catch just by testing velocity.
	// trimming the Z velocity a bit seems to help quite a bit.
	vecTestVelocity = pev->velocity;
	vecTestVelocity.z *= 0.45f;

	if( pev->flags & FL_ONGROUND )
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.8f;

		//pev->sequence = RANDOM_LONG( 1, 1 );
	}
	else
	{
		// play bounce sound
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "debris/metal6.wav", 0.20f, ATTN_NORM, 0, 85 );
	}
	pev->framerate = pev->velocity.Length() / 200.0f;
	if( pev->framerate > 1.0f )
		pev->framerate = 1.0f;
	else if( pev->framerate < 0.5f )
		pev->framerate = 0.0f;
}

void CFlashGrenade::TumbleThink( void )
{
	if( !IsInWorld() )
	{
		UTIL_Remove( this );
		return;
	}

	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1f;

	if( pev->dmgtime <= gpGlobals->time )
	{
		SetThink( &CFlashGrenade::Detonate );
	}
	if( pev->waterlevel != 0 )
	{
		pev->velocity = pev->velocity * 0.5f;
		pev->framerate = 0.2f;
	}
}

#define MINFLASH_DISTANCE 384.0f
#define MAXFLASH_DISTANCE 640.0f

void CFlashGrenade::Detonate( void )
{
	Vector vecSpot = pev->origin + Vector( 0, 0, 8 );
	if (UTIL_PointContents(vecSpot) == CONTENTS_SOLID)
		vecSpot = pev->origin;

	pev->model = iStringNull;//invisible
	pev->solid = SOLID_NOT;// intangible
	pev->takedamage = DAMAGE_NO;

	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "weapons/debris3.wav", 1.0f, ATTN_NORM, 0, 115 );

	pev->effects |= EF_NODRAW;

	pev->velocity = g_vecZero;

	const Vector white = Vector(255, 255, 255);
	const float fadeTime = gSkillData.massnFlashFadeTime;
	const float holdTime = gSkillData.massnFlashHoldTime;
	const float flashRadius = gSkillData.massnFlashRadius;

	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
		if (pPlayer)
		{
			const Vector playerEyes = pPlayer->EyePosition();
			const float dist = (playerEyes - vecSpot).Length();

			if (dist <= flashRadius)
			{
				TraceResult trToPlayer;
				UTIL_TraceLine( vecSpot, playerEyes, ignore_monsters, ENT( pev ), &trToPlayer );
				if (trToPlayer.flFraction == 1.0f)
				{
					const float fadeRadius = flashRadius * 0.75;
					int alpha = 255;
					if (dist > fadeRadius)
					{
						alpha -= ((dist - fadeRadius) / (flashRadius - fadeRadius)) * 30;
					}
					const bool faded = UTIL_ScreenFade( vecSpot, pPlayer, white, fadeTime, holdTime, alpha, 0 );
					if (!faded && dist <= flashRadius * 0.25f)
					{
						UTIL_ScreenFade(pPlayer, white, fadeTime, holdTime, 80, 0);
					}
				}
			}
		}
	}

	SetThink(&CFlashGrenade::Smoke);
	pev->nextthink = gpGlobals->time + 0.3f;

	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, -32 ), ignore_monsters, ENT( pev ), &tr );

	const int iContents = UTIL_PointContents( pev->origin );
	if( iContents != CONTENTS_WATER )
	{
		int sparkCount = RANDOM_LONG( 0, 3 );
		for( int i = 0; i < sparkCount; i++ )
			Create( "spark_shower", pev->origin, tr.vecPlaneNormal, NULL );
	}
}

void CFlashGrenade::Smoke( void )
{
	extern int gmsgSmoke;

	if( UTIL_PointContents( pev->origin ) == CONTENTS_WATER )
	{
		UTIL_Bubbles( pev->origin - Vector( 64, 64, 64 ), pev->origin + Vector( 64, 64, 64 ), 100 );
	}
	else
	{
		MESSAGE_BEGIN( MSG_PVS, gmsgSmoke, pev->origin );
			WRITE_BYTE( 0 );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 30 ); // scale * 10
			WRITE_BYTE( 10 ); // framerate
			WRITE_SHORT( 0 );
			WRITE_SHORT( 0 );
			WRITE_BYTE( 0 );
			WRITE_BYTE( 255 );
			WRITE_BYTE( 100 );
			WRITE_BYTE( 100 );
			WRITE_BYTE( 100 );
			WRITE_BYTE( 0 );
		MESSAGE_END();
	}
	UTIL_Remove( this );
}

#if 0
class CEnvFGrenadeShooter : public CPointEntity
{
public:
	void Spawn();
	void Precache();
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS( env_fgrenade_shooter, CEnvFGrenadeShooter )

void CEnvFGrenadeShooter::Spawn()
{
	Precache();
	CPointEntity::Spawn();
}

void CEnvFGrenadeShooter::Precache()
{
	UTIL_PrecacheOther("fgrenade");
}

void CEnvFGrenadeShooter::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	CFlashGrenade::ShootTimed(pev, pev->origin, Vector(0,0,1), 2.5f);
}
#endif

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define	MASSN_CLIP_SIZE				36 // how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!

// Weapon flags
#define MASSN_9MMAR					(1 << 0)
#define MASSN_HANDGRENADE			(1 << 1)
#define MASSN_GRENADELAUNCHER		(1 << 2)
#define MASSN_SNIPERRIFLE			(1 << 3)
#define MASSN_FLASHGRENADE			(1 << 5)

// Body groups.
#define MASSN_HEAD_GROUP					1
#define MASSN_GUN_GROUP					2

// Head values
enum
{
	MASSN_HEAD_WHITE,
	MASSN_HEAD_BLACK,
	MASSN_HEAD_GOOGLES,
	MASSN_HEAD_COUNT,
};

// Gun values
#define MASSN_GUN_MP5				0
#define MASSN_GUN_SNIPERRIFLE				1
#define MASSN_GUN_NONE					2

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		MASSN_AE_KICK			( 3 )
#define		MASSN_AE_BURST1			( 4 )
#define		MASSN_AE_GREN_TOSS		( 7 )
#define		MASSN_AE_CAUGHT_ENEMY	( 10 ) // grunt established sight with an enemy (player only) that had previously eluded the squad.
#define		MASSN_AE_DROP_GUN		( 11 ) // grunt (probably dead) is dropping his mp5.

class CMassn : public CHGrunt
{
public:
	const char* DefaultDisplayName() { return "Male Assassin"; }
	const char* ReverseRelationshipModel() { return "models/massnf.mdl"; }
	void KeyValue(KeyValueData* pkvd);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	bool CanThrowFlashGrenade();
	BOOL CheckRangeAttack2(float flDot, float flDist);
	void Sniperrifle(void);
	void GibMonster();
	void PlayUseSentence();
	void PlayUnUseSentence();
	int	DefaultClassify ( void )
	{
		if (g_modFeatures.blackops_classify)
			return CLASS_HUMAN_BLACKOPS;
		return CHGrunt::DefaultClassify();
	}

	BOOL FOkToSpeak(void);

	void Spawn( void );
	void Precache( void );
	bool IsEnabledInMod() { return g_modFeatures.IsMonsterEnabled("male_assassin"); }
	void MonsterInit();

	void DeathSound(void);
	void PainSound(void);
	void IdleSound(void);

	void TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	void SetHead(int head);

	void DropMyItems(BOOL isGibbed);

	int m_iHead;
};

LINK_ENTITY_TO_CLASS(monster_male_assassin, CMassn)

void CMassn::PlayUseSentence()
{
	SENTENCEG_PlayRndSz( ENT( pev ), "BA_OK", SentenceVolume(), SentenceAttn(), 0, 85 );
	JustSpoke();
}

void CMassn::PlayUnUseSentence()
{
	SENTENCEG_PlayRndSz( ENT( pev ), "BA_WAIT", SentenceVolume(), SentenceAttn(), 0, 85 );
	JustSpoke();
}

BOOL CMassn::FOkToSpeak(void)
{
	return FALSE;
}

void CMassn::IdleSound(void)
{
}

void CMassn::Sniperrifle(void)
{
	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass(vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL);
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_1DEGREES, 2048, BULLET_MONSTER_762, 1);

	pev->effects |= EF_MUZZLEFLASH;

	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CMassn::GibMonster( void )
{
	if( GetBodygroup( MASSN_GUN_GROUP ) != MASSN_GUN_NONE )
	{
		DropMyItems(TRUE);
	}

	CBaseMonster::GibMonster();
}

void CMassn::DropMyItems(BOOL isGibbed)
{
	if (g_pGameRules->FMonsterCanDropWeapons(this) && !FBitSet(pev->spawnflags, SF_MONSTER_DONT_DROP_GUN))
	{
		Vector vecGunPos;
		Vector vecGunAngles;
		GetAttachment( 0, vecGunPos, vecGunAngles );

		if (!isGibbed) {
			SetBodygroup( MASSN_GUN_GROUP, MASSN_GUN_NONE );
		}
		if( FBitSet( pev->weapons, MASSN_SNIPERRIFLE ) ) {
			DropMyItem( "weapon_sniperrifle", vecGunPos, vecGunAngles, isGibbed );
		} else if ( FBitSet( pev->weapons, MASSN_9MMAR ) ) {
			DropMyItem( "weapon_9mmAR", vecGunPos, vecGunAngles, isGibbed );
		}
		if( FBitSet( pev->weapons, MASSN_GRENADELAUNCHER ) ) {
			DropMyItem( "ammo_ARgrenades", isGibbed ? vecGunPos : BodyTarget( pev->origin ), vecGunAngles, isGibbed );
		}
#if FEATURE_MONSTERS_DROP_HANDGRENADES
		if ( FBitSet (pev->weapons, MASSN_HANDGRENADE ) ) {
			CBaseEntity* pGrenadeEnt = DropMyItem( "weapon_handgrenade", BodyTarget( pev->origin ), vecGunAngles, isGibbed );
			if (pGrenadeEnt)
			{
				CBasePlayerWeapon* pGrenadeWeap = pGrenadeEnt->MyWeaponPointer();
				if (pGrenadeWeap)
					pGrenadeWeap->m_iDefaultAmmo = 1;
			}
		}
#endif
	}
	pev->weapons = 0;
}

void CMassn::KeyValue(KeyValueData *pkvd)
{
	if( FStrEq(pkvd->szKeyName, "head" ) )
	{
		m_iHead = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CFollowingMonster::KeyValue( pkvd );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CMassn::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case MASSN_AE_DROP_GUN:
	{
		if(GetBodygroup(MASSN_GUN_GROUP) != MASSN_GUN_NONE)
			DropMyItems(FALSE);
	}
	break;


	case MASSN_AE_BURST1:
	{
		if (FBitSet(pev->weapons, MASSN_9MMAR))
		{
			Shoot();
			PlayFirstBurstSounds();
			CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);
		}
		else if (FBitSet(pev->weapons, MASSN_SNIPERRIFLE))
		{
			Sniperrifle();
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/sniper_fire.wav", 1, 0.4);
			CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 512, 0.3);

			Vector vecGunPos;
			Vector vecGunAngles;
			GetAttachment( 0, vecGunPos, vecGunAngles );

			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecGunPos );
				WRITE_BYTE( TE_ELIGHT );
				WRITE_SHORT( entindex() + 0x1000 );		// entity, attachment
				WRITE_COORD( vecGunPos.x );		// origin
				WRITE_COORD( vecGunPos.y );
				WRITE_COORD( vecGunPos.z );
				WRITE_COORD( 24 );	// radius
				WRITE_BYTE( 255 );	// R
				WRITE_BYTE( 255 );	// G
				WRITE_BYTE( 192 );	// B
				WRITE_BYTE( 3 );	// life * 10
				WRITE_COORD( 0 ); // decay
			MESSAGE_END();
		}

	}
	break;

	case MASSN_AE_KICK:
	{
		PerformKick(gSkillData.massnDmgKick);
	}
	break;

	case MASSN_AE_CAUGHT_ENEMY:
		break;

	case MASSN_AE_GREN_TOSS:
	{
		if (m_pCine)
		{
			CHGrunt::HandleAnimEvent(pEvent);
		}
		else
		{
			bool shouldThrowFgrenade = true;
			if (FBitSet(pev->weapons, MASSN_HANDGRENADE))
			{
				shouldThrowFgrenade = RANDOM_LONG(0,1);
			}
			shouldThrowFgrenade = shouldThrowFgrenade && CanThrowFlashGrenade();
			if (shouldThrowFgrenade)
			{
				CFlashGrenade::ShootTimed( pev, GetGunPosition(), m_vecTossVelocity, 2.8f );

				m_fThrowGrenade = FALSE;
				m_flNextGrenadeCheck = gpGlobals->time + 5;
			}
			else
			{
				CHGrunt::HandleAnimEvent(pEvent);
			}
		}
	}
		break;
	default:
		CHGrunt::HandleAnimEvent(pEvent);
		break;
	}
}

bool CMassn::CanThrowFlashGrenade()
{
	return g_hasFlashbangModel && FBitSet(pev->weapons, MASSN_FLASHGRENADE) && m_hEnemy != 0 && m_hEnemy->IsPlayer();
}

//=========================================================
// CheckRangeAttack2 - this checks the Grunt's grenade
// attack.
//=========================================================
BOOL CMassn::CheckRangeAttack2( float flDot, float flDist )
{
	const bool canThrowHandGrenade = FBitSet(pev->weapons, MASSN_HANDGRENADE);
	const bool canThrowFlashGrenade = CanThrowFlashGrenade();
	const bool canLaunchGrenade = FBitSet(pev->weapons, MASSN_GRENADELAUNCHER);
	if( !canThrowHandGrenade && !canThrowFlashGrenade && !canLaunchGrenade )
	{
		return FALSE;
	}
	return CheckRangeAttack2Impl(gSkillData.massnGrenadeSpeed, flDot, flDist, canLaunchGrenade);
}

//=========================================================
// Spawn
//=========================================================
void CMassn::Spawn()
{
	SpawnHelper("models/massn.mdl", gSkillData.massnHealth);

	if (pev->weapons == 0)
	{
		// initialize to original values
		pev->weapons = MASSN_9MMAR | MASSN_HANDGRENADE;
	}

	if (FBitSet(pev->weapons, MASSN_SNIPERRIFLE))
	{
		SetBodygroup(MASSN_GUN_GROUP, MASSN_GUN_SNIPERRIFLE);
		m_cClipSize = 1;
	}
	else
	{
		m_cClipSize = MASSN_CLIP_SIZE;
	}
	m_cAmmoLoaded = m_cClipSize;

	if (g_hasFlashbangModel && !FBitSet(pev->weapons, MASSN_GRENADELAUNCHER|MASSN_SNIPERRIFLE))
	{
		pev->weapons |= MASSN_FLASHGRENADE;
	}

	if (m_iHead == -1 || m_iHead >= MASSN_HEAD_COUNT) {
		m_iHead = RANDOM_LONG(MASSN_HEAD_WHITE, MASSN_HEAD_BLACK); // never random night googles
	}
	SetBodygroup(MASSN_HEAD_GROUP, m_iHead);

	FollowingMonsterInit();
}

void CMassn::MonsterInit()
{
	CHGrunt::MonsterInit();
	if (FBitSet(pev->weapons, MASSN_SNIPERRIFLE))
	{
		m_flDistTooFar = 2048.0f;
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMassn::Precache()
{
	PrecacheHelper("models/massn.mdl");
	if (g_hasFlashbangModel)
		UTIL_PrecacheOther("fgrenade");

	PRECACHE_SOUND("weapons/sniper_fire.wav");

	m_iBrassShell = PRECACHE_MODEL("models/shell.mdl");// brass shell
}

//=========================================================
// PainSound
//=========================================================
void CMassn::PainSound(void)
{
}

//=========================================================
// DeathSound
//=========================================================
void CMassn::DeathSound(void)
{
}

//=========================================================
// TraceAttack - reimplemented in male assassin because they never have helmets
//=========================================================
void CMassn::TraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CFollowingMonster::TraceAttack(pevInflictor, pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}

void CMassn::SetHead(int head)
{
	m_iHead = head;
}

//=========================================================
// CAssassinRepel - when triggered, spawns a monster_male_assassin
// repelling down a line.
//=========================================================

class CAssassinRepel : public CHGruntRepel
{
public:
	bool IsEnabledInMod() { return g_modFeatures.IsMonsterEnabled("male_assassin"); }
	void KeyValue(KeyValueData* pkvd);
	const char* TrooperName() {
		return "monster_male_assassin";
	}
	void PrepareBeforeSpawn(CBaseEntity* pEntity);

	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	int head;
};

LINK_ENTITY_TO_CLASS(monster_assassin_repel, CAssassinRepel)

TYPEDESCRIPTION	CAssassinRepel::m_SaveData[] =
{
	DEFINE_FIELD( CAssassinRepel, head, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CAssassinRepel, CHGruntRepel )

void CAssassinRepel::KeyValue(KeyValueData *pkvd)
{
	if( FStrEq(pkvd->szKeyName, "head" ) )
	{
		head = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CHGruntRepel::KeyValue( pkvd );
}

void CAssassinRepel::PrepareBeforeSpawn(CBaseEntity *pEntity)
{
	CMassn* massn = (CMassn*)pEntity;
	massn->m_iHead = head;
}

class CDeadMassn : public CDeadMonster
{
public:
	void Spawn( void );
	bool IsEnabledInMod() { return g_modFeatures.IsMonsterEnabled("male_assassin"); }
	int	DefaultClassify ( void )
	{
		if (g_modFeatures.blackops_classify)
			return CLASS_HUMAN_BLACKOPS;
		return CLASS_HUMAN_MILITARY;
	}

	void KeyValue( KeyValueData *pkvd );
	const char* getPos(int pos) const;

	int	m_iHead;
	static const char *m_szPoses[3];
};

const char *CDeadMassn::m_szPoses[] = { "deadstomach", "deadside", "deadsitting" };

const char* CDeadMassn::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

void CDeadMassn::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "head"))
	{
		m_iHead = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CDeadMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_male_assassin_dead, CDeadMassn )
LINK_ENTITY_TO_CLASS( monster_massassin_dead, CDeadMassn )

void CDeadMassn::Spawn( )
{
	SpawnHelper("models/massn.mdl");

	if ( pev->weapons <= 0 )
	{
		SetBodygroup( MASSN_GUN_GROUP, MASSN_GUN_NONE );
	}
	if (FBitSet( pev->weapons, MASSN_9MMAR ))
	{
		SetBodygroup(MASSN_GUN_GROUP, MASSN_GUN_MP5);
	}
	if (FBitSet( pev->weapons, MASSN_SNIPERRIFLE ))
	{
		SetBodygroup(MASSN_GUN_GROUP, MASSN_GUN_SNIPERRIFLE);
	}

	if ( m_iHead < 0 || m_iHead >= MASSN_HEAD_COUNT ) {
		m_iHead = RANDOM_LONG(MASSN_HEAD_WHITE, MASSN_HEAD_BLACK);  // never random night googles
	}

	SetBodygroup( MASSN_HEAD_GROUP, m_iHead );

	MonsterInitDead();
}

#endif
