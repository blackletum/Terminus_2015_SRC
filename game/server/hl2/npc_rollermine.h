//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef NPC_ROLLERMINE_H
#define NPC_ROLLERMINE_H
#ifdef _WIN32
#pragma once
#endif
#include "cbase.h"
#include "ai_default.h"
#include "ai_task.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_squadslot.h"
#include "ai_basenpc.h"
#include "ai_navigator.h"
#include "ai_interactions.h"
#include "ndebugoverlay.h"
#include "explode.h"
#include "bitstring.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "decals.h"
#include "antlion_dust.h"
#include "ai_memory.h"
#include "ai_squad.h"
#include "ai_senses.h"
#include "beam_shared.h"
#include "iservervehicle.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "physics_saverestore.h"
#include "vphysics/constraints.h"
#include "vehicle_base.h"
#include "eventqueue.h"
#include "te_effect_dispatch.h"
#include "npc_rollermine.h"
#include "func_break.h"
#include "soundenvelope.h"
#include "mapentities.h"
#include "RagdollBoogie.h"
#include "physics_collisionevent.h"


//------------------------------------
// Spawnflags
//------------------------------------
#define SF_ROLLERMINE_FRIENDLY		(1 << 16)
#define SF_ROLLERMINE_PROP_COLLISION		(1 << 17)

enum rollingsoundstate_t { ROLL_SOUND_NOT_READY = 0, ROLL_SOUND_OFF, ROLL_SOUND_CLOSED, ROLL_SOUND_OPEN };


//-----------------------------------------------------------------------------
// CRollerController implementation
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Purpose: This class only implements the IMotionEvent-specific behavior
//			It keeps track of the forces so they can be integrated
//-----------------------------------------------------------------------------
class CRollerController : public IMotionEvent
{
	DECLARE_SIMPLE_DATADESC();

public:
	IMotionEvent::simresult_e Simulate(IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular);

	AngularImpulse	m_vecAngular;
	Vector			m_vecLinear;

	void Off(void) { m_fIsStopped = true; }
	void On(void) { m_fIsStopped = false; }

	bool IsOn(void) { return !m_fIsStopped; }

private:
	bool	m_fIsStopped;
};

//=========================================================
//=========================================================
class CNPC_RollerMine : public CNPCBaseInteractive<CAI_BaseNPC>, public CDefaultPlayerPickupVPhysics
{
	DECLARE_CLASS(CNPC_RollerMine, CNPCBaseInteractive<CAI_BaseNPC>);
	DECLARE_SERVERCLASS();

public:

	CNPC_RollerMine(void) { m_bTurnedOn = true; m_bUniformSight = false; }
	~CNPC_RollerMine(void);

	void	Spawn(void);
	bool	CreateVPhysics();
	void	RunAI();
	void	StartTask(const Task_t *pTask);
	void	RunTask(const Task_t *pTask);
	void	SpikeTouch(CBaseEntity *pOther);
	void	ShockTouch(CBaseEntity *pOther);
	void	CloseTouch(CBaseEntity *pOther);
	void	EmbedTouch(CBaseEntity *pOther);
	float	GetAttackDamageScale(CBaseEntity *pVictim);
	void	VPhysicsCollision(int index, gamevcollisionevent_t *pEvent);
	void	Precache(void);
	void	OnPhysGunPickup(CBasePlayer *pPhysGunUser, PhysGunPickup_t reason);
	void	OnPhysGunDrop(CBasePlayer *pPhysGunUser, PhysGunDrop_t Reason);
	void	StopLoopingSounds(void);
	void	PrescheduleThink();
	bool	ShouldSavePhysics()	{ return true; }
	void	OnRestore();
	void	Bury(trace_t *tr);
	bool	QuerySeeEntity(CBaseEntity *pSightEnt, bool bOnlyHateOrFearIfNPC = false);

	int		RangeAttack1Conditions(float flDot, float flDist);
	int		SelectSchedule(void);
	int		TranslateSchedule(int scheduleType);
	int		GetHackedIdleSchedule(void);

	bool	OverrideMove(float flInterval) { return true; }
	bool	IsValidEnemy(CBaseEntity *pEnemy);
	bool	IsPlayerVehicle(CBaseEntity *pEntity);
	bool	IsShocking() { return gpGlobals->curtime < m_flShockTime ? true : false; }
	void	UpdateRollingSound();
	void	UpdatePingSound();
	void	StopRollingSound();
	void	StopPingSound();
	float	RollingSpeed();
	float	GetStunDelay();
	void	EmbedOnGroundImpact();
	void	UpdateEfficiency(bool bInPVS)	{ SetEfficiency((GetSleepState() != AISS_AWAKE) ? AIE_DORMANT : AIE_NORMAL); SetMoveEfficiency(AIME_NORMAL); }
	void	DrawDebugGeometryOverlays()
	{
		if (m_debugOverlays & OVERLAY_BBOX_BIT)
		{
			float dist = GetSenses()->GetDistLook();
			Vector range(dist, dist, 64);
			NDebugOverlay::Box(GetAbsOrigin(), -range, range, 255, 0, 0, 0, 0);
		}
		BaseClass::DrawDebugGeometryOverlays();
	}
	// UNDONE: Put this in the qc file!
	Vector EyePosition()
	{
		// This takes advantage of the fact that the system knows
		// that the abs origin is at the center of the rollermine
		// and that the OBB is actually world-aligned despite the
		// fact that SOLID_VPHYSICS is being used
		Vector eye = CollisionProp()->GetCollisionOrigin();
		eye.z += CollisionProp()->OBBMaxs().z;
		return eye;
	}

	int		OnTakeDamage(const CTakeDamageInfo &info);
	void	TraceAttack(const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator);

	Class_T	Classify()
	{
		if (!m_bTurnedOn)
			return CLASS_NONE;

		//About to blow up after being hacked so do damage to the player.
		if (m_bHackedByAlyx && (m_flPowerDownDetonateTime > 0.0f && m_flPowerDownDetonateTime <= gpGlobals->curtime))
			return CLASS_COMBINE;

		return (m_bHeld || m_bHackedByAlyx) ? CLASS_HACKED_ROLLERMINE : CLASS_COMBINE;
	}

	virtual bool ShouldGoToIdleState()
	{
		return gpGlobals->curtime > m_flGoIdleTime ? true : false;
	}

	virtual	void OnStateChange(NPC_STATE OldState, NPC_STATE NewState);

	// Vehicle interception
	bool	EnemyInVehicle(void);
	float	VehicleHeading(CBaseEntity *pVehicle);

	NPC_STATE SelectIdealState();

	// Vehicle sticking
	void		StickToVehicle(CBaseEntity *pOther);
	void		AnnounceArrivalToOthers(CBaseEntity *pOther);
	void		UnstickFromVehicle(void);
	CBaseEntity *GetVehicleStuckTo(void);
	int			CountRollersOnMyVehicle(CUtlVector<CNPC_RollerMine*> *pRollerList);
	void		InputConstraintBroken(inputdata_t &inputdata);
	void		InputRespondToChirp(inputdata_t &inputdata);
	void		InputRespondToExplodeChirp(inputdata_t &inputdata);
	void		InputJoltVehicle(inputdata_t &inputdata);
	void		InputTurnOn(inputdata_t &inputdata);
	void		InputTurnOff(inputdata_t &inputdata);
	void		InputPowerdown(inputdata_t &inputdata);

	void		PreventUnstickUntil(float flTime) { m_flPreventUnstickUntil = flTime; }

	virtual unsigned int	PhysicsSolidMaskForEntity(void) const;

	void		SetRollerSkin(void);
	void	NotifyInteraction(CAI_BaseNPC *pUser);

	COutputEvent m_OnPhysGunDrop;
	COutputEvent m_OnPhysGunPickup;
	void CommandMoveToLocation(const Vector &vecLocation, CBaseEntity *pCommandEntity);
protected:
	DEFINE_CUSTOM_AI;
	DECLARE_DATADESC();

	bool	BecomePhysical();
	void	WakeNeighbors();
	bool	WakeupMine(CAI_BaseNPC *pNPC);

	void	Open(void);
	void	Close(void);
	void	Explode(void);
	void	PreDetonate(void);
	void	Hop(float height);

	void	ShockTarget(CBaseEntity *pOther);

	bool	IsActive() { return m_flActiveTime > gpGlobals->curtime ? false : true; }

	// INPCInteractive Functions
	virtual bool	CanInteractWith(CAI_BaseNPC *pUser) { return true; }
	virtual	bool	HasBeenInteractedWith()	{ return m_bHackedByAlyx; }

	CSoundPatch					*m_pRollSound;
	CSoundPatch					*m_pPingSound;

	CRollerController			m_RollerController;
	IPhysicsMotionController	*m_pMotionController;

	float	m_flSeeVehiclesOnlyBeyond;
	float	m_flChargeTime;
	float	m_flGoIdleTime;
	float	m_flShockTime;
	float	m_flForwardSpeed;
	int		m_iSoundEventFlags;
	rollingsoundstate_t m_rollingSoundState;

	CNetworkVar(bool, m_bIsOpen);
	CNetworkVar(float, m_flActiveTime);	//If later than the current time, this will force the mine to be active

	bool	m_bHeld;		//Whether or not the player is holding the mine
	EHANDLE m_hVehicleStuckTo;
	float	m_flPreventUnstickUntil;
	float	m_flNextHop;
	bool	m_bStartBuried;
	bool	m_bBuried;
	bool	m_bIsPrimed;
	bool	m_wakeUp;
	bool	m_bEmbedOnGroundImpact;
	CNetworkVar(bool, m_bHackedByAlyx);

	// Constraint used to stick us to a vehicle
	IPhysicsConstraint *m_pConstraint;

	bool	m_bTurnedOn;
	bool	m_bUniformSight;

	CNetworkVar(bool, m_bPowerDown);
	float	m_flPowerDownTime;
	float	m_flPowerDownDetonateTime;

	static string_t gm_iszDropshipClassname;
private:
	Vector m_vecCommandLocation;
	EHANDLE m_hCommandEntity;
	CBasePlayer *m_pMaster;
};


bool NPC_Rollermine_IsRollermine( CBaseEntity *pEntity );
CBaseEntity *NPC_Rollermine_DropFromPoint( const Vector &originStart, CBaseEntity *pOwner, const char *pszTemplate );

#endif // NPC_ROLLERMINE_H
