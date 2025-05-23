//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef NPC_ASSASSIN_H
#define NPC_ASSASSIN_H
#ifdef _WIN32
#pragma once
#endif

#include "AI_BaseNPC.h"
#include "Sprite.h"
#include "SpriteTrail.h"
#include "soundent.h"

//Eye states
enum eyeState_t
{
	ASSASSIN_EYE_SEE_TARGET = 0,		//Sees the target, bright and big
	ASSASSIN_EYE_SEEKING_TARGET,	//Looking for a target, blinking (bright)
	ASSASSIN_EYE_ACTIVE,			//Actively looking
	ASSASSIN_EYE_DORMANT,			//Not active
	ASSASSIN_EYE_DEAD,				//Completely invisible
};
//=========================================================
// Anim Events	
//=========================================================
#define	ASSASSIN_AE_FIRE_PISTOL_RIGHT	( 1 )
#define	ASSASSIN_AE_FIRE_PISTOL_LEFT	( 2 )
#define	ASSASSIN_AE_KICK_HIT			( 3 )
//=========================================================
// Assassin activities
//=========================================================
int ACT_ASSASSIN_FLIP_LEFT;
int ACT_ASSASSIN_FLIP_RIGHT;
int ACT_ASSASSIN_FLIP_BACK;
int ACT_ASSASSIN_FLIP_FORWARD;
int ACT_ASSASSIN_PERCH;

//=========================================================
// Flip types
//=========================================================
enum 
{
	FLIP_LEFT,
	FLIP_RIGHT,
	FLIP_FORWARD,
	FLIP_BACKWARD,
	NUM_FLIP_TYPES,
};

//=========================================================
// Private conditions
//=========================================================
enum Assassin_Conds
{
	COND_ASSASSIN_ENEMY_TARGETTING_ME = LAST_SHARED_CONDITION,
};

//=========================================================
// Assassin schedules
//=========================================================
enum
{
	SCHED_ASSASSIN_FIND_VANTAGE_POINT = LAST_SHARED_SCHEDULE,
	SCHED_ASSASSIN_EVADE,
	SCHED_ASSASSIN_STALK_ENEMY,
	SCHED_ASSASSIN_LUNGE,
};

//=========================================================
// Assassin tasks
//=========================================================
enum 
{
	TASK_ASSASSIN_GET_PATH_TO_VANTAGE_POINT = LAST_SHARED_TASK,
	TASK_ASSASSIN_EVADE,
	TASK_ASSASSIN_SET_EYE_STATE,
	TASK_ASSASSIN_LUNGE,
};
//=========================================================
//=========================================================
class CNPC_Assassin : public CAI_BaseNPC
{
public:
	DECLARE_CLASS( CNPC_Assassin, CAI_BaseNPC );
	// DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CNPC_Assassin( void );
	
	Class_T		Classify( void )			{ return CLASS_COMBINE;	}
	int			GetSoundInterests ( void )	{ return (SOUND_WORLD|SOUND_COMBAT|SOUND_PLAYER);	}

	int			SelectSchedule ( void );
	int			MeleeAttack1Conditions ( float flDot, float flDist );
	int			RangeAttack1Conditions ( float flDot, float flDist );
	int			RangeAttack2Conditions ( float flDot, float flDist );

	void		Precache( void );
	void		Spawn( void );
	void		PrescheduleThink( void );
	void		HandleAnimEvent( animevent_t *pEvent );
	void		StartTask( const Task_t *pTask );
	void		RunTask( const Task_t *pTask );
	void		OnScheduleChange( void );
	void		GatherEnemyConditions( CBaseEntity *pEnemy );
	void		BuildScheduleTestBits( void );
	void		Event_Killed( const CTakeDamageInfo &info );

	bool		FValidateHintType ( CAI_Hint *pHint );
	bool		IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const;
	bool		MovementCost( int moveType, const Vector &vecStart, const Vector &vecEnd, float *pCost );

	float		MaxYawSpeed( void );

	const Vector &GetViewOffset( void );

private:

	void		SetEyeState( eyeState_t state );
	void		FirePistol( int hand );
	bool		CanFlip( int flipType, Activity &activity, const Vector *avoidPosition );

	int			m_nNumFlips;
	int			m_nLastFlipType;
	float		m_flNextFlipTime;	//Next earliest time the assassin can flip again
	float		m_flNextLungeTime;
	float		m_flNextShotTime;

	bool		m_bEvade;
	bool		m_bAggressive;		// Sets certain state, including whether or not her eye is visible
	bool		m_bBlinkState;

	CSprite				*m_pEyeSprite;
	CSpriteTrail		*m_pEyeTrail;

	DEFINE_CUSTOM_AI;
};


#endif // NPC_ASSASSIN_H
