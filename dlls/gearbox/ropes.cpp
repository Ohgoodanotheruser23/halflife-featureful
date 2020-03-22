/***
*
*	Copyright (c) 2001, Valve LLC. All rights reserved.
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
#include "squadmonster.h"
#include "player.h"
#include "weapons.h"
#include "decals.h"
#include "gamerules.h"
#include "effects.h"
#include "saverestore.h"
#include "ropes.h"

#define HOOK_CONSTANT	2500.0f
#define SPRING_DAMPING	0.1f
#define ROPE_IGNORE_SAMPLES	4		// integrator may be hanging if less than

#define SetEffects(x) pev->effects = x;
#define SetSolidType(x) pev->solid = x;
#define AddFlags(x) pev->flags |= x;
#define SetMoveType(x) pev->movetype = x;
#define AddEffectsFlags(x) pev->effects |= x;

struct RopeSampleData
{
	Vector mPosition;
	Vector mVelocity;
	Vector mForce;
	Vector mExternalForce;

	bool mApplyExternalForce;

	float mMassReciprocal;
	float restLength;
};

#define MAX_LIST_SEGMENTS	5
static RopeSampleData g_pTempList[MAX_LIST_SEGMENTS][MAX_SEGMENTS];

class CRopeSegment : public CBaseAnimating
{
public:
	CRopeSegment();

	void Precache();

	void Spawn();

	void Touch( CBaseEntity* pOther );

	void SetAbsOrigin(const Vector& pos)
	{
		pev->origin = pos;
	}

	static CRopeSegment* CreateSegment( string_t iszModelName, float flRecipMass );

	RopeSampleData* GetSample() { return &m_Data; }

	void ApplyExternalForce( const Vector& vecForce );
	void SetCauseDamageOnTouch( const bool bCauseDamage );
	void SetCanBeGrabbed( const bool bCanBeGrabbed );
	CRope* GetMasterRope() { return m_pMasterRope; }
	void SetMasterRope( CRope* pRope );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	RopeSampleData m_Data;
private:
	CRope* m_pMasterRope;
	string_t m_iszModelName;
	bool m_bCauseDamage;
	bool m_bCanBeGrabbed;
};

static const char* const g_pszCreakSounds[] =
{
	"items/rope1.wav",
	"items/rope2.wav",
	"items/rope3.wav"
};

TYPEDESCRIPTION	CRope::m_SaveData[] =
{
	DEFINE_FIELD( CRope, m_iSegments, FIELD_INTEGER ),
	DEFINE_FIELD( CRope, m_vecLastEndPos, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CRope, m_vecGravity, FIELD_VECTOR ),
	DEFINE_FIELD( CRope, m_iNumSamples, FIELD_INTEGER ),
	DEFINE_FIELD( CRope, m_bObjectAttached, FIELD_BOOLEAN ),
	DEFINE_FIELD( CRope, m_iAttachedObjectsSegment, FIELD_INTEGER ),
	DEFINE_FIELD( CRope, m_flDetachTime, FIELD_TIME ),
	DEFINE_ARRAY( CRope, m_pSegments, FIELD_CLASSPTR, MAX_SEGMENTS ),
	DEFINE_FIELD( CRope, m_bDisallowPlayerAttachment, FIELD_BOOLEAN ),
	DEFINE_FIELD( CRope, m_iszBodyModel, FIELD_STRING ),
	DEFINE_FIELD( CRope, m_iszEndingModel, FIELD_STRING ),
	DEFINE_FIELD( CRope, m_flAttachedObjectsOffset, FIELD_FLOAT ),
	DEFINE_FIELD( CRope, m_bMakeSound, FIELD_BOOLEAN ),
	DEFINE_FIELD( CRope, m_activated, FIELD_CHARACTER ),
};

IMPLEMENT_SAVERESTORE(CRope, CBaseDelay)

LINK_ENTITY_TO_CLASS( env_rope, CRope )

/* leak
CRope::~CRope()
{
	for( size_t uiIndex = 0; uiIndex < MAX_TEMP_SAMPLES; ++uiIndex )
	{
		delete[] m_TempSys[ uiIndex ];
		m_TempSys[ uiIndex ] = NULL;
	}
}*/

CRope::CRope()
{
	m_iszBodyModel = MAKE_STRING( "models/rope16.mdl" );
	m_iszEndingModel = MAKE_STRING( "models/rope16.mdl" );
}

void CRope::KeyValue( KeyValueData* pkvd )
{
	if( FStrEq( pkvd->szKeyName, "segments" ) )
	{
		pkvd->fHandled = true;

		m_iSegments = strtol( pkvd->szValue, NULL, 10 );

		if( m_iSegments >= MAX_SEGMENTS )
			m_iSegments = MAX_SEGMENTS - 1;
	}
	else if( FStrEq( pkvd->szKeyName, "bodymodel" ) )
	{
		pkvd->fHandled = true;

		m_iszBodyModel = ALLOC_STRING( pkvd->szValue );
	}
	else if( FStrEq( pkvd->szKeyName, "endingmodel" ) )
	{
		pkvd->fHandled = true;

		m_iszEndingModel = ALLOC_STRING( pkvd->szValue );
	}
	else if( FStrEq( pkvd->szKeyName, "disable" ) )
	{
		pkvd->fHandled = true;

		m_bDisallowPlayerAttachment = strtol( pkvd->szValue, NULL, 10 );
	}
	else
		CBaseDelay::KeyValue( pkvd );
}

void CRope::Precache()
{
	CBaseDelay::Precache();

	UTIL_PrecacheOther( "rope_segment" );

	PRECACHE_MODEL(STRING(GetBodyModel()));
	PRECACHE_MODEL(STRING(GetEndingModel()));
	PRECACHE_SOUND_ARRAY( g_pszCreakSounds );
}

void CRope::Spawn()
{
	if( m_iszEndingModel == iStringNull )
		m_iszEndingModel = m_iszBodyModel;

	m_bMakeSound = true;

	Precache();

	m_vecGravity = Vector( 0.0f, 0.0f, -50.0f );
	m_iNumSamples = m_iSegments + 1;

	m_activated = false;
}

void CRope::Activate()
{
	if (!m_activated)
	{
		InitRope();
		m_activated = true;
	}
}

void CRope::InitRope()
{
	for( int i = 0; i < m_iNumSamples; i++ )
	{
		CRopeSegment *pSample;

		// NOTE: last segment are invisible, first segment have an infinity mass
		if( i == m_iNumSamples - 2 )
			pSample = CRopeSegment :: CreateSegment( GetEndingModel( ), 1.0f );
		else if( i == m_iNumSamples - 1 )
			pSample = CRopeSegment :: CreateSegment( iStringNull, 0.2f );
		else pSample = CRopeSegment :: CreateSegment( GetBodyModel( ), i == 0 ? 0.0f : 1.0f );
		if( i < ROPE_IGNORE_SAMPLES ) pSample->SetCanBeGrabbed( false );

		// just to have something valid here
		pSample->SetAbsOrigin( pev->origin );
		pSample->SetMasterRope( this );
		m_pSegments[i] = pSample;
	}

	Vector origin, angles;
	const Vector vecGravity = m_vecGravity.Normalize();

	// setup the segments position
	if( m_iNumSamples > 2 )
	{
		CRopeSegment *pPrev, *pCurr;

		for( int iSeg = 1; iSeg < m_iNumSamples; iSeg++ )
		{
			pPrev = m_pSegments[iSeg - 1];
			pCurr = m_pSegments[iSeg - 0];

			// calc direction from previous sample origin and attachment
			origin = pPrev->pev->origin + pPrev->m_Data.restLength * vecGravity;

			pCurr->SetAbsOrigin( origin );
			m_vecLastEndPos = origin;
			SetSegmentAngles( pPrev, pCurr );
		}
	}

	// initialize position data
	for( int iSeg = 0; iSeg < m_iNumSamples; iSeg++ )
	{
		CRopeSegment* pSegment = m_pSegments[iSeg];
		pSegment->m_Data.mPosition = pSegment->pev->origin;
	}

	SetThink(&CRope::RopeThink);
	pev->nextthink = gpGlobals->time + 0.1;
}

#define Q_rint( x )		((x) < 0 ? ((int)((x)-0.5f)) : ((int)((x)+0.5f)))
#define bound( min, num, max )	((num) >= (min) ? ((num) < (max) ? (num) : (max)) : (min))

void CRope::RopeThink()
{
	int subSteps = Q_rint( 100.0f * gpGlobals->frametime ) * 2;
	float delta = (1.0f / 100.0f) * 2.0f;
	subSteps = bound( 2, subSteps, 10 );

	// make ropes nonsense to sv_fps
	for( int idx = 0; idx < subSteps; idx++ )
	{
		ComputeForces( m_pSegments );
		RK4Integrate( delta );
	}

	TraceModels();

	if( ShouldCreak() )
	{
		EMIT_SOUND( edict(), CHAN_BODY, g_pszCreakSounds[RANDOM_LONG( 0, ARRAYSIZE( g_pszCreakSounds ) - 1 )], VOL_NORM, ATTN_NORM );
	}

	pev->nextthink = gpGlobals->time;
}

void CRope::ComputeForces( RopeSampleData* pSystem )
{
	int i;

	for( i = 0; i < m_iNumSamples; i++ )
	{
		ComputeSampleForce( pSystem[i] );
	}

	for( i = 0; i < m_iSegments; i++ )
	{
		ComputeSpringForce( pSystem[i+0], pSystem[i+1] );
	}
}

void CRope::ComputeForces( CRopeSegment** ppSystem )
{
	int i;

	for( i = 0; i < m_iNumSamples; i++ )
	{
		ComputeSampleForce( ppSystem[i]->m_Data );
	}

	for( i = 0; i < m_iSegments; i++ )
	{
		ComputeSpringForce( ppSystem[i+0]->m_Data, ppSystem[i+1]->m_Data );
	}
}

void CRope::ComputeSampleForce( RopeSampleData& data )
{
	data.mForce = g_vecZero;

	if( data.mMassReciprocal != 0.0 )
	{
		data.mForce = data.mForce + ( m_vecGravity / data.mMassReciprocal );
	}

	if( data.mApplyExternalForce )
	{
		data.mForce += data.mExternalForce;
		data.mApplyExternalForce = false;
		data.mExternalForce = g_vecZero;
	}

	if( DotProduct( m_vecGravity, data.mVelocity ) >= 0 )
	{
		data.mForce += data.mVelocity * -0.04;
	}
	else
	{
		data.mForce -= data.mVelocity;
	}
}

void CRope::ComputeSpringForce( RopeSampleData& first, RopeSampleData& second )
{
	Vector vecDist = first.mPosition - second.mPosition;

	const double flDistance = vecDist.Length();
	const double flForce = ( flDistance - first.restLength ) * HOOK_CONSTANT;

	const double flNewRelativeDist = DotProduct( first.mVelocity - second.mVelocity, vecDist ) * SPRING_DAMPING;

	vecDist = vecDist.Normalize();

	const double flSpringFactor = -( flNewRelativeDist / flDistance + flForce );
	const Vector vecForce = flSpringFactor * vecDist;

	first.mForce += vecForce;
	second.mForce -= vecForce;
}

void CRope::RK4Integrate( const float flDeltaTime )
{
	const float flDeltas[MAX_LIST_SEGMENTS - 1] =
	{
		flDeltaTime * 0.5f,
		flDeltaTime * 0.5f,
		flDeltaTime * 0.5f,
		flDeltaTime
	};

	RopeSampleData *pTemp1, *pTemp2, *pTemp3, *pTemp4;
	int i;

	pTemp1 = g_pTempList[0];
	pTemp2 = g_pTempList[1];

	for( i = 0; i < m_iNumSamples; i++, pTemp1++, pTemp2++ )
	{
		const RopeSampleData& data = m_pSegments[i]->m_Data;

		pTemp2->mForce = data.mMassReciprocal * data.mForce * flDeltas[0];
		pTemp2->mVelocity = data.mVelocity * flDeltas[0];
		pTemp2->restLength = data.restLength;

		pTemp1->mMassReciprocal = data.mMassReciprocal;
		pTemp1->mVelocity = data.mVelocity + pTemp2->mForce;
		pTemp1->mPosition = data.mPosition + pTemp2->mVelocity;
		pTemp1->restLength = data.restLength;
	}

	ComputeForces( g_pTempList[0] );

	for( int iStep = 2; iStep < MAX_LIST_SEGMENTS - 1; iStep++ )
	{
		pTemp1 = g_pTempList[0];
		pTemp2 = g_pTempList[iStep];

		for( i = 0; i < m_iNumSamples; i++, pTemp1++, pTemp2++ )
		{
			const RopeSampleData& data = m_pSegments[i]->m_Data;

			pTemp2->mForce = data.mMassReciprocal * pTemp1->mForce * flDeltas[iStep - 1];
			pTemp2->mVelocity = pTemp1->mVelocity * flDeltas[iStep - 1];
			pTemp2->restLength = data.restLength;

			pTemp1->mMassReciprocal = data.mMassReciprocal;
			pTemp1->mVelocity = data.mVelocity + pTemp2->mForce;
			pTemp1->mPosition = data.mPosition + pTemp2->mVelocity;
			pTemp1->restLength = data.restLength;
		}

		ComputeForces( g_pTempList[0] );
	}

	pTemp1 = g_pTempList[0];
	pTemp2 = g_pTempList[4];

	for( i = 0; i < m_iNumSamples; i++, pTemp1++, pTemp2++ )
	{
		const RopeSampleData& data = m_pSegments[i]->m_Data;

		pTemp2->mForce = data.mMassReciprocal * pTemp1->mForce * flDeltas[3];
		pTemp2->mVelocity = pTemp1->mVelocity * flDeltas[3];
	}

	pTemp1 = g_pTempList[1];
	pTemp2 = g_pTempList[2];
	pTemp3 = g_pTempList[3];
	pTemp4 = g_pTempList[4];

	for( i = 0; i < m_iNumSamples; i++, pTemp1++, pTemp2++, pTemp3++, pTemp4++ )
	{
		CRopeSegment *pSegment = m_pSegments[i];
		const Vector vecPosChange = 1.0f / 6.0f * ( pTemp1->mVelocity + ( pTemp2->mVelocity + pTemp3->mVelocity ) * 2 + pTemp4->mVelocity );
		const Vector vecVelChange = 1.0f / 6.0f * ( pTemp1->mForce + ( pTemp2->mForce + pTemp3->mForce ) * 2 + pTemp4->mForce );

		// store final changes for each segment
		pSegment->m_Data.mPosition += vecPosChange;
		pSegment->m_Data.mVelocity += vecVelChange;
	}
}

//TODO move to common header - Solokiller
static const Vector DOWN( 0, 0, -1 );

static const Vector RIGHT( 0, 1, 0 );

void GetAlignmentAngles( const Vector& vecTop, const Vector& vecBottom, Vector& vecOut )
{
	Vector vecDist = vecBottom - vecTop;

	Vector vecResult = vecDist.Normalize();

	const float flRoll = acos( DotProduct( vecResult, RIGHT ) ) * ( 180.0 / M_PI );


	vecDist.y = 0;

	vecResult = vecDist.Normalize();

	const float flPitch = acos( DotProduct( vecResult, DOWN ) ) * ( 180.0 / M_PI );

	vecOut.x = ( vecResult.x >= 0.0 ) ? flPitch : -flPitch;
	vecOut.y = 0;
	vecOut.z = -flRoll;
}

void TruncateEpsilon( Vector& vec )
{
	Vector vec1 =  vec * 10.0;
	vec1.x += 0.5;
	vec = vec1 / 10;
}

void CRope::SetSegmentAngles( CRopeSegment *pCurr, CRopeSegment *pNext )
{
	Vector vecAngles;

	GetAlignmentAngles( pCurr->m_Data.mPosition, pNext->m_Data.mPosition, vecAngles );

	pCurr->pev->angles = vecAngles;
}

void CRope::TraceModels()
{
	if( m_iSegments <= 0 )
		return;

	if( m_iSegments > 1 )
		SetSegmentAngles( m_pSegments[0], m_pSegments[1] );

	TraceResult tr;

	if( m_bObjectAttached )
	{
		for( int iSeg = 1; iSeg < m_iNumSamples; iSeg++ )
		{
			CRopeSegment* pSegment = m_pSegments[iSeg];

			Vector vecDist = pSegment->m_Data.mPosition - m_pSegments[iSeg]->pev->origin;

			vecDist = vecDist.Normalize();

			const float flTraceDist = ( iSeg - m_iAttachedObjectsSegment + 2 ) < 5 ? 50 : 10;

			const Vector vecTraceDist = vecDist * flTraceDist;

			const Vector vecEnd = pSegment->m_Data.mPosition + vecTraceDist;

			UTIL_TraceLine( pSegment->pev->origin, vecEnd, ignore_monsters, edict(), &tr );

			if( tr.flFraction == 1.0 && tr.fAllSolid )
			{
				break;
			}

			if( tr.flFraction != 1.0 || tr.fStartSolid || !tr.fInOpen )
			{
				Vector vecOrigin = tr.vecEndPos - vecTraceDist;

				TruncateEpsilon( vecOrigin );

				pSegment->SetAbsOrigin( vecOrigin );

				Vector vecNormal = tr.vecPlaneNormal.Normalize() * 20000.0;

				RopeSampleData& data = pSegment->m_Data;

				data.mApplyExternalForce = true;
				data.mExternalForce = vecNormal;
				data.mVelocity = g_vecZero;
			}
			else
			{
				Vector vecOrigin = pSegment->m_Data.mPosition;

				TruncateEpsilon( vecOrigin );

				pSegment->SetAbsOrigin( vecOrigin );
			}
		}
	}
	else
	{
		for( int iSeg = 1; iSeg < m_iNumSamples; iSeg++ )
		{
			CRopeSegment* pSegment = m_pSegments[iSeg];

			UTIL_TraceLine( pSegment->pev->origin, pSegment->m_Data.mPosition, ignore_monsters, edict(), &tr );

			if( tr.flFraction == 1.0 )
			{
				Vector vecOrigin = pSegment->m_Data.mPosition;

				TruncateEpsilon( vecOrigin );

				pSegment->SetAbsOrigin( vecOrigin );
			}
			else
			{
				const Vector vecNormal = tr.vecPlaneNormal.Normalize();

				Vector vecOrigin = tr.vecEndPos + vecNormal * 10.0;

				TruncateEpsilon( vecOrigin );

				pSegment->SetAbsOrigin( vecOrigin );
				pSegment->m_Data.mApplyExternalForce = true;
				pSegment->m_Data.mExternalForce = vecNormal * 40000.0;
			}
		}
	}

	for( int iSeg = 1; iSeg < m_iNumSamples; iSeg++ )
	{
		SetSegmentAngles( m_pSegments[iSeg - 1], m_pSegments[iSeg] );
	}

	if( m_iSegments > 1 )
	{
		CRopeSegment *pSegment = m_pSegments[m_iNumSamples - 1];

		UTIL_TraceLine( m_vecLastEndPos, pSegment->m_Data.mPosition, ignore_monsters, edict(), &tr );

		if( tr.flFraction == 1.0 )
		{
			m_vecLastEndPos = pSegment->m_Data.mPosition;
		}
		else
		{
			m_vecLastEndPos = tr.vecEndPos;
			pSegment->m_Data.mApplyExternalForce = true;
			pSegment->m_Data.mExternalForce = tr.vecPlaneNormal.Normalize() * 40000.0;
		}
	}
}

bool CRope :: MoveUp( const float flDeltaTime )
{
	if( m_iAttachedObjectsSegment > ROPE_IGNORE_SAMPLES )
	{
		float flDistance = flDeltaTime * 128.0f;

		while( 1 )
		{
			float flOldDist = flDistance;

			flDistance = 0;

			if( flOldDist <= 0 )
				break;

			if( m_iAttachedObjectsSegment <= 3 )
				break;

			if( flOldDist > m_flAttachedObjectsOffset )
			{
				flDistance = flOldDist - m_flAttachedObjectsOffset;

				m_iAttachedObjectsSegment--;

				float flNewOffset = 0.0f;

				if( m_iAttachedObjectsSegment < m_iSegments )
					flNewOffset = m_pSegments[m_iAttachedObjectsSegment]->m_Data.restLength;
				m_flAttachedObjectsOffset = flNewOffset;
			}
			else
			{
				m_flAttachedObjectsOffset -= flOldDist;
			}
		}
	}

	return true;
}

bool CRope :: MoveDown( const float flDeltaTime )
{
	if( !m_bObjectAttached )
		return false;

	float flDistance = flDeltaTime * 128.0f;
	bool bDoIteration = true;
	bool bOnRope = true;

	while( bDoIteration )
	{
		bDoIteration = false;

		if( flDistance > 0.0f )
		{
			float flNewDist = flDistance;
			float flSegLength = 0.0f;

			while( bOnRope )
			{
				if( m_iAttachedObjectsSegment < m_iSegments )
					flSegLength = m_pSegments[m_iAttachedObjectsSegment]->m_Data.restLength;

				const float flOffset = flSegLength - m_flAttachedObjectsOffset;

				if( flNewDist <= flOffset )
				{
					m_flAttachedObjectsOffset += flNewDist;
					flDistance = 0;
					bDoIteration = true;
					break;
				}

				if( m_iAttachedObjectsSegment + 1 == m_iSegments )
					bOnRope = false;
				else m_iAttachedObjectsSegment++;

				flNewDist -= flOffset;
				flSegLength = 0;

				m_flAttachedObjectsOffset = 0;

				if( flNewDist <= 0 )
					break;
			}
		}
	}

	return bOnRope;
}

Vector CRope::GetAttachedObjectsVelocity( void ) const
{
	if( !m_bObjectAttached )
		return g_vecZero;

	return m_pSegments[m_iAttachedObjectsSegment]->m_Data.mVelocity;
}

void CRope::ApplyForceFromPlayer( const Vector& vecForce )
{
	if( !m_bObjectAttached )
		return;

	float flForce = 20000.0;

	if( m_iSegments < 26 )
		flForce *= ( m_iSegments / 26.0 );

	const Vector vecScaledForce = vecForce * flForce;

	ApplyForceToSegment( vecScaledForce, m_iAttachedObjectsSegment );
}

void CRope::ApplyForceToSegment( const Vector& vecForce, const int iSegment )
{
	if( iSegment < m_iSegments )
	{
		m_pSegments[iSegment]->ApplyExternalForce( vecForce );
	}
	else if( iSegment == m_iSegments )
	{
		// Apply force to the last sample.
		m_pSegments[iSegment - 1]->ApplyExternalForce( vecForce );
	}
}

void CRope::AttachObjectToSegment( CRopeSegment* pSegment )
{
	SetAttachedObjectsSegment( pSegment );

	m_flAttachedObjectsOffset = 0;
	m_bObjectAttached = true;
	m_flDetachTime = 0;
}

void CRope::DetachObject( void )
{
	m_flDetachTime = gpGlobals->time;
	m_bObjectAttached = false;
}

bool CRope::IsAcceptingAttachment( void ) const
{
	if( gpGlobals->time - m_flDetachTime > 2.0 && !m_bObjectAttached )
		return !m_bDisallowPlayerAttachment;
	return false;
}

bool CRope::ShouldCreak( void ) const
{
	if( m_bObjectAttached && m_bMakeSound )
	{
		RopeSampleData& data = m_pSegments[m_iAttachedObjectsSegment]->m_Data;

		if( data.mVelocity.Length() > 20.0 )
			return RANDOM_LONG( 1, 5 ) == 1;
	}

	return false;
}

float CRope::GetSegmentLength( int iSegmentIndex ) const
{
	if( iSegmentIndex < m_iSegments )
		return m_pSegments[iSegmentIndex]->m_Data.restLength;
	return 0;
}

float CRope::GetRopeLength( void ) const
{
	float flLength = 0;

	for( int i = 0; i < m_iSegments; i++ )
		flLength += m_pSegments[i]->m_Data.restLength;

	return flLength;
}

Vector CRope::GetRopeOrigin( void ) const
{
	return m_pSegments[0]->m_Data.mPosition;
}

bool CRope::IsValidSegmentIndex( const int iSegment ) const
{
	return iSegment < m_iSegments;
}

Vector CRope::GetSegmentOrigin( const int iSegment ) const
{
	if( !IsValidSegmentIndex( iSegment ))
		return g_vecZero;

	return m_pSegments[iSegment]->m_Data.mPosition;
}

Vector CRope::GetSegmentAttachmentPoint( const int iSegment ) const
{
	if( !IsValidSegmentIndex( iSegment ) )
		return g_vecZero;

	Vector vecOrigin, vecAngles;

	CRopeSegment* pSegment = m_pSegments[iSegment];

	pSegment->GetAttachment( 0, vecOrigin, vecAngles );

	return vecOrigin;
}

void CRope :: SetAttachedObjectsSegment( CRopeSegment* pSegment )
{
	for( int i = 0; i < m_iSegments; i++ )
	{
		if( m_pSegments[i] == pSegment )
		{
			m_iAttachedObjectsSegment = i;
			break;
		}
	}
}

Vector CRope :: GetSegmentDirFromOrigin( const int iSegmentIndex ) const
{
	if( iSegmentIndex >= m_iSegments )
		return g_vecZero;

	// there is one more sample than there are segments, so this is fine.
	const Vector vecResult = m_pSegments[iSegmentIndex + 1]->m_Data.mPosition - m_pSegments[iSegmentIndex]->m_Data.mPosition;

	return vecResult.Normalize();
}

Vector CRope :: GetAttachedObjectsPosition( void ) const
{
	if( !m_bObjectAttached )
		return g_vecZero;

	Vector vecResult;

	if( m_iAttachedObjectsSegment < m_iSegments )
		vecResult = m_pSegments[m_iAttachedObjectsSegment]->m_Data.mPosition;

	vecResult = vecResult + ( m_flAttachedObjectsOffset * GetSegmentDirFromOrigin( m_iAttachedObjectsSegment ));

	return vecResult;
}

TYPEDESCRIPTION	CRopeSegment::m_SaveData[] =
{
	DEFINE_FIELD( CRopeSegment, m_iszModelName, FIELD_STRING ),
	DEFINE_FIELD( CRopeSegment, m_bCauseDamage, FIELD_BOOLEAN ),
	DEFINE_FIELD( CRopeSegment, m_bCanBeGrabbed, FIELD_BOOLEAN ),
	DEFINE_FIELD( CRopeSegment, m_Data.restLength, FIELD_FLOAT ),
	DEFINE_FIELD( CRopeSegment, m_Data.mPosition, FIELD_VECTOR ),
	DEFINE_FIELD( CRopeSegment, m_Data.mVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CRopeSegment, m_Data.mForce, FIELD_VECTOR ),
	DEFINE_FIELD( CRopeSegment, m_Data.mExternalForce, FIELD_VECTOR ),
	DEFINE_FIELD( CRopeSegment, m_Data.mApplyExternalForce, FIELD_BOOLEAN ),
	DEFINE_FIELD( CRopeSegment, m_Data.mMassReciprocal, FIELD_FLOAT ),
	DEFINE_FIELD( CRopeSegment, m_pMasterRope, FIELD_CLASSPTR ),
};
IMPLEMENT_SAVERESTORE( CRopeSegment, CBaseAnimating )

LINK_ENTITY_TO_CLASS( rope_segment, CRopeSegment );

CRopeSegment::CRopeSegment()
{
	m_iszModelName = MAKE_STRING( "models/rope16.mdl" );
}

void CRopeSegment::Precache()
{
	CBaseAnimating::Precache();

	if (m_iszModelName)
		PRECACHE_MODEL( STRING( m_iszModelName ) );
	PRECACHE_SOUND( "items/grab_rope.wav" );
}

void CRopeSegment::Spawn()
{
	pev->classname = MAKE_STRING( "rope_segment" );

	memset(&m_Data, 0, sizeof(m_Data));

	Precache();

	if (m_iszModelName)
		SET_MODEL( edict(), STRING( m_iszModelName ) );

	UTIL_SetOrigin(pev, pev->origin);

	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_TRIGGER;

	if (!FStringNull(m_iszModelName))
	{
		Vector origin, angles;
		GetAttachment( 0, origin, angles );
		m_Data.restLength = ( origin - pev->origin ).Length();
	}

	UTIL_SetSize( pev, Vector( -30, -30, -30 ), Vector( 30, 30, 30 ) );

	pev->nextthink = gpGlobals->time + 0.5;
}

void CRopeSegment::Touch( CBaseEntity* pOther )
{
	if( pOther->IsPlayer() )
	{
		CBasePlayer *pPlayer = static_cast<CBasePlayer*>( pOther );

		//Electrified wires deal damage. - Solokiller
		if( m_bCauseDamage )
		{
			if( gpGlobals->time >= pev->dmgtime )
			{
				if( pev->dmg < 0 ) pOther->TakeHealth( -pev->dmg, DMG_GENERIC );
				else pOther->TakeDamage( pev, pev, pev->dmg, DMG_SHOCK );
				pev->dmgtime = gpGlobals->time + 0.5f;
			}
		}

		if( GetMasterRope()->IsAcceptingAttachment() && !pPlayer->IsOnRope() )
		{
			if( m_bCanBeGrabbed )
			{
				pOther->pev->origin = m_Data.mPosition;

				pPlayer->SetOnRopeState( true );
				pPlayer->SetRope( GetMasterRope() );
				GetMasterRope()->AttachObjectToSegment( this );

				const Vector& vecVelocity = pOther->pev->velocity;

				if( vecVelocity.Length() > 0.5 )
				{
					//Apply some external force to move the rope. - Solokiller
					m_Data.mApplyExternalForce = true;
					m_Data.mExternalForce += vecVelocity * 750;
				}

				if( GetMasterRope()->IsSoundAllowed() )
				{
					EMIT_SOUND( edict(), CHAN_BODY, "items/grab_rope.wav", 1.0, ATTN_NORM );
				}
			}
			else
			{
				//This segment cannot be grabbed, so grab the highest one if possible. - Solokiller
				CRope *pRope = GetMasterRope();

				CRopeSegment* pSegment;

				if( pRope->GetNumSegments() <= 4 )
				{
					//Fewer than 5 segments exist, so allow grabbing the last one. - Solokiller
					pSegment = pRope->GetSegments()[ pRope->GetNumSegments() - 1 ];
					pSegment->SetCanBeGrabbed( true );
				}
				else
				{
					pSegment = pRope->GetSegments()[ 4 ];
				}

				pSegment->Touch( pOther );
			}
		}
	}
}

CRopeSegment* CRopeSegment::CreateSegment( string_t iszModelName, float flRecipMass )
{
	CRopeSegment *pSegment = GetClassPtr(( CRopeSegment *)NULL );

	pSegment->pev->classname = MAKE_STRING( "rope_segment" );
	pSegment->m_iszModelName = iszModelName;
	pSegment->Spawn();

	pSegment->m_bCauseDamage = false;
	pSegment->m_bCanBeGrabbed = true;
	pSegment->m_Data.mMassReciprocal = flRecipMass;

	return pSegment;
}

void CRopeSegment::ApplyExternalForce( const Vector& vecForce )
{
	m_Data.mApplyExternalForce = true;
	m_Data.mExternalForce += vecForce;
}

void CRopeSegment::SetCauseDamageOnTouch( const bool bCauseDamage )
{
	m_bCauseDamage = bCauseDamage;
}

void CRopeSegment::SetCanBeGrabbed( const bool bCanBeGrabbed )
{
	m_bCanBeGrabbed = bCanBeGrabbed;
}

void CRopeSegment::SetMasterRope(CRope *pRope)
{
	m_pMasterRope = pRope;
	pev->renderfx = pRope->pev->renderfx;
	pev->rendermode = pRope->pev->rendermode;
	pev->rendercolor = pRope->pev->rendercolor;
	pev->renderamt = pRope->pev->renderamt;
	pev->scale = pRope->pev->scale;
	pev->dmg = pRope->pev->dmg;
}

#if 0
class CElectrifiedWire : public CRope
{
public:
	CElectrifiedWire();
	void InitElectrifiedRope();
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void KeyValue( KeyValueData* pkvd );

	void Precache();

	void Spawn();
	void Activate();

	void EXPORT ElectrifiedRopeThink();

	void Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float flValue );

	/**
	*	@return Whether the wire is active.
	*/
	bool IsActive() const { return m_bIsActive; }

	/**
	*	@param iFrequency Frequency.
	*	@return Whether the spark effect should be performed.
	*/
	bool ShouldDoEffect( const int iFrequency );

	/**
	*	Do spark effects.
	*/
	void DoSpark( const size_t uiSegment, const bool bExertForce );

	/**
	*	Do lightning effects.
	*/
	void DoLightning();

private:
	bool m_bIsActive;

	int m_iTipSparkFrequency;
	int m_iBodySparkFrequency;
	int m_iLightningFrequency;

	int m_iXJoltForce;
	int m_iYJoltForce;
	int m_iZJoltForce;

	size_t m_uiNumUninsulatedSegments;
	size_t m_uiUninsulatedSegments[ MAX_SEGMENTS ];

	int m_iLightningSprite;

	float m_flLastSparkTime;
};

CElectrifiedWire::CElectrifiedWire()
{
	m_bIsActive = true;
	m_iTipSparkFrequency = 3;
	m_iBodySparkFrequency = 100;
	m_iLightningFrequency = 150;
	m_iXJoltForce = 0;
	m_iYJoltForce = 0;
	m_iZJoltForce = 0;
	m_uiNumUninsulatedSegments = 0;
}


TYPEDESCRIPTION CElectrifiedWire::m_SaveData[] =
{
	DEFINE_FIELD( CElectrifiedWire, m_bIsActive, FIELD_CHARACTER ),

	DEFINE_FIELD( CElectrifiedWire, m_iTipSparkFrequency, FIELD_INTEGER ),
	DEFINE_FIELD( CElectrifiedWire, m_iBodySparkFrequency, FIELD_INTEGER ),
	DEFINE_FIELD( CElectrifiedWire, m_iLightningFrequency, FIELD_INTEGER ),

	DEFINE_FIELD( CElectrifiedWire, m_iXJoltForce, FIELD_INTEGER ),
	DEFINE_FIELD( CElectrifiedWire, m_iYJoltForce, FIELD_INTEGER ),
	DEFINE_FIELD( CElectrifiedWire, m_iZJoltForce, FIELD_INTEGER ),

	DEFINE_FIELD( CElectrifiedWire, m_uiNumUninsulatedSegments, FIELD_INTEGER ),
	DEFINE_ARRAY( CElectrifiedWire, m_uiUninsulatedSegments, FIELD_INTEGER, MAX_SEGMENTS ),

	//DEFINE_FIELD( m_iLightningSprite, FIELD_INTEGER ), //Not restored, reset in Precache. - Solokiller

	DEFINE_FIELD( CElectrifiedWire, m_flLastSparkTime, FIELD_TIME ),
};

LINK_ENTITY_TO_CLASS( env_electrified_wire, CElectrifiedWire );
IMPLEMENT_SAVERESTORE( CElectrifiedWire, CRope );


void CElectrifiedWire::KeyValue( KeyValueData* pkvd )
{
	if( FStrEq( pkvd->szKeyName, "sparkfrequency" ) )
	{
		m_iTipSparkFrequency = strtol( pkvd->szValue, NULL, 10 );

		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "bodysparkfrequency" ) )
	{
		m_iBodySparkFrequency = strtol( pkvd->szValue, NULL, 10 );

		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "lightningfrequency" ) )
	{
		m_iLightningFrequency = strtol( pkvd->szValue, NULL, 10 );

		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "xforce" ) )
	{
		m_iXJoltForce = strtol( pkvd->szValue, NULL, 10 );

		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "yforce" ) )
	{
		m_iYJoltForce = strtol( pkvd->szValue, NULL, 10 );

		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "zforce" ) )
	{
		m_iZJoltForce = strtol( pkvd->szValue, NULL, 10 );

		pkvd->fHandled = true;
	}
	else
		CRope::KeyValue( pkvd );
}

void CElectrifiedWire::Precache()
{
	CRope::Precache();

	m_iLightningSprite = PRECACHE_MODEL( "sprites/lgtning.spr" );
}

void CElectrifiedWire::Spawn()
{
	CRope::Spawn();
	pev->classname = MAKE_STRING( "env_electrified_wire" );
}

void CElectrifiedWire::Activate()
{
	if (!m_activated)
	{
		InitElectrifiedRope();
		m_activated = true;
	}
}

void CElectrifiedWire::StartElectrifiedThink()
{
	InitRope();

	m_uiNumUninsulatedSegments = 0;
	m_bIsActive = true;

	if( m_iBodySparkFrequency > 0 )
	{
		for( size_t uiIndex = 0; uiIndex < GetNumSegments(); ++uiIndex )
		{
			if( IsValidSegmentIndex( uiIndex ) )
			{
				m_uiUninsulatedSegments[ m_uiNumUninsulatedSegments++ ] = uiIndex;
			}
		}
	}

	if( m_uiNumUninsulatedSegments > 0 )
	{
		for( size_t uiIndex = 0; uiIndex < m_uiNumUninsulatedSegments; ++uiIndex )
		{
			GetSegments()[ uiIndex ]->SetCauseDamageOnTouch( m_bIsActive );
			GetAltSegments()[ uiIndex ]->SetCauseDamageOnTouch( m_bIsActive );
		}
	}

	if( m_iTipSparkFrequency > 0 )
	{
		GetSegments()[ GetNumSegments() - 1 ]->SetCauseDamageOnTouch( m_bIsActive );
		GetAltSegments()[ GetNumSegments() - 1 ]->SetCauseDamageOnTouch( m_bIsActive );
	}

	m_flLastSparkTime = gpGlobals->time;

	SetSoundAllowed( false );

	pev->nextthink = gpGlobals->time + 0.01;
	SetThink(&CElectrifiedWire::ElectrifiedRopeThink);
}

void CElectrifiedWire::ElectrifiedRopeThink()
{
	if( gpGlobals->time - m_flLastSparkTime > 0.1 )
	{
		m_flLastSparkTime = gpGlobals->time;

		if( m_uiNumUninsulatedSegments > 0 )
		{
			for( size_t uiIndex = 0; uiIndex < m_uiNumUninsulatedSegments; ++uiIndex )
			{
				if( ShouldDoEffect( m_iBodySparkFrequency ) )
				{
					DoSpark( m_uiUninsulatedSegments[ uiIndex ], false );
				}
			}
		}

		if( ShouldDoEffect( m_iTipSparkFrequency ) )
		{
			DoSpark( GetNumSegments() - 1, true );
		}

		if( ShouldDoEffect( m_iLightningFrequency ) )
			DoLightning();
	}

	CRope::RopeThink();
}

void CElectrifiedWire::Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float flValue )
{
	m_bIsActive = !m_bIsActive;

	if( m_uiNumUninsulatedSegments > 0 )
	{
		for( size_t uiIndex = 0; uiIndex < m_uiNumUninsulatedSegments; ++uiIndex )
		{
			GetSegments()[ m_uiUninsulatedSegments[ uiIndex ] ]->SetCauseDamageOnTouch( m_bIsActive );
			GetAltSegments()[ m_uiUninsulatedSegments[ uiIndex ] ]->SetCauseDamageOnTouch( m_bIsActive );
		}
	}

	if( m_iTipSparkFrequency > 0 )
	{
		GetSegments()[ GetNumSegments() - 1 ]->SetCauseDamageOnTouch( m_bIsActive );
		GetAltSegments()[ GetNumSegments() - 1 ]->SetCauseDamageOnTouch( m_bIsActive );
	}
}

bool CElectrifiedWire::ShouldDoEffect( const int iFrequency )
{
	if( iFrequency <= 0 )
		return false;

	if( !IsActive() )
		return false;

	return RANDOM_LONG( 1, iFrequency ) == 1;
}

void CElectrifiedWire::DoSpark( const size_t uiSegment, const bool bExertForce )
{
	const Vector vecOrigin = GetSegmentAttachmentPoint( uiSegment );

	UTIL_Sparks( vecOrigin );

	if( bExertForce )
	{
		const Vector vecSparkForce(
			RANDOM_FLOAT( -m_iXJoltForce, m_iXJoltForce ),
			RANDOM_FLOAT( -m_iYJoltForce, m_iYJoltForce ),
			RANDOM_FLOAT( -m_iZJoltForce, m_iZJoltForce )
		);

		ApplyForceToSegment( vecSparkForce, uiSegment );
	}
}

void CElectrifiedWire::DoLightning()
{
	const size_t uiSegment1 = RANDOM_LONG( 0, GetNumSegments() - 1 );

	size_t uiSegment2;

	size_t uiIndex;

	//Try to get a random segment.
	for( uiIndex = 0; uiIndex < 10; ++uiIndex )
	{
		uiSegment2 = RANDOM_LONG( 0, GetNumSegments() - 1 );

		if( uiSegment2 != uiSegment1 )
			break;
	}

	if( uiIndex >= 10 )
		return;

	CRopeSegment* pSegment1;
	CRopeSegment* pSegment2;

	if( GetToggleValue() )
	{
		pSegment1 = GetAltSegments()[ uiSegment1 ];
		pSegment2 = GetAltSegments()[ uiSegment2 ];
	}
	else
	{
		pSegment1 = GetSegments()[ uiSegment1 ];
		pSegment2 = GetSegments()[ uiSegment2 ];
	}

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMENTS );
		WRITE_SHORT( pSegment1->entindex() );
		WRITE_SHORT( pSegment2->entindex() );
		WRITE_SHORT( m_iLightningSprite );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 1 );
		WRITE_BYTE( 10 );
		WRITE_BYTE( 80 );
		WRITE_BYTE( 255 );
		WRITE_BYTE( 255 );
		WRITE_BYTE( 255 );
		WRITE_BYTE( 255 );
		WRITE_BYTE( 255 );
	MESSAGE_END();
}
#endif
