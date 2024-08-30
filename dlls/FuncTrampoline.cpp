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

	int Save(CSave& save);
	int Restore(CRestore& restore);

	static TYPEDESCRIPTION m_SaveData[];

	int m_iCanExplode = 0;
	bool bCanExplode = false;
};

LINK_ENTITY_TO_CLASS(func_trampoline, CFuncTrampoline);

TYPEDESCRIPTION CFuncTrampoline::m_SaveData[] =
{
	DEFINE_FIELD(CFuncTrampoline, m_iCanExplode, FIELD_INTEGER),
	DEFINE_FIELD(CFuncTrampoline, bCanExplode, FIELD_BOOLEAN) };

IMPLEMENT_SAVERESTORE(CFuncTrampoline, CBaseEntity);

void CFuncTrampoline::Spawn()
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_TRIGGER;
	SET_MODEL(edict(), STRING(pev->model));
}

void CFuncTrampoline::Touch(CBaseEntity* pOther)
{
	if (bCanExplode)
	{
		pOther->pev->velocity.z += 400.0f;

		Vector position = pOther->pev->origin + (pOther->pev->maxs + pOther->pev->mins) * 0.5f;
		ExplosionCreate(position, pev->angles, edict(), 50, true);
	}
	else
		pOther->pev->velocity.z += 400.0f;
}

void CFuncTrampoline::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "canExplode"))
	{
		m_iCanExplode = atoi(pkvd->szValue);
		bCanExplode = true;

		pkvd->fHandled = true;
	}
	else
	{
		return CBaseEntity::KeyValue(pkvd);
	}
}