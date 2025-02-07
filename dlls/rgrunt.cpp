#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"weapons.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"decals.h"
#include	"hgrunt.h"
#include	"mod_features.h"
#include	"game.h"

#if FEATURE_ROBOGRUNT

#define	GRUNT_CLIP_SIZE					36

#define HGRUNT_9MMAR				( 1 << 0)
#define HGRUNT_HANDGRENADE			( 1 << 1)
#define HGRUNT_GRENADELAUNCHER			( 1 << 2)
#define HGRUNT_SHOTGUN				( 1 << 3)

#define GUN_GROUP					2
#define GUN_MP5						0
#define GUN_SHOTGUN					1
#define GUN_NONE					2

class CRGrunt : public CHGrunt
{
public:
	void Spawn();
	void Precache();
	bool IsEnabledInMod() { return g_modFeatures.IsMonsterEnabled("robogrunt"); }
	int DefaultClassify() { return CLASS_MACHINE; }
	const char* DefaultDisplayName() { return "Robo Grunt"; }
	const char* ReverseRelationshipModel() { return "models/rgruntf.mdl"; }
	void RunAI();
	void StartTask( Task_t* pTask );
	void RunTask( Task_t* pTask );
	void EXPORT Spark();
	void EXPORT Explode();

	void PlayUseSentence();
	void PlayUnUseSentence();

	void DeathSound(void);
	void PainSound(void);

	const char* DefaultGibModel() {
		return "models/computergibs.mdl";
	}

	void Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib );
	void BecomeDead();
	void TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	float m_flSparkTime;

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

protected:
	static const char *pRoboSentences[HGRUNT_SENT_COUNT];
	virtual const char* SentenceByNumber(int sentence);
	bool AlertSentenceIsForPlayerOnly() {
		return false;
	}

	void DoSpark(const Vector& sparkLocation, float flVolume);
};

LINK_ENTITY_TO_CLASS(monster_robogrunt, CRGrunt)

TYPEDESCRIPTION CRGrunt::m_SaveData[] =
{
	DEFINE_FIELD( CRGrunt, m_flSparkTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE( CRGrunt, CHGrunt )

const char *CRGrunt::pRoboSentences[] =
{
	"RB_GREN", // grenade scared grunt
	"RB_ALERT", // sees player
	"RB_MONST", // sees monster
	"RB_COVER", // running to cover
	"RB_THROW", // about to throw grenade
	"RB_CHARGE",  // running out to get the enemy
	"RB_TAUNT", // say rude things
	"RB_CHECK",
	"RB_QUEST",
	"RB_IDLE",
	"RB_CLEAR",
	"RB_ANSWER",
	"RB_HOSTILE",
};

const char* CRGrunt::SentenceByNumber(int sentence)
{
	return pRoboSentences[sentence];
}

void CRGrunt::Spawn()
{
	SpawnHelper("models/rgrunt.mdl", gSkillData.hgruntHealth, DONT_BLEED);
	if( pev->weapons == 0 )
	{
		pev->weapons = HGRUNT_9MMAR | HGRUNT_HANDGRENADE;
	}

	if( FBitSet( pev->weapons, HGRUNT_SHOTGUN ) )
	{
		SetBodygroup( GUN_GROUP, GUN_SHOTGUN );
		m_cClipSize = 8;
	}
	else
	{
		m_cClipSize = GRUNT_CLIP_SIZE;
	}
	m_cAmmoLoaded = m_cClipSize;

	CTalkMonster::g_talkWaitTime = 0;

	FollowingMonsterInit();
}

void CRGrunt::Precache()
{
	PrecacheHelper("models/rgrunt.mdl");
	PRECACHE_MODEL("models/computergibs.mdl");
	PRECACHE_SOUND( "turret/tu_die.wav" );
	PRECACHE_SOUND( "turret/tu_die2.wav" );
	PRECACHE_SOUND( "turret/tu_die3.wav" );

	PRECACHE_SOUND( "buttons/spark1.wav" );
	PRECACHE_SOUND( "buttons/spark2.wav" );
	PRECACHE_SOUND( "buttons/spark3.wav" );
	PRECACHE_SOUND( "buttons/spark4.wav" );
	PRECACHE_SOUND( "buttons/spark5.wav" );
	PRECACHE_SOUND( "buttons/spark6.wav" );

	PRECACHE_SOUND( "buttons/button2.wav" );
	PRECACHE_SOUND( "buttons/button3.wav" );

	m_voicePitch = 115;

	m_iBrassShell = PRECACHE_MODEL( "models/shell.mdl" );// brass shell
	m_iShotgunShell = PRECACHE_MODEL( "models/shotgunshell.mdl" );
}

void CRGrunt::PlayUseSentence()
{
	EmitSound( CHAN_VOICE, "buttons/button3.wav", SentenceVolume(), SentenceAttn() );
	JustSpoke();
}

void CRGrunt::PlayUnUseSentence()
{
	EmitSound( CHAN_VOICE, "buttons/button2.wav", SentenceVolume(), SentenceAttn() );
	JustSpoke();
}

void CRGrunt::DeathSound()
{
	switch (RANDOM_LONG(0,2))
	{
	case 0:
		EmitSound( CHAN_VOICE, "turret/tu_die.wav", 1.0, ATTN_NORM );
		break;
	case 1:
		EmitSound( CHAN_VOICE, "turret/tu_die2.wav", 1.0, ATTN_NORM );
		break;
	case 2:
		EmitSound( CHAN_VOICE, "turret/tu_die3.wav", 1.0, ATTN_NORM );
		break;
	}
}

void CRGrunt::PainSound()
{
}

void CRGrunt::RunAI()
{
	if (pev->health <= LimpHealth())
	{
		if (m_flSparkTime <= gpGlobals->time)
		{
			m_flSparkTime += RANDOM_FLOAT(0.2, 0.5);

			Vector sparkLocation = Center();

			sparkLocation.x += RANDOM_FLOAT(-pev->size.x, pev->size.x) * 0.3;
			sparkLocation.y += RANDOM_FLOAT(-pev->size.y, pev->size.y) * 0.3;
			sparkLocation.z += RANDOM_FLOAT(-pev->size.z, pev->size.z) * 0.45;

			DoSpark(sparkLocation, RANDOM_FLOAT( 0.2 , 0.5 ));
		}
	}
	CHGrunt::RunAI();
}

void CRGrunt::StartTask(Task_t *pTask)
{
	switch(pTask->iTask)
	{
	case TASK_DIE:
	{
		CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin, 400, 2 );

		if( UTIL_PointContents( pev->origin ) == CONTENTS_WATER )
		{
			UTIL_Bubbles( pev->origin, pev->origin + Vector( 64, 64, 64 ), 100 );
		}
		else
		{
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_SMOKE );
				WRITE_COORD( pev->origin.x );
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z );
				WRITE_SHORT( g_sModelIndexSmoke );
				WRITE_BYTE( 25 ); // scale * 10
				WRITE_BYTE( 10 ); // framerate
			MESSAGE_END();
		}
	}
	default:
		CHGrunt::StartTask(pTask);
		break;
	}
}

void CRGrunt::RunTask(Task_t *pTask)
{
	switch(pTask->iTask)
	{
	case TASK_DIE:
	{
		if( m_fSequenceFinished && pev->frame >= 255 )
		{
			pev->deadflag = DEAD_DEAD;
			SetThink(&CRGrunt::Spark);
			pev->nextthink = gpGlobals->time;
			StopAnimation();
			TaskComplete();
		}
	}
		break;
	default:
		CHGrunt::RunTask(pTask);
		break;
	}
}

#define ROBOGRUNT_SPARKS 10
void CRGrunt::Spark()
{
	Vector sparkLocation = pev->origin;

	sparkLocation.z += RANDOM_FLOAT(0,4);
	sparkLocation.x += RANDOM_FLOAT(-pev->size.x, pev->size.x) * 0.3;
	sparkLocation.y += RANDOM_FLOAT(-pev->size.y, pev->size.y) * 0.3;

	pev->button++;
	DoSpark(sparkLocation, 0.5 + pev->button * (0.5 / ROBOGRUNT_SPARKS) );
	pev->nextthink = gpGlobals->time + 0.2;
	if (pev->button >= ROBOGRUNT_SPARKS)
	{
		SetThink(&CRGrunt::Explode);
	}
}

void CRGrunt::Explode()
{
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, -32 ), ignore_monsters, ENT( pev ), & tr );

	pev->dmg = gSkillData.rgruntExplode;
	int iContents = UTIL_PointContents( pev->origin );

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
		WRITE_COORD( pev->origin.x );	// Send to PAS because of the sound
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		if( iContents != CONTENTS_WATER )
		{
			WRITE_SHORT( g_sModelIndexFireball );
		}
		else
		{
			WRITE_SHORT( g_sModelIndexWExplosion );
		}
		WRITE_BYTE( ( pev->dmg - 50 ) * .60  ); // scale * 10
		WRITE_BYTE( 15 ); // framerate
		WRITE_BYTE( TE_EXPLFLAG_NONE );
	MESSAGE_END();

	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0 );

	RadiusDamage( pev, pev, pev->dmg, CLASS_NONE, DMG_BLAST );

	if( RANDOM_LONG(0,1) )
	{
		UTIL_DecalTrace( &tr, DECAL_SCORCH1 );
	}
	else
	{
		UTIL_DecalTrace( &tr, DECAL_SCORCH2 );
	}

	CGib::SpawnRandomGibs( pev, GibCount(), GibModel() );

	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

void CRGrunt::Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevInflictor, pevAttacker, GIB_NEVER );
}

void CRGrunt::BecomeDead()
{
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_TOSS;
}

#define ROBOGRUNT_DAMAGE (DMG_ENERGYBEAM|DMG_CRUSH|DMG_MORTAR|DMG_BLAST|DMG_SHOCK|DMG_FREEZE|DMG_ACID)

void CRGrunt::TraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if ((bitsDamageType & ROBOGRUNT_DAMAGE) == 0)
	{
		if( pev->dmgtime != gpGlobals->time || (RANDOM_LONG( 0, 10 ) < 1 ) )
		{
			UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT( 1, 2 ) );
			pev->dmgtime = gpGlobals->time;
		}
		flDamage *= 0.2;
	}
	CSquadMonster::TraceAttack( pevInflictor, pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

void CRGrunt::DoSpark(const Vector &sparkLocation, float flVolume)
{
	UTIL_Sparks( sparkLocation );

	switch( RANDOM_LONG( 0, 5 ) )
	{
		case 0:
			EmitSound( CHAN_BODY, "buttons/spark1.wav", flVolume, ATTN_NORM );
			break;
		case 1:
			EmitSound( CHAN_BODY, "buttons/spark2.wav", flVolume, ATTN_NORM );
			break;
		case 2:
			EmitSound( CHAN_BODY, "buttons/spark3.wav", flVolume, ATTN_NORM );
			break;
		case 3:
			EmitSound( CHAN_BODY, "buttons/spark4.wav", flVolume, ATTN_NORM );
			break;
		case 4:
			EmitSound( CHAN_BODY, "buttons/spark5.wav", flVolume, ATTN_NORM );
			break;
		case 5:
			EmitSound( CHAN_BODY, "buttons/spark6.wav", flVolume, ATTN_NORM );
			break;
	}
}

class CRGruntRepel : public CHGruntRepel
{
public:
	const char* TrooperName() {
		return "monster_robogrunt";
	}
};

LINK_ENTITY_TO_CLASS(monster_robogrunt_repel, CRGruntRepel)

#endif
