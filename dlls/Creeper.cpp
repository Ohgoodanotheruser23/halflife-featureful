/*********************************************************************
*		Creeper
*			Aw man
**********************************************************************/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "explode.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define CREEP_AE_ATTACK_RIGHT 0x01

#define CREEP_FLINCH_DELAY 1 // at most one flinch every n secs

class CCreeper : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int Classify(void);
	const char* DefaultDisplayName() { return "Creeper"; }
	void HandleAnimEvent(MonsterEvent_t* pEvent);
	int IgnoreConditions(void);

	float m_flNextFlinch;
	void AlertSound(void);
	void DeathSound(void);
	void PainSound(void);
	static const char* pAlertSounds[];
	static const char* pDeathSounds[];
	static const char* pPainSounds[];

	// No range attacks
	BOOL CheckRangeAttack1(float flDot, float flDist) { return FALSE; }
	BOOL CheckRangeAttack2(float flDot, float flDist) { return FALSE; }
	int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
};

LINK_ENTITY_TO_CLASS(monster_creeper, CCreeper);

const char* CCreeper::pAlertSounds[] =
{
	"creeper/fuse.wav", //ssssssssssssssss
	"creeper/fuse2.wav", //ssssssssssssssss
};

const char* CCreeper::pDeathSounds[] =
{
	"creeper/Creeper_death.wav",
};

const char* CCreeper::pPainSounds[] =
{
	"creeper/Creeper_hurt1.wav",
	"creeper/Creeper_hurt2.wav",
	"creeper/Creeper_hurt3.wav",
	"creeper/Creeper_hurt4.wav",
};

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CCreeper::Classify(void)
{
	return CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CCreeper::SetYawSpeed(void)
{
	int ys;

	ys = 120;

#if 0
	switch (m_Activity)
	{
	}
#endif

	pev->yaw_speed = ys;
}

int CCreeper::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// Take 70% damage from bullets
	if (bitsDamageType == DMG_BULLET)
	{
		Vector vecDir = pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5;
		vecDir = vecDir.Normalize();
		float flForce = DamageForce(flDamage);
		pev->velocity = pev->velocity + vecDir * flForce;
		flDamage *= 0.7;
	}

	// HACK HACK -- until we fix this.
	if (IsAlive())
		PainSound();
	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

void CCreeper::AlertSound(void)
{
	int pitch = 95 + RANDOM_LONG(0, 9);
	// Play a random attack sound
	EmitSoundDyn(CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), 1.0, ATTN_NORM, 0, pitch);
}

void CCreeper::PainSound(void)
{
	int pitch = 95 + RANDOM_LONG(0, 9);

	if (RANDOM_LONG(0, 5) < 2)
		EmitSoundDyn(CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), 1.0, ATTN_NORM, 0, pitch);
}

void CCreeper::DeathSound(void)
{
	int pitch = 95 + RANDOM_LONG(0, 9);

	// Play a random attack sound
	EmitSoundDyn(CHAN_VOICE, RANDOM_SOUND_ARRAY(pDeathSounds), 1.0, ATTN_NORM, 0, pitch);
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CCreeper::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case CREEP_AE_ATTACK_RIGHT:
	{
		pev->health = 1;											// Hack because Creeper has an insane amount of health, this lasts for .1 seconds
		ExplosionCreate(Center(), pev->angles, edict(), 128, TRUE); // BOOM!
	}
	break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CCreeper::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/creeper.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.creeperHealth;
	pev->view_ofs = VEC_VIEW; // position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.5;	  // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CCreeper::Precache()
{
	int i;

	PRECACHE_MODEL("models/creeper.mdl");

	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================



int CCreeper::IgnoreConditions(void)
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK1))
	{
#if 0
		if (pev->health < 20)
			iIgnore |= (bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE);
		else
#endif
			if (m_flNextFlinch >= gpGlobals->time)
				iIgnore |= (bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE);
	}

	if ((m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH))
	{
		if (m_flNextFlinch < gpGlobals->time)
			m_flNextFlinch = gpGlobals->time + CREEP_FLINCH_DELAY;
	}

	return iIgnore;
}
