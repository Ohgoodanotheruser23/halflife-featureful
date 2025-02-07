/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "shake.h"
#include "gamerules.h"
#include "game.h"

#define	GAUSS_PRIMARY_CHARGE_VOLUME	256// how loud gauss is while charging
#define GAUSS_PRIMARY_FIRE_VOLUME	450// how loud gauss is when discharged

LINK_ENTITY_TO_CLASS( weapon_gauss, CGauss )

float CGauss::GetFullChargeTime( void )
{
#if CLIENT_DLL
	if( bIsMultiplayer() )
#else
	if( g_pGameRules->IsMultiplayer() )
#endif
	{
		return 1.5f;
	}

	return 4.0f;
}

#if CLIENT_DLL
extern int g_irunninggausspred;
#endif

void CGauss::Spawn()
{
	Precache();
	m_iId = WEAPON_GAUSS;
	SET_MODEL( ENT( pev ), MyWModel() );

	InitDefaultAmmo(GAUSS_DEFAULT_GIVE);

	FallInit();// get ready to fall down.
}

void CGauss::Precache( void )
{
	PRECACHE_MODEL( MyWModel() );
	PRECACHE_MODEL( "models/v_gauss.mdl" );
	PrecachePModel( "models/p_gauss.mdl" );

	PRECACHE_SOUND( "items/9mmclip1.wav" );

	PRECACHE_SOUND( "weapons/gauss2.wav" );
	PRECACHE_SOUND( "weapons/electro4.wav" );
	PRECACHE_SOUND( "weapons/electro5.wav" );
	PRECACHE_SOUND( "weapons/electro6.wav" );
	PRECACHE_SOUND( "ambience/pulsemachine.wav" );

	m_iGlow = PRECACHE_MODEL( "sprites/hotglow.spr" );
	m_iBalls = PRECACHE_MODEL( "sprites/hotglow.spr" );
	m_iBeam = PRECACHE_MODEL( "sprites/smoke.spr" );

	precacheGunPickupSound();
	precacheAmmoPickupSound();

	m_usGaussFire = PRECACHE_EVENT( 1, "events/gauss.sc" );
	m_usGaussSpin = PRECACHE_EVENT( 1, "events/gaussspin.sc" );
}

int CGauss::AddToPlayer( CBasePlayer *pPlayer )
{
	return AddToPlayerDefault(pPlayer);
}

int CGauss::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "uranium";
	p->pszAmmo2 = NULL;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 1;
	p->iId = WEAPON_GAUSS;
	p->iFlags = 0;
	p->iWeight = GAUSS_WEIGHT;
	p->pszAmmoEntity = "ammo_gaussclip";
	p->iDropAmmo = AMMO_URANIUMBOX_GIVE;

	return 1;
}

BOOL CGauss::Deploy()
{
	m_pPlayer->m_flPlayAftershock = 0.0;
	return DefaultDeploy( "models/v_gauss.mdl", "models/p_gauss.mdl", GAUSS_DRAW, "gauss" );
}

void CGauss::Holster()
{
	PLAYBACK_EVENT_FULL( FEV_RELIABLE | FEV_GLOBAL, m_pPlayer->edict(), m_usGaussFire, 0.01f, m_pPlayer->pev->origin, m_pPlayer->pev->angles, 0.0, 0.0, 0, 0, 0, 1 );

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;

	SendWeaponAnim( GAUSS_HOLSTER );
	m_fInAttack = 0;
}

void CGauss::PrimaryAttack()
{
	// don't fire underwater
	if( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound();
		m_flNextSecondaryAttack = m_flNextPrimaryAttack = GetNextAttackDelay( 0.15f );
		return;
	}

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < 2 )
	{
		PlayEmptySound();
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
		return;
	}

	m_pPlayer->m_iWeaponVolume = GAUSS_PRIMARY_FIRE_VOLUME;
	m_fPrimaryFire = TRUE;

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 2;

	StartFire();
	m_fInAttack = 0;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.2f;
}

void CGauss::SecondaryAttack()
{
	if( m_pPlayer->m_flStartCharge > gpGlobals->time )
		m_pPlayer->m_flStartCharge = gpGlobals->time;
	// don't fire underwater
	if( m_pPlayer->pev->waterlevel == 3 )
	{
		if( m_fInAttack != 0 )
		{
			PLAYBACK_EVENT_FULL( FEV_RELIABLE, m_pPlayer->edict(), m_usGaussSpin, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, 80 + RANDOM_LONG( 0, 0x3f ), 1, 0, 0 );
			//EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/electro4.wav", 1.0, ATTN_NORM, 0, 80 + RANDOM_LONG( 0, 0x3f ) );
			SendWeaponAnim( GAUSS_IDLE );
			m_fInAttack = 0;
		}
		else
		{
			PlayEmptySound();
		}

		m_flNextSecondaryAttack = m_flNextPrimaryAttack = GetNextAttackDelay( 0.5f );
		return;
	}

	if( m_fInAttack == 0 )
	{
		if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		{
			EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM );
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
			return;
		}

		m_fPrimaryFire = FALSE;

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;// take one ammo just to start the spin
		m_pPlayer->m_flNextAmmoBurn = UTIL_WeaponTimeBase();

		// spin up
		m_pPlayer->m_iWeaponVolume = GAUSS_PRIMARY_CHARGE_VOLUME;

		SendWeaponAnim( GAUSS_SPINUP );
		m_fInAttack = 1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5f;
		m_pPlayer->m_flStartCharge = gpGlobals->time;
		m_pPlayer->m_flAmmoStartCharge = UTIL_WeaponTimeBase() + GetFullChargeTime();

		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usGaussSpin, 0.0f, g_vecZero, g_vecZero, 0.0f, 0.0f, 110, 0, 0, 0 );

		m_iSoundState = SND_CHANGE_PITCH;
	}
	else if( m_fInAttack == 1 )
	{
		if( m_flTimeWeaponIdle < UTIL_WeaponTimeBase() )
		{
			SendWeaponAnim( GAUSS_SPIN );
			m_fInAttack = 2;
		}
	}
	else
	{
		// during the charging process, eat one bit of ammo every once in a while
		if( UTIL_WeaponTimeBase() >= m_pPlayer->m_flNextAmmoBurn && m_pPlayer->m_flNextAmmoBurn != 1000 )
		{
#if CLIENT_DLL
			if( bIsMultiplayer() )
#else
			if( g_pGameRules->IsMultiplayer() )
#endif
			{
				m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
				m_pPlayer->m_flNextAmmoBurn = UTIL_WeaponTimeBase() + 0.1f;
			}
			else
			{
				m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
				m_pPlayer->m_flNextAmmoBurn = UTIL_WeaponTimeBase() + 0.3f;
			}
		}

		if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		{
			// out of ammo! force the gun to fire
			StartFire();
			m_fInAttack = 0;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1;
			return;
		}

		if( UTIL_WeaponTimeBase() >= m_pPlayer->m_flAmmoStartCharge )
		{
			// don't eat any more ammo after gun is fully charged.
			m_pPlayer->m_flNextAmmoBurn = 1000;
		}

		int pitch = (int)( ( gpGlobals->time - m_pPlayer->m_flStartCharge ) * ( 150 / GetFullChargeTime() ) + 100 );
		if( pitch > 250 ) 
			 pitch = 250;
		
		// ALERT( at_console, "%d %d %d\n", m_fInAttack, m_iSoundState, pitch );

		const bool overcharge = m_pPlayer->m_flStartCharge < gpGlobals->time - 10.0f;

		if( m_iSoundState == 0 )
			ALERT( at_console, "sound state %d\n", m_iSoundState );

		if (!overcharge)
			PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usGaussSpin, 0.0f, (float *)&g_vecZero, (float *)&g_vecZero, 0.0f, 0.0f, pitch, 0, ( m_iSoundState == SND_CHANGE_PITCH ) ? 1 : 0, 0 );

		m_iSoundState = SND_CHANGE_PITCH; // hack for going through level transitions

		m_pPlayer->m_iWeaponVolume = GAUSS_PRIMARY_CHARGE_VOLUME;

		// m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1;
		if( overcharge )
		{
			// Player charged up too long. Zap him.
			PLAYBACK_EVENT_FULL( FEV_NOTHOST | FEV_RELIABLE, m_pPlayer->edict(), m_usGaussSpin, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 80 + RANDOM_LONG( 0, 0x3f ), 1, 0, 0 );
			//EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/electro4.wav", 1.0f, ATTN_NORM, 0, 80 + RANDOM_LONG( 0, 0x3f ) );
			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/electro6.wav", 1.0f, ATTN_NORM, 0, 75 + RANDOM_LONG( 0, 0x3f ) );

			m_fInAttack = 0;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0f;
#if !CLIENT_DLL
			m_pPlayer->TakeDamage( VARS( eoNullEntity ), VARS( eoNullEntity ), 50, DMG_SHOCK );
			UTIL_ScreenFade( m_pPlayer, Vector( 255, 128, 0 ), 2, 0.5f, 128, FFADE_IN );
#endif
			SendWeaponAnim( GAUSS_IDLE );

			// Player may have been killed and this weapon dropped, don't execute any more code after this!
			return;
		}
	}
}

//=========================================================
// StartFire- since all of this code has to run and then 
// call Fire(), it was easier at this point to rip it out 
// of weaponidle() and make its own function then to try to
// merge this into Fire(), which has some identical variable names 
//=========================================================
void CGauss::StartFire( void )
{
	float flDamage;

	if( m_pPlayer->m_flStartCharge > gpGlobals->time )
		m_pPlayer->m_flStartCharge = gpGlobals->time;
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecSrc = m_pPlayer->GetGunPosition(); // + gpGlobals->v_up * -8 + gpGlobals->v_right * 8;

	if( gpGlobals->time - m_pPlayer->m_flStartCharge > GetFullChargeTime() )
	{
		flDamage = 200.0f;
	}
	else
	{
		flDamage = 200.0f * ( ( gpGlobals->time - m_pPlayer->m_flStartCharge ) / GetFullChargeTime() );
	}

	if( m_fPrimaryFire )
	{
		// fixed damage on primary attack
#if CLIENT_DLL
		flDamage = 20.0f;
#else 
		flDamage = gSkillData.plrDmgGauss;
#endif
	}

	if( m_fInAttack != 3 )
	{
		//ALERT( at_console, "Time:%f Damage:%f\n", gpGlobals->time - m_pPlayer->m_flStartCharge, flDamage );
#if !CLIENT_DLL
		float flZVel = m_pPlayer->pev->velocity.z;

		if( !m_fPrimaryFire )
		{
			m_pPlayer->pev->velocity = m_pPlayer->pev->velocity - gpGlobals->v_forward * flDamage * 5.0f;
		}

		if( !g_pGameRules->IsMultiplayer() )
		{
			// in deathmatch, gauss can pop you up into the air. Not in single play.
			m_pPlayer->pev->velocity.z = flZVel;
		}
#endif
		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	}

	// time until aftershock 'static discharge' sound
	m_pPlayer->m_flPlayAftershock = gpGlobals->time + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.3f, 0.8f );

	Fire( vecSrc, vecAiming, flDamage );
}

void CGauss::Fire( Vector vecOrigSrc, Vector vecDir, float flDamage )
{
	m_pPlayer->m_iWeaponVolume = GAUSS_PRIMARY_FIRE_VOLUME;
	TraceResult tr, beam_tr;
#if !CLIENT_DLL
	Vector vecSrc = vecOrigSrc;
	Vector vecDest = vecSrc + vecDir * 8192.0f;
	edict_t	*pentIgnore;
	float flMaxFrac = 1.0f;
	int nTotal = 0;
	int fHasPunched = 0;
	int fFirstBeam = 1;
	int nMaxHits = 10;

	pentIgnore = ENT( m_pPlayer->pev );
#else
	if( m_fPrimaryFire == false )
		 g_irunninggausspred = true;
#endif	
	// The main firing event is sent unreliably so it won't be delayed.
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usGaussFire, 0.0f, m_pPlayer->pev->origin, m_pPlayer->pev->angles, flDamage, 0.0, 0, 0, m_fPrimaryFire ? 1 : 0, 0 );

	// This reliable event is used to stop the spinning sound
	// It's delayed by a fraction of second to make sure it is delayed by 1 frame on the client
	// It's sent reliably anyway, which could lead to other delays

	PLAYBACK_EVENT_FULL( FEV_NOTHOST | FEV_RELIABLE | FEV_GLOBAL, m_pPlayer->edict(), m_usGaussFire, 0.01f, m_pPlayer->pev->origin, m_pPlayer->pev->angles, 0.0, 0.0, 0, 0, 0, 1 );

	/*ALERT( at_console, "%f %f %f\n%f %f %f\n", 
		vecSrc.x, vecSrc.y, vecSrc.z, 
		vecDest.x, vecDest.y, vecDest.z );*/

	//ALERT( at_console, "%f %f\n", tr.flFraction, flMaxFrac );

#if !CLIENT_DLL
	while( flDamage > 10 && nMaxHits > 0 )
	{
		nMaxHits--;

		// ALERT( at_console, "." );
		UTIL_TraceLine( vecSrc, vecDest, dont_ignore_monsters, pentIgnore, &tr );

		if( tr.fAllSolid )
			break;

		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		if( pEntity == NULL )
			break;

		if( fFirstBeam )
		{
			m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
			fFirstBeam = 0;

			nTotal += 26;
		}

		if( pEntity->pev->takedamage )
		{
			ClearMultiDamage();
			if( pEntity->pev == m_pPlayer->pev )
				tr.iHitgroup = 0;
			pEntity->TraceAttack( m_pPlayer->pev, m_pPlayer->pev, flDamage, vecDir, &tr, DMG_BULLET );
			ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );
		}

		if( pEntity->ReflectGauss() )
		{
			float n;

			pentIgnore = NULL;

			n = -DotProduct( tr.vecPlaneNormal, vecDir );

			if( n < 0.5f ) // 60 degrees
			{
				// ALERT( at_console, "reflect %f\n", n );
				// reflect
				Vector r;

				r = 2.0 * tr.vecPlaneNormal * n + vecDir;
				flMaxFrac = flMaxFrac - tr.flFraction;
				vecDir = r;
				vecSrc = tr.vecEndPos + vecDir * 8.0f;
				vecDest = vecSrc + vecDir * 8192.0f;

				// explode a bit
				m_pPlayer->RadiusDamage( tr.vecEndPos, pev, m_pPlayer->pev, flDamage * n, CLASS_NONE, DMG_BLAST );

				nTotal += 34;

				// lose energy
				if( n == 0.0f ) n = 0.1f;
				flDamage = flDamage * ( 1.0f - n );
			}
			else
			{
				nTotal += 13;

				// limit it to one hole punch
				if( fHasPunched )
					break;
				fHasPunched = 1;

				// try punching through wall if secondary attack (primary is incapable of breaking through)
				if( !m_fPrimaryFire )
				{
					UTIL_TraceLine( tr.vecEndPos + vecDir * 8, vecDest, dont_ignore_monsters, pentIgnore, &beam_tr );
					if( !beam_tr.fAllSolid )
					{
						// trace backwards to find exit point
						UTIL_TraceLine( beam_tr.vecEndPos, tr.vecEndPos, dont_ignore_monsters, pentIgnore, &beam_tr );

						n = ( beam_tr.vecEndPos - tr.vecEndPos ).Length();

						if( n < flDamage )
						{
							if( n == 0.0f )
								n = 1.0f;
							flDamage -= n;

							// ALERT( at_console, "punch %f\n", n );
							nTotal += 21;

							// exit blast damage
							//m_pPlayer->RadiusDamage( beam_tr.vecEndPos + vecDir * 8, pev, m_pPlayer->pev, flDamage, CLASS_NONE, DMG_BLAST );
							float damage_radius;

							if( g_pGameRules->IsMultiplayer() )
							{
								damage_radius = flDamage * 1.75f;  // Old code == 2.5
							}
							else
							{
								damage_radius = flDamage * 2.5f;
							}

							::RadiusDamage( beam_tr.vecEndPos + vecDir * 8, pev, m_pPlayer->pev, flDamage, damage_radius, CLASS_NONE, DMG_BLAST );

							CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0f );

							nTotal += 53;

							vecSrc = beam_tr.vecEndPos + vecDir;
						}
						else if( !selfgauss.value )
						{
							flDamage = 0;
						}
					}
					else
					{
						 //ALERT( at_console, "blocked %f\n", n );
						flDamage = 0;
					}
				}
				else
				{
					//ALERT( at_console, "blocked solid\n" );

					flDamage = 0;
				}

			}
		}
		else
		{
			vecSrc = tr.vecEndPos + vecDir;
			pentIgnore = ENT( pEntity->pev );
		}
	}
#endif
	// ALERT( at_console, "%d bytes\n", nTotal );
}

void CGauss::WeaponIdle( void )
{
	ResetEmptySound();

	// play aftershock static discharge
	if( m_pPlayer->m_flPlayAftershock && m_pPlayer->m_flPlayAftershock < gpGlobals->time )
	{
		switch( RANDOM_LONG( 0, 3 ) )
		{
		case 0:
			EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/electro4.wav", RANDOM_FLOAT( 0.7f, 0.8f ), ATTN_NORM );
			break;
		case 1:
			EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/electro5.wav", RANDOM_FLOAT( 0.7f, 0.8f ), ATTN_NORM );
			break;
		case 2:
			EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/electro6.wav", RANDOM_FLOAT( 0.7f, 0.8f ), ATTN_NORM );
			break;
		case 3:
			break; // no sound
		}
		m_pPlayer->m_flPlayAftershock = 0.0f;
	}

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if( m_fInAttack != 0 )
	{
		StartFire();
		m_fInAttack = 0;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0f;
	}
	else
	{
		int iAnim;
		bool canPlayFidgetAnimation = false;
#if !CLIENT_DLL
		canPlayFidgetAnimation = g_modFeatures.gauss_fidget;
#endif
		const float idleRand = canPlayFidgetAnimation ? 0.5f : 0.66f;

		float flRand = RANDOM_FLOAT( 0.0f, 1.0f );
		if( flRand <= idleRand )
		{
			iAnim = GAUSS_IDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10.0f, 15.0f );
		}
		else if( flRand <= 0.75f || !canPlayFidgetAnimation )
		{
			iAnim = GAUSS_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10.0f, 15.0f );
		}
		else
		{
			iAnim = GAUSS_FIDGET;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;
		}
#if !CLIENT_DLL
		SendWeaponAnim( iAnim );
#endif
	}
}

void CGauss::GetWeaponData(weapon_data_t& data)
{
	data.iuser2 = m_fInAttack;
	data.fuser1 = m_pPlayer->m_flStartCharge;
}

void CGauss::SetWeaponData(const weapon_data_t& data)
{
	m_fInAttack = data.iuser2;
	m_pPlayer->m_flStartCharge = data.fuser1;
}
