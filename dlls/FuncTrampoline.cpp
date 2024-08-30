/*
================================================================
	Func_Trampoline
		Boing
================================================================
*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "explode.h"

// TODO: Add keyvalues for sounds

class CFuncTrampoline : public CBaseEntity
{
	void Spawn() override;
	void Touch(CBaseEntity* pOther) override;

	void KeyValue(KeyValueData* pkvd) override;

	void Precache();

	int Save(CSave& save);
	int Restore(CRestore& restore);

	static TYPEDESCRIPTION m_SaveData[];

	bouncesound_t m_bs;

	string_t m_bounceSound;			// BOING

	int m_iCanExplode = 0;
};

LINK_ENTITY_TO_CLASS(func_trampoline, CFuncTrampoline);

TYPEDESCRIPTION CFuncTrampoline::m_SaveData[] =
{
	DEFINE_FIELD(CFuncTrampoline, m_iCanExplode, FIELD_INTEGER),
	DEFINE_FIELD(CFuncTrampoline, m_bounceSound, FIELD_CHARACTER) };

IMPLEMENT_SAVERESTORE(CFuncTrampoline, CBaseEntity);

void CFuncTrampoline::Spawn()
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_TRIGGER;
	SET_MODEL(edict(), STRING(pev->model));
}

void PlayBounceSounds(entvars_t* pev, bouncesound_t* pls)
{
	float fvol = 0.7;
	float flsoundwait = 3;

	int fplaysound = (pls->sBounceSound && gpGlobals->time > pls->flwaitSound);

	if (fplaysound)
	{
		EMIT_SOUND(ENT(pev), CHAN_ITEM, STRING(pls->sBounceSound), fvol, ATTN_NORM);
		pls->flwaitSound = gpGlobals->time + flsoundwait;
	}
}

void CFuncTrampoline::Precache()
{
	const char* pszSound;
	//BOOL NullSound = FALSE;

	if (!FStringNull(m_bounceSound))
	{
		pszSound = STRING(m_bounceSound);
		PRECACHE_SOUND(pszSound);
		m_bs.sBounceSound = m_bounceSound;
	}
}

void CFuncTrampoline::Touch(CBaseEntity* pOther)
{
	if (m_iCanExplode == 1)
	{
		pOther->pev->velocity.z += 400.0f;

		Vector position = pOther->pev->origin + (pOther->pev->maxs + pOther->pev->mins) * 0.5f;
		ExplosionCreate(position, pev->angles, edict(), 50, true);
	}
	else
		pOther->pev->velocity.z += 400.0f;
		PlayBounceSounds(pev, &m_bs);
}

void CFuncTrampoline::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "canExplode"))
	{
		m_iCanExplode = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq(pkvd->szKeyName, "bounceSound"))
	{
		m_bounceSound = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
	{
		return CBaseEntity::KeyValue(pkvd);
	}
}