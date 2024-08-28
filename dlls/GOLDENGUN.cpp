/*
===========================================================================================
		WEAPON_GOLDENGUN
	Sorry, Slappers Only has been banned
===========================================================================================
*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#ifndef CLIENT_DLL
#include "game.h"
#endif

#if FEATURE_GOLDENGUN

LINK_ENTITY_TO_CLASS(weapon_goldengun, CGOLDENGUN);

void CGOLDENGUN::Spawn()
{
	pev->classname = MAKE_STRING("weapon_goldengun");
	Precache();
	m_iId = WEAPON_GOLDENGUN;
	SET_MODEL(ENT(pev), MyWModel());

	InitDefaultAmmo(GOLDENGUN_MAX_DEFAULT_GIVE);

	m_iDefaultAmmo = GOLDENGUN_MAX_DEFAULT_GIVE;
	FallInit();
}

void CGOLDENGUN::Precache(void)
{
	PRECACHE_MODEL("models/v_goldengun.mdl");
	PRECACHE_MODEL(MyWModel());
	PrecachePModel("models/p_goldengun.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl");

	//PRECACHE_SOUND("items/goldengunclip.wav");
	PRECACHE_SOUND("weapons/ggun_fire.wav");

	m_usFireGOLDENGUN = PRECACHE_EVENT(1, "events/goldengun.sc");
}

bool CGOLDENGUN::IsEnabledInMod()
{
#ifndef CLIENT_DLL
	return g_modFeatures.IsWeaponEnabled(WEAPON_GOLDENGUN);
#else
	return true;
#endif
}

int CGOLDENGUN::AddToPlayer(CBasePlayer* pPlayer)
{
	return AddToPlayerDefault(pPlayer);
}

int CGOLDENGUN::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "gold";
	//  p->pszAmmo2 = "Nothing";
	//  p->iMaxAmmo2 = 0;
	p->iMaxClip = GOLDENGUN_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 4;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_GOLDENGUN;
	p->iWeight = GOLDENGUN_WEIGHT;
	p->pszAmmoEntity = "weapon_goldengun";
	p->iDropAmmo = AMMO_GOLDENGUNCLIP_GIVE;

	return 1;
}

BOOL CGOLDENGUN::Deploy()
{
	TriggerReleased = FALSE;

	return DefaultDeploy("models/v_goldengun.mdl", "models/p_goldengun.mdl", GOLDENGUN_DEPLOY, "goldengun", 0);
}

void CGOLDENGUN::SecondaryAttack(void)
{
	//spaceholder
}

void CGOLDENGUN::PrimaryAttack(void)
{
	if (!TriggerReleased)
		return;

	GOLDENGUNFire(0.015, (60 / 600), FALSE);

	TriggerReleased = FALSE;
}

void CGOLDENGUN::GOLDENGUNFire(float flSpread, float flCycleTime, BOOL fUseAutoAim)
{
	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = GetNextAttackDelay(0.2f);
		}
		return;
	}

	m_iClip--;
	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;
	int flags;

#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), fUseAutoAim ? m_usFireGOLDENGUN : m_usFireGOLDENGUN, 0.0, (float*)&g_vecZero, (float*)&g_vecZero, 0.0, 0.0, 0, 0, (m_iClip == 0) ? 1 : 0, 0);

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	// silenced
	if (pev->body == 1)
	{
		m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
	}
	else
	{
		// non-silenced
		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	}

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming;

	if (fUseAutoAim)
	{
		vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);
	}
	else
	{
		vecAiming = gpGlobals->v_forward;
	}

	m_pPlayer->FireBullets(1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_GOLDENGUN, 2);
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);

	if (m_pPlayer->pev->flags & FL_DUCKING)
	{
		switch (RANDOM_LONG(0, 1))
		{
		case 0: m_pPlayer->pev->punchangle.y -= 3; break;
		case 1: m_pPlayer->pev->punchangle.y += 3; break;
		}
	}
	else if (m_pPlayer->pev->velocity.Length() > .01)
	{
		switch (RANDOM_LONG(0, 1))
		{
		case 0: m_pPlayer->pev->punchangle.y -= 3; break;
		case 1: m_pPlayer->pev->punchangle.y += 3; break;
		}
	}
	else
	{
		switch (RANDOM_LONG(0, 1))
		{
		case 0: m_pPlayer->pev->punchangle.y -= 3; break;
		case 1: m_pPlayer->pev->punchangle.y += 3; break;
		}
	}
}

void CGOLDENGUN::Reload(void)
{
	int iResult;

	if (m_iClip == GOLDENGUN_MAX_CLIP)
		return;

	if (m_iClip == 0)
	{
		iResult = DefaultReload(GOLDENGUN_MAX_CLIP, GOLDENGUN_RELOAD2, 1.5, 0);
	}
	else
	{
		iResult = DefaultReload(GOLDENGUN_MAX_CLIP, GOLDENGUN_RELOAD2, 1.5, 0);
	}
	if (iResult)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
	}
}

void CGOLDENGUN::WeaponIdle(void)
{
	TriggerReleased = TRUE;

	ResetEmptySound();
	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (m_iClip != 0)
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0.0, 1.0);

		if (flRand <= 0.3 + 0 * 0.75)
		{
			iAnim = GOLDENGUN_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1 / 16;
		}
		else if (flRand <= 0.6 + 0 * 0.875)
		{
			iAnim = GOLDENGUN_LONGIDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1 / 16;
		}
		else
		{
			iAnim = GOLDENGUN_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1 / 16;
		}
		SendWeaponAnim(iAnim, 1);
	}
}

#endif