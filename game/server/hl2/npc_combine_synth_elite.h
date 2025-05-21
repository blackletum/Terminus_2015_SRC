#include "cbase.h"
#include "basehlcombatweapon.h"
#include "basecombatcharacter.h"
#include "AI_Task.h"
#include "AI_Default.h"
#include "AI_Schedule.h"
#include "AI_Hull.h"
#include "AI_Motor.h"
#include "AI_Memory.h"
#include "bitstring.h"
#include "activitylist.h"
#include "game.h"
#include "gamerules.h"
#include "npcevent.h"
#include "Player.h"
#include "EntityList.h"
#include "AI_Interactions.h"
#include "soundent.h"
#include "Gib.h"
#include "shake.h"
#include "Sprite.h"
#include "explode.h"
#include "grenade_homer.h"
#include "ndebugoverlay.h"
#include "AI_BaseNPC.h"
#include "soundenvelope.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"

#if defined( _WIN32 )
#pragma once
#endif

#include "tier0/memdbgon.h"

extern void CreateConcussiveBlast( const Vector &origin, const Vector &surfaceNormal, CBaseEntity *pOwner, float magnitude );
extern void UTIL_RotorWash( const Vector &origin, const Vector &direction, float maxDistance );

#define	COMBINEGUARD_MODEL	"models/combine_synth_elite.mdl"
#define CGUARD_MSG_SHOT			1
#define CGUARD_MSG_SHOT_START	2

ConVar sk_combine_synth_elite_health( "sk_combine_synth_elite_health", "500" );
ConVar sk_combine_synth_elite_armor_health( "sk_combine_synth_elite_armor_health", "0" );
ConVar sk_combine_synth_elite_shove( "sk_combine_synth_elite_shove", "0" );
ConVar sk_combine_synth_elite_shove_npc( "sk_combine_synth_elite_shove_npc", "0" );

#define	COMBINEGUARD_MELEE1_RANGE		92
#define	COMBINEGUARD_MELEE1_CONE		0.5f
#define	COMBINEGUARD_RANGE1_RANGE		2048
#define	COMBINEGUARD_RANGE1_MINRANGE	384
#define	COMBINEGUARD_RANGE1_CONE		0.0f
#define	CGUARD_GLOW_TIME				0.5f
#define CGUARD_ARMORPIECE_HEALTH		sk_combine_synth_elite_armor_health.GetInt();

enum CombineGuardSchedules 
{	
	SCHED_CGUARD_RANGE_ATTACK1,
	SCHED_CGUARD_WARMUP_ATTACK1,
	SCHED_CGUARD_MELEE_ATTACK1 = LAST_SHARED_SCHEDULE,	
	SCHED_CGUARD_RUN_TO_MELEE,
	SCHED_COMBINEGUARD_CLOBBERED,
	SCHED_COMBINEGUARD_TOPPLE,
	SCHED_COMBINEGUARD_HELPLESS,
};
enum CombineGuardTasks 
{	
//	TASK_CGUARD_RANGE_ATTACK1,
//	TASK_CGUARD_WARMUP_ATTACK1,
	TASK_CGUARD_MELEE_ATTACK1 = LAST_SHARED_TASK,
	TASK_COMBINEGUARD_SET_BALANCE,
};
enum CombineGuardConditions
{	
	COND_COMBINEGUARD_CLOBBERED = LAST_SHARED_CONDITION,
};
int	ACT_CGUARD_IDLE_TO_ANGRY;
int ACT_COMBINEGUARD_CLOBBERED;
int ACT_COMBINEGUARD_TOPPLE;
int ACT_COMBINEGUARD_GETUP;
int ACT_COMBINEGUARD_HELPLESS;
// anim events
#define	CGUARD_AE_SHOVE					( 11 )
#define	CGUARD_AE_FIRE					( 12 )
#define	CGUARD_AE_FIRE_START			( 13 )
#define	CGUARD_AE_GLOW					( 14 )
#define CGUARD_AE_LEFTFOOT				( 20 ) // footsteps
#define CGUARD_AE_RIGHTFOOT				( 21 ) // footsteps
#define CGUARD_AE_SHAKEIMPACT			( 22 ) // hard body impact that makes screenshake.

enum CombineGuardArmorPieces
{
	CGUARD_BG_INVALID = -1,
	CGUARD_BG_MAINBODY,
	CGUARD_BG_GUARDGUN,
	CGUARD_BG_LSHOULDER,
	CGUARD_BG_LELBOW,
	CGUARD_BG_LFOREARM,
	CGUARD_BG_LSHIELD,

	CGUARD_BG_RSHOULDER,
	CGUARD_BG_RBICEPS,
	CGUARD_BG_RKNEE,
	CGUARD_BG_LKNEE,

	NUM_CGUARD_BODYGROUPS,
};

enum CombineGuardRefPoints
{
	CGUARD_REF_INVALID = -1,
	CGUARD_REF_CORPUS,
	CGUARD_REF_GUARDGUN,
	CGUARD_REF_LSHOULDER,
	CGUARD_REF_LELBOW,
	CGUARD_REF_LFOREARM,
	CGUARD_REF_LSHIELD,
	CGUARD_REF_RSHOULDER,
	CGUARD_REF_RBICEPS,
	CGUARD_REF_RKNEE,
	CGUARD_REF_LKNEE,

	NUM_CGUARD_REFPOINTS,
};

struct armorPiece_t
{
	DECLARE_DATADESC();

	bool	m_bDestroyed;
	int		m_iHealth;
};

class CNPC_Combine_synth_elite : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_Combine_synth_elite, CAI_BaseNPC );
public:
	DECLARE_SERVERCLASS();
	CNPC_Combine_synth_elite( void );

	int				OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void			TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	void			Event_Killed(const CTakeDamageInfo &info);
//	int				TranslateSchedule( int type );
	int				MeleeAttack1Conditions( float flDot, float flDist );
//	int				RangeAttack1Conditions( float flDot, float flDist );

	void			Precache( void );
	void			Spawn( void );
	void			PrescheduleThink( void );
	void			HandleAnimEvent( animevent_t *pEvent );
	void			StartTask( const Task_t *pTask );
	void			RunTask( const Task_t *pTask );
	
	
	float			MaxYawSpeed( void );

	Class_T			Classify( void ) { return CLASS_COMBINE; }
	Activity		NPC_TranslateActivity( Activity baseAct );

	virtual int		SelectSchedule( void );

	DECLARE_DATADESC();

private:
	bool			m_fOffBalance;
	float			m_flNextClobberTime;
	float			m_flShotDelay;

	void			Shove( void );
//	void			FireRangeWeapon( void );
//	void			WarmupRangeWeapon( void );
	
	bool			IsArmorPiece( int iArmorPiece );
	
	void			InitArmorPieces( void );
	void			DamageArmorPiece( int pieceID, float damage, const Vector &vecOrigin, const Vector &vecDir );
	void			DestroyArmorPiece( int pieceID );

	bool			AimGunAt( CBaseEntity *pEntity, float flInterval );

	float			m_flGlowTime;
	float			m_flLastRangeTime;
	float			m_flRangeAnimTime;

	float			m_aimYaw;
	float			m_aimPitch;

	int				m_YawControl;
	int				m_PitchControl;
	int				m_MuzzleAttachment;
	int				m_iArmorHealth;
	int				GetReferencePointForBodyGroup( int bodyGroup );

	armorPiece_t	m_armorPieces[NUM_CGUARD_BODYGROUPS];

	EHANDLE			m_hCannonTarget;
	Vector			m_blastHit;
	Vector			m_blastNormal;

	DEFINE_CUSTOM_AI;
};

IMPLEMENT_SERVERCLASS_ST(CNPC_Combine_synth_elite, DT_NPC_combine_synth_elite)
END_SEND_TABLE()

BEGIN_DATADESC_NO_BASE( armorPiece_t )
	DEFINE_FIELD( m_bDestroyed,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_iHealth,		FIELD_INTEGER ),
END_DATADESC()

BEGIN_DATADESC( CNPC_Combine_synth_elite )
	DEFINE_FIELD( m_fOffBalance, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flNextClobberTime, FIELD_TIME ),
	DEFINE_FIELD( m_flShotDelay, FIELD_FLOAT ),
	DEFINE_FIELD( m_flGlowTime,	FIELD_TIME ),
	DEFINE_FIELD( m_flLastRangeTime, FIELD_TIME ),
	DEFINE_FIELD( m_flRangeAnimTime, FIELD_TIME ),
	DEFINE_FIELD( m_aimYaw,		FIELD_FLOAT ),
	DEFINE_FIELD( m_aimPitch,	FIELD_FLOAT ),
	DEFINE_FIELD( m_YawControl,	FIELD_INTEGER ),
	DEFINE_FIELD( m_PitchControl, FIELD_INTEGER ),
	DEFINE_FIELD( m_MuzzleAttachment, FIELD_INTEGER ),
	DEFINE_FIELD( m_hCannonTarget,		FIELD_EHANDLE ),
	DEFINE_EMBEDDED_AUTO_ARRAY( m_armorPieces ),
END_DATADESC()

CNPC_Combine_synth_elite::CNPC_Combine_synth_elite( void )
{
};