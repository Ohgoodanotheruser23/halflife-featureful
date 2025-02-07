#ifndef SHOCKBEAM_H
#define SHOCKBEAM_H

#include "mod_features.h"
#include "cbase.h"

#if FEATURE_SHOCKBEAM

class CBeam;
class CSprite;

//=========================================================
// Shockrifle projectile
//=========================================================
class CShock : public CBaseAnimating
{
public:
	void Spawn(void);
	void Precache();

	static void Shoot(entvars_t *pevOwner, const Vector angles, const Vector vecStart, const Vector vecVelocity, string_t soundList = iStringNull);
	static float ShockSpeed() { return 2000.0f; }
	void Touch(CBaseEntity *pOther);
	void EXPORT FlyThink();

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	void CreateEffects();
	void ClearEffects();
	void UpdateOnRemove();

	CBeam *m_pBeam;
	CBeam *m_pNoise;
	CSprite *m_pSprite;
};
#endif
#endif
