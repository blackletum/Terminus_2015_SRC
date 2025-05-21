//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "player_control.h"
#include "soundent.h"
#include "func_break.h"		// For materials
#include "hl2_player.h"
#include "ai_hull.h"
#include "decals.h"
#include "shake.h"
#include "ndebugoverlay.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC( CPlayer_Control )

	DEFINE_FIELD( m_bActive, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_nSaveFOV,					FIELD_INTEGER ),
	DEFINE_FIELD( m_vSaveOrigin,				FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vSaveAngles,				FIELD_VECTOR ),
	DEFINE_FIELD( m_nSaveMoveType,			FIELD_INTEGER ),
	DEFINE_FIELD( m_nSaveMoveCollide,			FIELD_INTEGER ),
	DEFINE_FIELD( m_vSaveViewOffset,			FIELD_VECTOR ),
	DEFINE_FIELD( m_pSaveWeapon,				FIELD_CLASSPTR ),
	DEFINE_FIELD( m_nPClipTraceDir,			FIELD_INTEGER),
	DEFINE_FIELD( m_flCurrMinClipDist,		FIELD_FLOAT),
	DEFINE_FIELD( m_flLastMinClipDist,		FIELD_FLOAT),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Activate", InputActivate ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Deactivate", InputDeactivate ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetThrust", InputSetThrust ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetSideThrust", InputSetSideThrust ),

END_DATADESC()

LINK_ENTITY_TO_CLASS(player_control, CPlayer_Control);

#define PMANHACK_OFFSET		Vector(0,0,-18)
#define PMANHACK_HULL_MINS	Vector(-22,-22,-22)
#define PMANHACK_HULL_MAXS	Vector(22,22,22)
#define PMANHACK_MAX_SPEED	400
#define PMANHACK_DECAY		6

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CPlayer_Control::Precache( void )
{
	PrecacheModel("models/manhack.mdl");
	PrecacheModel("models/manhack_sphere.mdl");

	PrecacheScriptSound("Player_Manhack.ThrustLow");
	PrecacheScriptSound("Player_Manhack.ThrustHigh");
	PrecacheScriptSound("Player_Manhack.GrindFlesh");
	PrecacheScriptSound("Player_Manhack.Grind");
	return;
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CPlayer_Control::Spawn( void )
{
	Precache();
	SetFriction( 0.55 ); // deading the bounce a bit
	SetModel("models/manhack.mdl");
	UTIL_SetSize(this, PMANHACK_HULL_MINS, PMANHACK_HULL_MAXS);

	m_bActive = false;

	CreateVPhysics();
}

//-----------------------------------------------------------------------------

bool CPlayer_Control::CreateVPhysics( void )
{
	// Create the object in the physics system
	VPhysicsInitNormal( SOLID_BBOX, 0, false );
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayer_Control::ControlActivate( void )
{
	m_bActive			= true;
	AddEffects( EF_NODRAW );
	m_nPClipTraceDir	= PC_TRACE_LAST;
	m_flCurrMinClipDist = PCONTROL_CLIP_DIST;

	SetNextThink( gpGlobals->curtime );

	CHL2_Player*	pPlayer		= (CHL2_Player*)UTIL_GetLocalPlayer();

	Assert( pPlayer );

	// Save Data
	m_nSaveFOV			= pPlayer->GetFOV();
	m_vSaveOrigin		= pPlayer->GetLocalOrigin();
	m_vSaveAngles		= pPlayer->pl.v_angle;
	m_nSaveMoveType		= pPlayer->GetMoveType();
	m_nSaveMoveCollide	= pPlayer->GetMoveCollide();
	m_vSaveViewOffset	= pPlayer->GetViewOffset();
	m_pSaveWeapon		= pPlayer->GetActiveWeapon();

	pPlayer->AddSolidFlags( FSOLID_NOT_SOLID );
	pPlayer->SetLocalOrigin( GetLocalOrigin() );
	pPlayer->pl.v_angle	= GetLocalAngles();
	pPlayer->SetViewOffset( vec3_origin );

	DispatchUpdateTransmitState();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayer_Control::ControlDeactivate( void )
{
	m_bActive = false;
	RemoveEffects( EF_NODRAW );

	SetThink(NULL);
	SetTouch(NULL);

	CHL2_Player*	pPlayer  = (CHL2_Player*)UTIL_GetLocalPlayer();

	Assert( pPlayer );
	
	// Restore Data
	pPlayer->SetFOV( this, m_nSaveFOV );
	//FOV never gets restored
	pPlayer->RemoveSolidFlags( FSOLID_NOT_SOLID );
	pPlayer->SetLocalOrigin( m_vSaveOrigin );
	pPlayer->SetLocalAngles( m_vSaveAngles );	// Note: Set GetLocalAngles(), not pl->v_angle
	pPlayer->SnapEyeAngles( m_vSaveAngles );
	pPlayer->StopFollowingEntity();
	pPlayer->SetMoveType( m_nSaveMoveType, m_nSaveMoveCollide );
	pPlayer->SetViewOffset( m_vSaveViewOffset );
	pPlayer->SetControlClass( CLASS_NONE );

	DispatchUpdateTransmitState();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayer_Control::InputActivate( inputdata_t &inputdata )
{
	ControlActivate();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayer_Control::InputDeactivate( inputdata_t &inputdata )
{
	ControlDeactivate();
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CPlayer_Control::InputSetThrust( inputdata_t &inputdata )
{
	// Should be handles by subclass
	return;
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CPlayer_Control::InputSetSideThrust( inputdata_t &inputdata )
{
	// Should be handles by subclass
	return;
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
float CPlayer_Control::PlayerControlClipDistance(void)
{
	// Send out a curb feeler, but not in every direction on every time.  Instead
	// send out a feeling in one direction each time and keep track of the nearest
	// player_control_clip
	
	// Reset if on last trace
	if (m_nPClipTraceDir == PC_TRACE_LAST)
	{
		m_nPClipTraceDir	= 0;
		m_flLastMinClipDist = m_flCurrMinClipDist;
		m_flCurrMinClipDist = PCONTROL_CLIP_DIST;
	}

	Vector vForward,vRight,vUp;
	AngleVectors( GetLocalAngles(), &vForward, &vRight,&vUp);

	Vector vTracePos = GetAbsOrigin();
	switch (m_nPClipTraceDir)
	{
		case PC_TRACE_LEFT:
		{
			vTracePos -= vRight*PCONTROL_CLIP_DIST;
		}
		case PC_TRACE_RIGHT:
		{
			vTracePos += vRight*PCONTROL_CLIP_DIST;
		}
		case PC_TRACE_UP:
		{
			vTracePos += vUp*PCONTROL_CLIP_DIST;
		}
		case PC_TRACE_DOWN:
		{
			vTracePos -= vUp*PCONTROL_CLIP_DIST;
		}
		case PC_TRACE_FORWARD:
		{
			vTracePos += 2*vForward*PCONTROL_CLIP_DIST;
		}
	}

	// Trace in a different direction next time
	m_nPClipTraceDir++;


	trace_t tr;
//	UTIL_TraceLine(GetAbsOrigin(), vTracePos, CONTENTS_MIST, this, COLLISION_GROUP_NONE, &tr);
	UTIL_TraceLine(GetAbsOrigin(), vTracePos, 0, this, COLLISION_GROUP_NONE, &tr);
	if (tr.fraction < 1.0)
	{
		surfacedata_t* pSurface = physprops->GetSurfaceData( tr.surface.surfaceProps );
		char	  	   m_chTextureType = pSurface->game.material;

		if (m_chTextureType == CHAR_TEX_CLIP)
		{	
			float flClipDist = tr.fraction * (GetAbsOrigin()-vTracePos).Length();
			if (flClipDist < m_flCurrMinClipDist)
			{
				m_flCurrMinClipDist = flClipDist;
			}
			if (flClipDist < m_flLastMinClipDist)
			{
				m_flLastMinClipDist = flClipDist;
			}	
		}
	}

	return m_flLastMinClipDist;
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
int CPlayer_Control::UpdateTransmitState()
{
	if ( m_bActive )
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}
	else 
	{
		return BaseClass::UpdateTransmitState();
	}
}

