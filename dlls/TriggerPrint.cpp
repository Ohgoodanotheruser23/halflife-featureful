/**********************************************************************************************
*			trigger_print
*		Print me some text, Gaben.
**********************************************************************************************/

#include "extdll.h"
#include "util.h"
#include "cbase.h"

class CTriggerPrint : public CBaseEntity
{
public:
	void Spawn(void);
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE usetype, float value);

	void KeyValue(KeyValueData* pkvd);

	virtual int Save(CSave& save);
	virtual int Restore(CRestore& restore);

	static TYPEDESCRIPTION m_SaveData[];

	int m_iPrintMode = at_console; // What level of alert I should print?
};

LINK_ENTITY_TO_CLASS(trigger_print, CTriggerPrint);

TYPEDESCRIPTION CTriggerPrint::m_SaveData[] =
{
	DEFINE_FIELD(CTriggerPrint, m_iPrintMode, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CTriggerPrint, CBaseEntity);

void CTriggerPrint::Spawn()
{
	// stub lol
}

void CTriggerPrint::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE usetype, float value)
{
	ALERT((ALERT_TYPE)m_iPrintMode, "%s\n", STRING(pev->message));
}

void CTriggerPrint::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "printMode"))
	{
		m_iPrintMode = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
	{
		return CBaseEntity::KeyValue(pkvd);
	}
}