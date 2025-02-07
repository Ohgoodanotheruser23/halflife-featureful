/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"
#include "shake.h"

#ifndef CLIENT_DLL
#include "game.h"
#include "displacerball.h"
#endif

#if FEATURE_DISPLACER

#ifndef CLIENT_DLL
extern edict_t *EntSelectSpawnPoint( CBaseEntity *pPlayer );
#endif // !defined ( CLIENT_DLL )

#define DISPLACER_SECONDARY_USAGE 60
#define DISPLACER_PRIMARY_USAGE 20

LINK_ENTITY_TO_CLASS(weapon_displacer, CDisplacer)

int CDisplacer::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "uranium";
	p->pszAmmo2 = NULL;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iFlags = 0;
#if FEATURE_OPFOR_WEAPON_SLOTS
	p->iSlot = 5;
	p->iPosition = 1;
#else
	p->iSlot = 3;
	p->iPosition = 6;
#endif
	p->iId = WEAPON_DISPLACER;
	p->iWeight = DISPLACER_WEIGHT;
	p->pszAmmoEntity = "ammo_gaussclip";
	p->iDropAmmo = AMMO_URANIUMBOX_GIVE;

	return 1;
}

int CDisplacer::AddToPlayer(CBasePlayer *pPlayer)
{
	return AddToPlayerDefault(pPlayer);
}

BOOL CDisplacer::PlayEmptySound(void)
{
	if (m_iPlayEmptySound)
	{
		EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "buttons/button11.wav", 0.9f, ATTN_NORM);
		m_iPlayEmptySound = 0;
		return 0;
	}
	return 0;
}

void CDisplacer::Spawn()
{
	Precache();
	m_iId = WEAPON_DISPLACER;
	SET_MODEL(ENT(pev), MyWModel());

	InitDefaultAmmo(DISPLACER_DEFAULT_GIVE);

	FallInit();// get ready to fall down.
}

void CDisplacer::Precache(void)
{
	PRECACHE_MODEL("models/v_displacer.mdl");
	PRECACHE_MODEL(MyWModel());
	PrecachePModel("models/p_displacer.mdl");

	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/displacer_fire.wav");
	PRECACHE_SOUND("weapons/displacer_self.wav");
	PRECACHE_SOUND("weapons/displacer_spin.wav");
	PRECACHE_SOUND("weapons/displacer_spin2.wav");

	PRECACHE_SOUND("buttons/button11.wav");

	PRECACHE_MODEL("sprites/lgtning.spr");

	UTIL_PrecacheOther("displacer_ball");

	precacheGunPickupSound();
	precacheAmmoPickupSound();

	m_usDisplacer = PRECACHE_EVENT(1, "events/displacer.sc");
}

bool CDisplacer::IsEnabledInMod()
{
#ifndef CLIENT_DLL
	return g_modFeatures.IsWeaponEnabled(WEAPON_DISPLACER);
#else
	return true;
#endif
}

BOOL CDisplacer::Deploy()
{
	return DefaultDeploy("models/v_displacer.mdl", "models/p_displacer.mdl", DISPLACER_DRAW, "egon");
}

void CDisplacer::Holster()
{
	m_fInReload = FALSE;// cancel any reload in progress.

	ClearBeams();
	ClearSpin();

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
	SendWeaponAnim(DISPLACER_HOLSTER);
}

void CDisplacer::SecondaryAttack(void)
{
	if (m_fFireOnEmpty || !CanFireDisplacer(DISPLACER_SECONDARY_USAGE))
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
		return;
	}

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_iFireMode = FIREMODE_BACKWARD;

	SetThink (&CDisplacer::SpinUp);

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 4.0;
	pev->nextthink = gpGlobals->time;
}

void CDisplacer::PrimaryAttack()
{
	if ( m_fFireOnEmpty || !CanFireDisplacer(DISPLACER_PRIMARY_USAGE))
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
		return;
	}
	m_iFireMode = FIREMODE_FORWARD;

	SetThink (&CDisplacer::SpinUp);
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.6;
	pev->nextthink = gpGlobals->time;
}

void CDisplacer::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_2DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
	if (flRand <= 0.5)
	{
		iAnim = DISPLACER_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;
	}
	else
	{
		iAnim = DISPLACER_IDLE2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;
	}

	SendWeaponAnim(iAnim);
}

//=========================================================
// Purpose:
//=========================================================
void CDisplacer::ClearSpin( void )
{

	switch (m_iFireMode)
	{
	case FIREMODE_FORWARD:
		STOP_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/displacer_spin.wav");
		break;
	case FIREMODE_BACKWARD:
		STOP_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/displacer_spin2.wav");
		break;
	}
}

//=========================================================
// Purpose:
//=========================================================
void CDisplacer::SpinUp( void )
{
	SendWeaponAnim( DISPLACER_SPINUP );

	LightningEffect();

	if( m_iFireMode == FIREMODE_FORWARD )
	{
		EMIT_SOUND( m_pPlayer->edict(), CHAN_WEAPON, "weapons/displacer_spin.wav", 1, ATTN_NORM );
		SetThink (&CDisplacer::Displace);
	}
	else
	{
		EMIT_SOUND( m_pPlayer->edict(), CHAN_WEAPON, "weapons/displacer_spin2.wav", 1, ATTN_NORM );
		SetThink (&CDisplacer::Teleport);
	}
	pev->nextthink = gpGlobals->time + 0.9;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.3;
}

//=========================================================
// Purpose:
//=========================================================
void CDisplacer::Displace( void )
{
	ClearBeams();
	ClearSpin();

	SendWeaponAnim( DISPLACER_FIRE );
	EMIT_SOUND( edict(), CHAN_WEAPON, "weapons/displacer_fire.wav", 1, ATTN_NORM );

	// player "shoot" animation
        m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_pPlayer->pev->punchangle.x -= 2;
#ifndef CLIENT_DLL
	Vector vecSrc;
	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= DISPLACER_PRIMARY_USAGE;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);

	vecSrc = m_pPlayer->GetGunPosition();
	vecSrc = vecSrc + gpGlobals->v_forward	* 22;
	vecSrc = vecSrc + gpGlobals->v_right	* 8;
	vecSrc = vecSrc + gpGlobals->v_up		* -12;

	CDisplacerBall::Shoot( m_pPlayer->pev, vecSrc, gpGlobals->v_forward * CDisplacerBall::BallSpeed(), m_pPlayer->pev->v_angle );

	SetThink( NULL );
#endif
}

//=========================================================
// Purpose:
//=========================================================
void CDisplacer::Teleport( void )
{
	ClearBeams();
	ClearSpin();
#ifndef CLIENT_DLL
	CBaseEntity *pTarget = NULL;
	Vector tmp( 0, 0, 0 );

	if( g_pGameRules->IsMultiplayer() && !g_pGameRules->IsCoOp() )
	{
		pTarget = GetClassPtr( (CBaseEntity *)VARS( EntSelectSpawnPoint( m_pPlayer ) ) );
	}
	else
	{
		const char *pszName;
		if( !m_pPlayer->m_fInXen )
			pszName = "info_displacer_xen_target";
		else
			pszName = "info_displacer_earth_target";

		CBaseEntity *pDisplacerTarget = NULL;

		while ((pDisplacerTarget = UTIL_FindEntityByClassname( pDisplacerTarget, pszName )) != NULL)
		{
			if (!FBitSet(pDisplacerTarget->pev->spawnflags, SF_DISPLACER_TARGET_DISABLED))
			{
				pTarget = pDisplacerTarget;
				break;
			}
		}
	}

	if( pTarget )
		tmp = pTarget->pev->origin;

	if( pTarget && /*HACK*/( tmp != Vector( 0, 0, 0 )/*HACK*/ ) )
	{
#if FEATURE_ROPE
		if( (m_pPlayer->m_afPhysicsFlags & PFLAG_ONROPE) )
			m_pPlayer->LetGoRope();
#endif

		// UTIL_ScreenFade( m_pPlayer, Vector( 0, 200, 0 ), 0.5, 0.5, 255, FFADE_IN );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase();

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= DISPLACER_SECONDARY_USAGE;

		UTIL_CleanSpawnPoint( tmp, 50 );

		EMIT_SOUND( m_pPlayer->edict(), CHAN_WEAPON, "weapons/displacer_self.wav", 1, ATTN_NORM );
	 	CDisplacerBall::SelfCreate(m_pPlayer->pev, m_pPlayer->pev->origin);

		// make origin adjustments (origin in center, not at feet)
		//tmp.z -= m_pPlayer->pev->mins.z + 36;
		tmp.z+=37;

		m_pPlayer->pev->flags &= ~FL_ONGROUND;

		UTIL_SetOrigin(m_pPlayer->pev, tmp);

		m_pPlayer->pev->angles = pTarget->pev->angles;

		m_pPlayer->pev->v_angle = pTarget->pev->angles;

		m_pPlayer->pev->fixangle = TRUE;
		m_pPlayer->pev->velocity = m_pPlayer->pev->basevelocity = g_vecZero;

		if( !g_pGameRules->IsMultiplayer())
		{
			m_pPlayer->m_fInXen = !m_pPlayer->m_fInXen;
			if (m_pPlayer->m_fInXen)
				m_pPlayer->pev->gravity = 0.6;
			else
				m_pPlayer->pev->gravity = 1.0;
		}
	}
	else
	{
		EMIT_SOUND( m_pPlayer->edict(), CHAN_WEAPON, "buttons/button11.wav", 0.9f, ATTN_NORM );
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 3.0;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.9;
	}

	SetThink( NULL );
#endif
}

void CDisplacer::LightningEffect( void )
{
#ifndef CLIENT_DLL
	int m_iBeams = 0;

	if( g_pGameRules->IsMultiplayer())
		return;

	for( int i = 2; i < 5; ++i )
	{
		if( !m_pBeam[m_iBeams] )
			m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 16 );
		m_pBeam[m_iBeams]->EntsInit( m_pPlayer->entindex(), m_pPlayer->entindex() );
 		m_pBeam[m_iBeams]->SetStartAttachment( i );
		m_pBeam[m_iBeams]->SetEndAttachment( i == 4 ? i - 2 : i + 1 );
		m_pBeam[m_iBeams]->SetColor( 96, 128, 16 );
		m_pBeam[m_iBeams]->SetBrightness( 240 );
		m_pBeam[m_iBeams]->SetNoise( 60 );
		m_pBeam[m_iBeams]->SetScrollRate( 30 );
		m_pBeam[m_iBeams]->pev->scale = 10;
		m_iBeams++;
	}
#endif
}

void CDisplacer::ClearBeams( void )
{
#ifndef CLIENT_DLL
	if( g_pGameRules->IsMultiplayer())
		return;

	for( int i = 0; i < 3; i++ )
	{
		if( m_pBeam[i] )
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}
#endif
}

BOOL CDisplacer::CanFireDisplacer( int count ) const
{
	return m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= count;
}
#endif
