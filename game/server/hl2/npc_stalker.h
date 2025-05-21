#ifndef NPC_STALKER_H
#define NPC_STALKER_H
#ifdef _WIN32
#pragma once
#endif

#include "soundent.h"
#include "game.h"
#include "beam_shared.h"
#include "Sprite.h"
#include "npcevent.h"
#include "ai_hull.h"
#include "ai_default.h"
#include "ai_node.h"
#include "ai_network.h"
#include "ai_hint.h"
#include "ai_link.h"
#include "ai_waypoint.h"
#include "ai_navigator.h"
#include "ai_senses.h"
#include "ai_squadslot.h"
#include "ai_squad.h"
#include "ai_memory.h"
#include "ai_tacticalservices.h"
#include "ai_moveprobe.h"

#include "ai_basenpc.h"
#include "entityoutput.h"
#include "ai_behavior.h"
#include "ai_behavior_actbusy.h"
#include "scriptedtarget.h"

#include "npc_talker.h"
#include "activitylist.h"
#include "bitstring.h"
#include "decals.h"
#include "player.h"
#include "entitylist.h"
#include "ndebugoverlay.h"
#include "ai_interactions.h"
#include "animation.h"
#include "scriptedtarget.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "world.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar	sk_stalker_health( "sk_stalker_health","0");
ConVar	sk_stalker_melee_dmg( "sk_stalker_melee_dmg","0");
ConVar  sk_stalker_beam_power( "sk_stalker_beam_power", "1" ); 


extern void		SpawnBlood(Vector vecSpot, const Vector &vAttackDir, int bloodColor, float flDamage);

//#define		STALKER_DEBUG
#define	MIN_STALKER_FIRE_RANGE		128
#define	MAX_STALKER_FIRE_RANGE		1024 // 3600 feet.
#define	STALKER_LASER_ATTACHMENT	1
#define	STALKER_TRIGGER_DIST		200	// Enemy dist. that wakes up the stalker
#define	STALKER_SENTENCE_VOLUME		(float)0.35
#define STALKER_LASER_DURATION		5
#define STALKER_LASER_RECHARGE		2
#define STALKER_PLAYER_AGGRESSION	0

enum StalkerBeamPower_e
{
	STALKER_BEAM_LOW,
	STALKER_BEAM_MED,
	STALKER_BEAM_HIGH,
};

//Animation events
#define STALKER_AE_MELEE_HIT			1

//=========================================================
// Private activities.
//=========================================================
static int ACT_STALKER_WORK = 0;

//=========================================================
// Stalker schedules
//=========================================================
enum
{
	SCHED_STALKER_CHASE_ENEMY = LAST_SHARED_SCHEDULE,
	SCHED_STALKER_RANGE_ATTACK,	
	SCHED_STALKER_MELEE_ATTACK,
};

//=========================================================
// Stalker Tasks
//=========================================================
enum 
{
	TASK_STALKER_ZIGZAG = LAST_SHARED_TASK,
	TASK_STALKER_SCREAM,
	TASK_STALKER_MELEE_ATTACK1,
};

// -----------------------------------------------
//	> Squad slots
// -----------------------------------------------
enum SquadSlot_T
{
	SQUAD_SLOT_CHASE_ENEMY_1	= LAST_SHARED_SQUADSLOT,
	SQUAD_SLOT_CHASE_ENEMY_2,
};

//=====================================================
//
//=====================================================

class CBeam;
class CSprite;
class CScriptedTarget;

typedef CAI_BehaviorHost<CAI_BaseNPC> CAI_BaseStalker;

class CNPC_Stalker : public CAI_BaseStalker
{
	DECLARE_CLASS( CNPC_Stalker, CAI_BaseStalker );

public:
	float			m_flNextAttackSoundTime;
	float			m_flNextBreatheSoundTime;
	float			m_flNextScrambleSoundTime;
	float			m_flNextNPCThink;

	// ------------------------------
	//	Laser Beam
	// ------------------------------
	int					m_eBeamPower;
	Vector				m_vLaserDir;
	Vector				m_vLaserTargetPos;
	float				m_fBeamEndTime;
	float				m_fBeamRechargeTime;
	float				m_fNextDamageTime;
	float				m_nextSmokeTime;
	float				m_bPlayingHitWall;
	float				m_bPlayingHitFlesh;
	CBeam*				m_pBeam;
	CSprite*			m_pLightGlow;
	int					m_iPlayerAggression;
	float				m_flNextScreamTime;

	void				KillAttackBeam(void);
	void				DrawAttackBeam(void);
	void				CalcBeamPosition(void);
	Vector				LaserStartPosition(Vector vStalkerPos);

	Vector				m_vLaserCurPos;			// Last position successfully burned
	bool				InnateWeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions );
	
	// ------------------------------
	//	Scripted Target Burns
	// ------------------------------
	CScriptedTarget*	m_pScriptedTarget;		// My current scripted target
	void				SetScriptedTarget( CScriptedTarget *pScriptedTarget );

	//Vector				m_vLaserCurPos;			// Last position successfully burned
	//bool				InnateWeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions );
	Vector				ScriptedBurnPosition(void);

	// ------------------------------
	//	Dormancy
	// ------------------------------
	CAI_Schedule*	WakeUp(void);
	void			GoDormant(void);

public:
	void			Spawn( void );
	void			Precache( void );
	bool			CreateBehaviors();
	float			MaxYawSpeed( void );
	Class_T			Classify ( void );

	void			PrescheduleThink();

	bool			IsValidEnemy( CBaseEntity *pEnemy );
	
	void			StartTask( const Task_t *pTask );
	void			RunTask( const Task_t *pTask );
	virtual int		SelectSchedule ( void );
	virtual int		TranslateSchedule( int scheduleType );
	int				OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void			OnScheduleChange();

	void			StalkerThink(void);
	void			NotifyDeadFriend( CBaseEntity *pFriend );

	int				MeleeAttack1Conditions ( float flDot, float flDist );
	int				RangeAttack1Conditions ( float flDot, float flDist );
	void			HandleAnimEvent( animevent_t *pEvent );

	bool			FValidateHintType(CAI_Hint *pHint);
	Activity		GetHintActivity( short sHintType, Activity HintsActivity );
	float			GetHintDelay( short sHintType );

	void			IdleSound( void );
	void			DeathSound( const CTakeDamageInfo &info );
	void			PainSound( const CTakeDamageInfo &info );

	void			Event_Killed( const CTakeDamageInfo &info );
	void			DoSmokeEffect( const Vector &position );

	void			AddZigZagToPath(void);
	void			StartAttackBeam();
	void			UpdateAttackBeam();
	
	void			InitArmBeam(void);
	void			KillArmBeam(void);
	void			InputToggleArmBeam(inputdata_t &inputdata);
	
	Activity		NPC_TranslateActivity( Activity baseAct );

	CNPC_Stalker(void);

	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;

private:
	CAI_ActBusyBehavior		m_ActBusyBehavior;
	
protected:
	float			m_lastHurtTime;
	// ==================
	// Attack 
	// ==================
	CBeam*			m_pArmBeam;
	bool			m_bArmBeamEnable;
};

#endif // NPC_CREMATOR_H
//-----------------------------------------------------------------------------
