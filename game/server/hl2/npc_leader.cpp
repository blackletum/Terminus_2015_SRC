//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: NPC for compatibility with Pissing Misinformation 1.5
//
//=============================================================================//

#include "cbase.h"
#include "ai_default.h"
#include "ai_task.h"
#include "ai_schedule.h"
#include "ai_node.h"
#include "ai_hull.h"
#include "ai_hint.h"
#include "ai_squad.h"
#include "ai_senses.h"
#include "ai_navigator.h"
#include "ai_motor.h"
#include "ai_behavior.h"
#include "ai_baseactor.h"
#include "ai_behavior_lead.h"
#include "ai_behavior_follow.h"
#include "ai_behavior_standoff.h"
#include "ai_behavior_assault.h"
#include "npc_playercompanion.h"
#include "soundent.h"
#include "game.h"
#include "npcevent.h"
#include "activitylist.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "sceneentity.h"
#include "ai_behavior_functank.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define LEADER_MODEL "models/vance.mdl"

ConVar	sk_leader_health( "sk_leader_health","50");

//=========================================================
// Barney activities
//=========================================================

class CNPC_Leader : public CNPC_PlayerCompanion
{
public:
	DECLARE_CLASS( CNPC_Leader, CNPC_PlayerCompanion );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual void Precache()
	{
		// Prevents a warning
		SelectModel( );
		BaseClass::Precache();

		PrecacheScriptSound( "NPC_Barney.FootstepLeft" );
		PrecacheScriptSound( "NPC_Barney.FootstepRight" );
		PrecacheScriptSound( "NPC_Barney.Die" );

		PrecacheInstancedScene( "scenes/Expressions/BarneyIdle.vcd" );
		PrecacheInstancedScene( "scenes/Expressions/BarneyAlert.vcd" );
		PrecacheInstancedScene( "scenes/Expressions/BarneyCombat.vcd" );
	}

	void	Spawn( void );
	void	SelectModel();
	Class_T Classify( void );
	void	Weapon_Equip( CBaseCombatWeapon *pWeapon );

	bool CreateBehaviors( void );

	void HandleAnimEvent( animevent_t *pEvent );

	bool ShouldLookForBetterWeapon() { return false; }

	void OnChangeRunningBehavior( CAI_BehaviorBase *pOldBehavior,  CAI_BehaviorBase *pNewBehavior );

	void DeathSound( const CTakeDamageInfo &info );
	void GatherConditions();
	void UseFunc( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	CAI_FuncTankBehavior		m_FuncTankBehavior;
	COutputEvent				m_OnPlayerUse;

	DEFINE_CUSTOM_AI;
};


LINK_ENTITY_TO_CLASS( npc_leader, CNPC_Leader );

//---------------------------------------------------------
// 
//---------------------------------------------------------
//IMPLEMENT_CUSTOM_AI( npc_conscript,CNPC_Leader );
IMPLEMENT_SERVERCLASS_ST(CNPC_Leader, DT_NPC_Leader)
END_SEND_TABLE()


//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_Leader )
//						m_FuncTankBehavior
	DEFINE_OUTPUT( m_OnPlayerUse, "OnPlayerUse" ),
	DEFINE_USEFUNC( UseFunc ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Leader::SelectModel()
{
	SetModelName( AllocPooledString( LEADER_MODEL ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Leader::Spawn( void )
{
	Precache();

	m_iHealth = sk_leader_health.GetFloat();;

	m_iszIdleExpression = MAKE_STRING("scenes/Expressions/BarneyIdle.vcd");
	m_iszAlertExpression = MAKE_STRING("scenes/Expressions/BarneyAlert.vcd");
	m_iszCombatExpression = MAKE_STRING("scenes/Expressions/BarneyCombat.vcd");

	BaseClass::Spawn();

	AddEFlags( EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL | EFL_NO_PHYSCANNON_INTERACTION );

	NPCInit();

	SetUse( &CNPC_Leader::UseFunc );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_Leader::Classify( void )
{
	return	CLASS_CONSCRIPT;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Leader::Weapon_Equip( CBaseCombatWeapon *pWeapon )
{
	BaseClass::Weapon_Equip( pWeapon );

	if( hl2_episodic.GetBool() && FClassnameIs( pWeapon, "weapon_ar2" ) )
	{
		// Allow Barney to defend himself at point-blank range in c17_05.
		pWeapon->m_fMinRange1 = 0.0f;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Leader::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
	case NPC_EVENT_LEFTFOOT:
		{
			EmitSound( "NPC_Barney.FootstepLeft", pEvent->eventtime );
		}
		break;
	case NPC_EVENT_RIGHTFOOT:
		{
			EmitSound( "NPC_Barney.FootstepRight", pEvent->eventtime );
		}
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_Leader::DeathSound( const CTakeDamageInfo &info )
{
	// Sentences don't play on dead NPCs
	SentenceStop();

	EmitSound( "npc_barney.die" );

}

bool CNPC_Leader::CreateBehaviors( void )
{
	BaseClass::CreateBehaviors();
	AddBehavior( &m_FuncTankBehavior );

	return true;
}

void CNPC_Leader::OnChangeRunningBehavior( CAI_BehaviorBase *pOldBehavior,  CAI_BehaviorBase *pNewBehavior )
{
	if ( pNewBehavior == &m_FuncTankBehavior )
	{
		m_bReadinessCapable = false;
	}
	else if ( pOldBehavior == &m_FuncTankBehavior )
	{
		m_bReadinessCapable = IsReadinessCapable();
	}

	BaseClass::OnChangeRunningBehavior( pOldBehavior, pNewBehavior );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Leader::GatherConditions()
{
	BaseClass::GatherConditions();

	// Handle speech AI. Don't do AI speech if we're in scripts unless permitted by the EnableSpeakWhileScripting input.
	if ( m_NPCState == NPC_STATE_IDLE || m_NPCState == NPC_STATE_ALERT || m_NPCState == NPC_STATE_COMBAT ||
		( ( m_NPCState == NPC_STATE_SCRIPT ) && CanSpeakWhileScripting() ) )
	{
		DoCustomSpeechAI();
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Leader::UseFunc( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_bDontUseSemaphore = true;
	SpeakIfAllowed( TLK_USE );
	m_bDontUseSemaphore = false;

	m_OnPlayerUse.FireOutput( pActivator, pCaller );
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_Leader, CNPC_Leader )

AI_END_CUSTOM_NPC()
