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
#ifndef ROPES_H
#define ROPES_H

class CRopeSegment;

struct RopeSampleData;

#define MAX_SEGMENTS 64
#define MAX_TEMP_SAMPLES 5

/**
*	A rope with a number of segments.
*	Uses an RK4 integrator with dampened springs to simulate rope physics.
*/
class CRope : public CBaseDelay
{
public:
	CRope();

	virtual void KeyValue( KeyValueData* pkvd );

	virtual void Precache();

	virtual void Spawn();
	void Activate();

	void InitRope();
	void EXPORT RopeThink();

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void ComputeForces( RopeSampleData* pSystem );
	void ComputeForces( CRopeSegment** ppSystem );
	void ComputeSampleForce( RopeSampleData& data );
	void ComputeSpringForce( RopeSampleData& first, RopeSampleData& second );

	void RK4Integrate(const float flDeltaTime);

	void TraceModels();
	bool MoveUp( const float flDeltaTime );
	bool MoveDown( const float flDeltaTime );

	Vector GetAttachedObjectsVelocity() const;
	void ApplyForceFromPlayer( const Vector& vecForce );
	void ApplyForceToSegment( const Vector& vecForce, const int uiSegment );
	void AttachObjectToSegment( CRopeSegment* pSegment );
	void DetachObject();

	bool IsObjectAttached() const { return m_bObjectAttached; }
	bool IsAcceptingAttachment() const;

	int GetNumSegments() const { return m_iSegments; }
	CRopeSegment** GetSegments() { return m_pSegments; }

	bool IsSoundAllowed() const { return m_bMakeSound; }
	void SetSoundAllowed( const bool bAllowed )
	{
		m_bMakeSound = bAllowed;
	}

	bool ShouldCreak() const;

	string_t GetBodyModel() const { return m_iszBodyModel; }
	string_t GetEndingModel() const { return m_iszEndingModel; }
	float GetSegmentLength( int uiSegmentIndex ) const;
	float GetRopeLength() const;
	Vector GetRopeOrigin() const;
	bool IsValidSegmentIndex( const int uiSegment ) const;
	Vector GetSegmentOrigin( const int uiSegment ) const;
	Vector GetSegmentAttachmentPoint( const int uiSegment ) const;
	void SetAttachedObjectsSegment( CRopeSegment* pSegment );
	Vector GetSegmentDirFromOrigin( const int uiSegmentIndex ) const;
	Vector GetAttachedObjectsPosition() const;

	void SetSegmentAngles( CRopeSegment *pCurr, CRopeSegment *pNext );

private:
	int m_iSegments;
	int m_iNumSamples;

	Vector m_vecLastEndPos;
	Vector m_vecGravity;

	CRopeSegment* m_pSegments[MAX_SEGMENTS];
	int m_iAttachedObjectsSegment;
	BOOL m_bDisallowPlayerAttachment;
	float m_flAttachedObjectsOffset;
	BOOL m_bObjectAttached;
	float m_flDetachTime;

	string_t m_iszBodyModel;
	string_t m_iszEndingModel;
	BOOL m_bMakeSound;

protected:
	bool m_activated;
};

#endif //ROPES_H
