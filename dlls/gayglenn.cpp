/***
* Gay Glenn.cpp
* Gay Glenn is actually not gay.
****/

// UNDONE: Don't flinch every time you get hit

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "gamerules.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define GLENN_AE_ATTACK_RIGHT 0x01
#define GLENN_AE_ATTACK_LEFT 0x02
#define GLENN_AE_ATTACK_BOTH 0x03

#define GLENN_FLINCH_DELAY 2 // at most one flinch every n secs

class CGay : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int Classify(void);
	void HandleAnimEvent(MonsterEvent_t* pEvent);
	int IgnoreConditions(void);

	float m_flNextFlinch;

	void AlertSound(void);
	void IdleSound(void);
	void AttackSound(void);

	static const char* pAttackSounds[];
	static const char* pIdleSounds[];
	static const char* pAlertSounds[];
	static const char* pPainSounds[];
	static const char* pAttackHitSounds[];
	static const char* pAttackMissSounds[];

	// No range attacks
	BOOL CheckRangeAttack1(float flDot, float flDist) { return FALSE; }
	BOOL CheckRangeAttack2(float flDot, float flDist) { return FALSE; }
	int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
};

LINK_ENTITY_TO_CLASS(monster_gayglenn, CGay);
LINK_ENTITY_TO_CLASS(monster_gay, CGay);  // Compatability hack, perhaps?

const char* CGay::pAttackHitSounds[] =
{
	"glenn/strike1.wav",
	"weapons/cbar_hitbod1.wav",
	"weapons/cbar_hitbod2.wav",
	"bonus_shit/fart2.wav",
	"scientist/sci_pain3.wav",
};

const char* CGay::pAttackMissSounds[] =
{
	"glenn/miss1.wav",
	"hassault/hw_shoot1.wav",
};

const char* CGay::pAttackSounds[] =
{
	"glenn/gg_attack1.wav",
	"glenn/gg_attack2.wav",
};

const char* CGay::pIdleSounds[] =
{
	"glenn/gg_idle1.wav",
	"glenn/gg_idle2.wav",
	"glenn/gg_idle3.wav",
	"glenn/gg_idle4.wav",
};

const char* CGay::pAlertSounds[] =
{
	"glenn/gg_alert10.wav",
	"glenn/gg_alert20.wav",
	"glenn/gg_alert30.wav",
};

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CGay::Classify(void)
{
	return CLASS_GAYGLENN;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CGay::SetYawSpeed(void)
{
	int ys;

	ys = 360;

#if 0
	switch (m_Activity)
	{
	}
#endif

	pev->yaw_speed = ys;
}

int CGay::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// HACK HACK -- until we fix this.
	if (IsAlive())
		PainSound();
	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

void CGay::AlertSound(void)
{
	int pitch = 95 + RANDOM_LONG(0, 9);

	EmitSoundDyn(CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), 1.0, ATTN_NORM, 0, pitch);
}

void CGay::IdleSound(void)
{
	int pitch = 95 + RANDOM_LONG(0, 9);

	// Play a random idle sound
	EmitSoundDyn(CHAN_VOICE, RANDOM_SOUND_ARRAY(pIdleSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
}

void CGay::AttackSound(void)
{
	// Play a random attack sound
	EmitSoundDyn(CHAN_VOICE, RANDOM_SOUND_ARRAY(pAttackSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGay::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case GLENN_AE_ATTACK_RIGHT:
	{
		// do stuff for this event.
		//		ALERT( at_console, "Slash right!\n" );
		CBaseEntity* pHurt = CheckTraceHullAttack(70, gSkillData.glennDmgOneSlash, DMG_SLASH);
		if (pHurt)
		{
			if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
			{
				pHurt->pev->punchangle.z = -18;
				pHurt->pev->punchangle.x = 5;
				pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
			}
			// Play a random attack hit sound
			EmitSoundDyn(CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackHitSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
		else // Play a random attack miss sound
			EmitSoundDyn(CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackMissSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

		if (RANDOM_LONG(0, 1))
			AttackSound();
	}
	break;

	case GLENN_AE_ATTACK_LEFT:
	{
		// do stuff for this event.
		//		ALERT( at_console, "Slash left!\n" );
		CBaseEntity* pHurt = CheckTraceHullAttack(70, gSkillData.glennDmgOneSlash, DMG_SLASH);
		if (pHurt)
		{
			if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
			{
				pHurt->pev->punchangle.z = 18;
				pHurt->pev->punchangle.x = 5;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 100;
			}
			EmitSoundDyn(CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackHitSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
		else
			EmitSoundDyn(CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackMissSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

		if (RANDOM_LONG(0, 1))
			AttackSound();
	}
	break;

	case GLENN_AE_ATTACK_BOTH:
	{
		// do stuff for this event.
		CBaseEntity* pHurt = CheckTraceHullAttack(70, gSkillData.glennDmgBothSlash, DMG_SLASH);
		if (pHurt)
		{
			if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
			{
				pHurt->pev->punchangle.x = 5;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -100;
			}
			EmitSoundDyn(CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackHitSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
		else
			EmitSoundDyn(CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackMissSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

		if (RANDOM_LONG(0, 1))
			AttackSound();
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
void CGay::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/chris.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.glennHealth;
	pev->view_ofs = VEC_VIEW; // position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.5;	  // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP | bits_CAP_AUTO_DOORS | bits_CAP_SQUAD;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGay::Precache()
{
	int i;

	PRECACHE_MODEL("models/chris.mdl");

	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pAttackMissSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================



int CGay::IgnoreConditions(void)
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
			m_flNextFlinch = gpGlobals->time + GLENN_FLINCH_DELAY;
	}

	return iIgnore;
}