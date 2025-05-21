#ifndef NPC_ALIEN_ASSASSIN_H
#define NPC_ALIEN_ASSASSIN_H
#ifdef _WIN32
#pragma once
#endif

#include "npcevent.h"
#include "ai_hull.h"
#include "AI_BaseNPC.h"
#include "AI_Task.h"
#include "AI_Default.h"
#include "AI_Schedule.h"
#include "ai_node.h"
#include "ai_network.h"
#include "ai_hint.h"
#include "ai_link.h"
#include "ai_waypoint.h"
#include "ai_navigator.h"
#include "AI_Motor.h"
#include "ai_senses.h"
#include "ai_squadslot.h"
#include "ai_squad.h"
#include "ai_memory.h"
#include "ai_tacticalservices.h"
#include "ai_moveprobe.h"
#include "ai_utils.h"
#include "datamap.h"
#include "basecombatcharacter.h"
#include "basehlcombatweapon.h"

#include "soundent.h"
#include "activitylist.h"
#include "engine/IEngineSound.h"

#include "particle_parse.h"
#include "te_particlesystem.h"
#include "te_effect_dispatch.h"

#include "IEffects.h"

#include "ammodef.h" // included for ammo-related damage table

#include "grenade_hopwire.h" // for an npc-side hopwire attack.

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define ASSASSIN_STAB_DIST		64.0
#define ASSASSIN_THROWKNIFE_MAX_RANGE	1024
#define ASSASSIN_RANGE1_DIST_MIN		80
#define ASSASSIN_RANGE1_DIST_MAX		750
#define ASSASSIN_HOPWIRE_SPEED			600

#define ASSASSIN_SKIN_CALM		0
#define ASSASSIN_SKIN_ALERT		1
#define ASSASSIN_SKIN_DEAD		2

#define ASSASSIN_SMOKE_TIME		50.0f

#define ASSASSIN_SACKTICK_NEST_RADIUS	60

#define SF_ASSASSIN_NEST	( 1<<16 )
#define SF_ASSASSIN_FERAL	( 1<<17 )

ConVar sk_assassin_health( "sk_assassin_health", "0" );
ConVar sk_assassin_feral_health( "sk_assassin_feral_health", "0" );
ConVar sk_assassin_stab	( "sk_assassin_stab", "0" );
ConVar sk_assassin_stab_npc	( "sk_assassin_stab_npc", "0" );
ConVar sk_assassin_thrown	( "sk_assassin_thrown", "0" );
ConVar sk_assassin_regen_rate( "sk_assassin_regen_rate", "1.0", FCVAR_REPLICATED );
ConVar sk_assassin_regen_period( "sk_assassin_regen_period", "10.0", FCVAR_REPLICATED );

//=====================================================
//
//=====================================================
enum AssassinType_e
{
	ASSASSIN_TYPE_NEST,
	ASSASSIN_TYPE_FERAL,
	ASSASSIN_TYPE_GENERIC,
};

enum AssassinTasks
{
	TASK_ASSASSIN_ALERT = LAST_SHARED_TASK + 1,
	TASK_ASSASSIN_IDLE,
	TASK_ASSASSIN_INSPECTNEST,
	TASK_ASSASSIN_HEAR_TARGET,
	TASK_ASSASSIN_SEEPLAYER,	
	TASK_ASSASSIN_RANGE_ATTACK1,
	TASK_ASSASSIN_MELEE_ATTACK1,
	TASK_ASSASSIN_SMOKE_ATTACK1,
	TASK_ASSASSIN_SMOKE_ATTACK2,
	TASK_ASSASSIN_TRIPWIRE_ATTACK1,
//	TASK_ASSASSIN_PEST_ATTACK1,
	TASK_ASSASSIN_SET_BALANCE,
	TASK_ASSASSIN_GET_COVER_PATH,
	TASK_ASSASSIN_START_REGEN,
};
enum AssassinSchedules
{
	SCHED_ASSASSIN_CHASE = LAST_SHARED_SCHEDULE + 1,
	SCHED_ASSASSIN_RANGE_ATTACK1,
	SCHED_ASSASSIN_RANGE_ATTACK2,
	SCHED_ASSASSIN_SMOKE,
	SCHED_ASSASSIN_MELEE_ATTACK1,
	SCHED_ASSASSIN_IDLE,
	SCHED_ASSASSIN_INSPECTNEST,
};
enum AssassinConditions
{
	COND_ASSASSIN_NEED_REGEN = LAST_SHARED_CONDITION + 1,
	COND_ASSASSIN_HEAR_TARGET,
	COND_ASSASSIN_BUSY_REGEN,
};
#define	ASSASSIN_AE_THROWMINE					( 1 )
#define	ASSASSIN_AE_THROWPEST					( 3 )
#define	ASSASSIN_AE_STAB						( 4 )
#define	ASSASSIN_AE_SMOKE						( 10 )
#define	ASSASSIN_AE_FSLEFT						( 11 )
#define	ASSASSIN_AE_FSRIGHT						( 12 )
#define	ASSASSIN_AE_THROWKNIFE					( 13 )
#define ASSASSIN_AE_LAND						( 16 )

//Activity ACT_ASSASSIN_HOPWIRE;
Activity ACT_ASSASSIN_SMOKE;
Activity ACT_ASSASSIN_THROWKNIFE;
Activity ACT_ASSASSIN_INSPECT;
Activity ACT_ASSASSIN_TOINSPECT;
Activity ACT_ASSASSIN_ENDINSPECT;

class CAssassin : public CAI_BaseNPC
{
	DECLARE_CLASS( CAssassin, CAI_BaseNPC );
public:
	
	//CAssassin( void );
//====================================================
	void Precache( void );
	void Spawn( void );
	Class_T Classify( void );
//====================================================
	void DeathSound( const CTakeDamageInfo &info );
	void AlertSound( void );
//====================================================
	int	 OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void Event_Killed( const CTakeDamageInfo &info );
//====================================================
	void LandEffect( const Vector &origin );
//====================================================
	NPC_STATE SelectIdealState ( void );

	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );
	void HandleAnimEvent( animevent_t *pEvent );
	int TranslateSchedule( int scheduleType );
	int	SelectSchedule ( void );
	int	MeleeAttack1Conditions( float flDot, float flDist );
//	int	RangeAttack2Conditions( float flDot, float flDist );
	int	RangeAttack1Conditions( float flDot, float flDist );
//====================================================
	void MeleeAttack( void );
	void ThrowKnife();
	void ThrowHopwire( float timer );
	void ThrowSmoke( void );
//	void DispatchSmoke( void );
	void RegenerateHopwire( void );
//====================================================
//	void FindNest( void );
//	void InspectNest( void );
//====================================================
	bool HasUsedRegen() { return m_bUsedRegen; };
	bool m_bUsedRegen;
	bool HasStartedRegen() { return m_bStartedRegen; };
	bool m_bStartedRegen;
	bool HasFinishedRegen() { return m_bFinishedRegen; };
	bool m_bFinishedRegen;
	float m_flRegenImmunityTime;
	int m_nSkin;
	
//====================================================	
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI();
//====================================================
private:

	float m_flNextHopwireTime;
	float m_flNextNPCThink;
	int m_iKnivesAmount;
	int m_iHopsAmount;
	int m_flNextInspectTime;

	CBaseEntity* m_pMyRegenerator;
};

#include "basegrenade_shared.h"

#define SMOKEGRENADE_TIMER 1.0f
#define SMOKEGRENADE_WORLDMODEL "models/Weapons/w_grenade.mdl"

class CGrenadeSmoke : public CBaseGrenade
{
public:
	DECLARE_CLASS( CGrenadeSmoke, CBaseGrenade );

	~CGrenadeSmoke( void );

public:
	void	Precache( void );
	void	Spawn( void );
	bool	CreateVPhysics( void );
	void	ThrowThink();
	void	TimerThink();
	void	Detonate( void );
	void	DetonateEffects( void );
	void	SetVelocity( const Vector &velocity, const AngularImpulse &angVelocity );
	void	Event_Killed( const CTakeDamageInfo &info );

	DECLARE_DATADESC();
};

extern short	g_sModelIndexFireball;			// (in combatweapon.cpp) holds the index for the smoke cloud

BEGIN_DATADESC( CGrenadeSmoke )

	// Function Pointers
	DEFINE_THINKFUNC( ThrowThink ),
	DEFINE_THINKFUNC( TimerThink ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( grenade_smoke, CGrenadeSmoke );

CGrenadeSmoke::~CGrenadeSmoke( void )
{
}

void CGrenadeSmoke::Precache( void )
{
	PrecacheModel( SMOKEGRENADE_WORLDMODEL );

	BaseClass::Precache();
}

void CGrenadeSmoke::Spawn( void )
{
	Precache( );

	SetModel( SMOKEGRENADE_WORLDMODEL );

	m_takedamage	= DAMAGE_NO;

	m_flDetonateTime = gpGlobals->curtime + SMOKEGRENADE_TIMER;

	SetSize( -Vector(4,4,4), Vector(4,4,4) );
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
	SetCollisionGroup( COLLISION_GROUP_INTERACTIVE );
	CreateVPhysics();

	AddSolidFlags( FSOLID_NOT_STANDABLE );

	SetThink( &CGrenadeSmoke::TimerThink );

	BaseClass::Spawn();
}

bool CGrenadeSmoke::CreateVPhysics()
{
	// Create the object in the physics system
	VPhysicsInitNormal( SOLID_BBOX, 0, false );
	return true;
}

void CGrenadeSmoke::ThrowThink()
{
}

void CGrenadeSmoke::TimerThink() 
{
	if( gpGlobals->curtime > m_flDetonateTime )
	{
		Detonate();
		return;
	}

	SetNextThink( gpGlobals->curtime + 0.1 );
}

void CGrenadeSmoke::Detonate( void )
{
	DevMsg( "SMOKEBOOM\n" );
	DetonateEffects();
}

void CGrenadeSmoke::DetonateEffects( void )
{
}

void CGrenadeSmoke::SetVelocity( const Vector &velocity, const AngularImpulse &angVelocity )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		pPhysicsObject->AddVelocity( &velocity, &angVelocity );
	}
}
void CGrenadeSmoke::Event_Killed( const CTakeDamageInfo &info )
{
	Detonate();
};

extern ConVar sk_assassin_health;
extern ConVar sk_assassin_regen_rate;

#define ASSASSIN_MAX_REGEN_HEALTH sk_assassin_health.GetFloat() * 0.75

class CAssassinRegenerator : public CBaseEntity
{
public:
	DECLARE_CLASS( CAssassinRegenerator, CBaseEntity );

	void Precache( void );
	void Spawn( void );
	void StartRegen( CAssassin *pRegenTarget, float flRegenTime, int nStartingHealth, bool m_bUseEffects );
	void RegenThink( void );
	void StopRegen( void );
//	void ApplyRegenEffects( void ); // TODO: Particle/shader effects on target
//	void DestroyRegenEffects( void ); // called by StopRegen()
//	void InputSetRegenTarget( inputdata_t &inputdata );

	int m_nStartingHealth; // target's health at the start of healing
	float m_flRegenTime;

private:
	CAssassin* m_pRegenTarget;
	
	DECLARE_DATADESC();
};

BEGIN_DATADESC( CAssassinRegenerator )

	DEFINE_FIELD( m_pRegenTarget, FIELD_CLASSPTR ),
	DEFINE_FIELD( m_flRegenTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_nStartingHealth, FIELD_INTEGER ),
	DEFINE_THINKFUNC( RegenThink ),
//	DEFINE_INPUTFUNC( FIELD_CLASSPTR, "SetRegenTarget", InputSetRegenTarget

END_DATADESC()

LINK_ENTITY_TO_CLASS( npc_assassin_regenerator, CBaseEntity );
void CAssassinRegenerator::Precache( void )
{
	BaseClass::Precache();
}
void CAssassinRegenerator::Spawn( void )
{
	m_pRegenTarget = NULL;
	m_flRegenTime = 10; //default 10 sec
	m_nStartingHealth = 0; //default 0 hp, meaning full regen to ASSASSIN_MAX_REGEN_HEALTH
	m_takedamage = DAMAGE_NO;

	Precache();

	BaseClass::Spawn();
}
void CAssassinRegenerator::StartRegen( CAssassin *pRegenTarget, float flRegenTime, int nStartingHealth, bool m_bUseEffects )
{
	m_pRegenTarget = pRegenTarget;
	m_flRegenTime = flRegenTime;
	m_nStartingHealth = nStartingHealth;
	SetThink( &CAssassinRegenerator::RegenThink );
	SetNextThink( gpGlobals->curtime + 0.025f );
}
void CAssassinRegenerator::RegenThink( void )
{
	if( m_pRegenTarget )
	{
		if( m_pRegenTarget->GetHealth() >= ASSASSIN_MAX_REGEN_HEALTH || !m_pRegenTarget->IsAlive() )
		{
			SetThink( NULL );
			SetNextThink( gpGlobals->curtime + 0.025f );
			StopRegen();
		}
		else
		{
			m_pRegenTarget->m_iHealth += ((ASSASSIN_MAX_REGEN_HEALTH - m_nStartingHealth)/m_flRegenTime);
			//The healing rate is a function of time needed to heal the set amount of health. 
			//Said amount is equal to the max. health the assassin is allowed to regenerate, minus the assassin's current health.
			//Meaning that e.g. if the target should heal up to 150 hp and starts regeneration at 50 hp, it'll have
			//to heal 150 - 100 hp in X seconds (default 10). The process' rate can be scaled by the rate convar, while
			//the starting health value is taken from the target, the healing period is set in sk_assassin_regen_period convar.

			SetNextThink( gpGlobals->curtime + 1 / sk_assassin_regen_rate.GetFloat() );
		}
	}
	else
		StopRegen();
}
void CAssassinRegenerator::StopRegen( void )
{
	if( m_pRegenTarget )
	{
		m_pRegenTarget->m_nBody = 0;
		m_pRegenTarget->RemoveEFlags( EF_NOSHADOW );
		m_pRegenTarget->m_bFinishedRegen = 1;
	}
//	UTIL_Remove( this );
};

#endif // NPC_ALIEN_ASSASSIN_H