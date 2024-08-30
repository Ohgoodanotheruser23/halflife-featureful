/*
=========================================================
	custom spark
=========================================================
*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"

class CEnvSparkCustom : public CBaseEntity
{
public:
	void Think() override;
	void Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value ) override;
private:
	bool canSpark = false;
};

LINK_ENTITY_TO_CLASS( env_spark_custom, CEnvSparkCustom );

void CEnvSparkCustom::Think()
{
	if (canSpark)
	{
		UTIL_Sparks(pev->origin);
	}

	pev->nextthink = gpGlobals->time + pev->speed;
}

void CEnvSparkCustom::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	canSpark = !canSpark;
	pev->nextthink = gpGlobals->time;
}