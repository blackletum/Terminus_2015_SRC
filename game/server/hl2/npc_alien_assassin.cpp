#include "cbase.h"
#include "npc_alien_assassin.h"

LINK_ENTITY_TO_CLASS( npc_alien_assassin, CAssassin );

BEGIN_DATADESC( CAssassin )

DEFINE_FIELD( m_bUsedRegen, FIELD_BOOLEAN ),
DEFINE_FIELD( m_bStartedRegen, FIELD_BOOLEAN ),
DEFINE_FIELD( m_bFinishedRegen, FIELD_BOOLEAN ),
// need separate bools m_bUsedRegen and m_bFinished regen because
// m_bUsedRegen is changed to 1 when assassin initiates regen so he won't start
// it again if he's hurt during regeneration, and m_bFinishedRegen
// is changed to 1 whether regen is actually finished or aborted.
DEFINE_FIELD( m_flNextInspectTime, FIELD_TIME ),
DEFINE_FIELD( m_flNextHopwireTime, FIELD_TIME ),
DEFINE_FIELD( m_flNextNPCThink,	FIELD_FLOAT),
DEFINE_FIELD( m_flRegenImmunityTime, FIELD_TIME ),
DEFINE_FIELD( m_pMyRegenerator, FIELD_CLASSPTR ),
DEFINE_KEYFIELD( m_iKnivesAmount, FIELD_INTEGER, "KnivesAmount" ),
DEFINE_KEYFIELD( m_iHopsAmount, FIELD_INTEGER, "HopwireAmount" ),

END_DATADESC()

extern ConVar di_hipoly_character_models;

#define ASSASSIN_START_REGEN_THRESHOLD ( sk_assassin_health.GetFloat() / 4 )

//=====================================================
//
//=====================================================
void CAssassin::Precache( void )
{
	BaseClass::Precache();
	PrecacheModel( "models/alien_assassin.mdl" );
	if( HasSpawnFlags( SF_ASSASSIN_FERAL ))
	{
		PrecacheModel( "models/alien_assassin_feral.mdl" );
	}

	UTIL_PrecacheOther( "npc_assassin_nest" );
	UTIL_PrecacheOther( "weapon_hopwire" );
	UTIL_PrecacheOther( "grenade_smoke" );
	UTIL_PrecacheOther( "npc_assassin_regenerator" );

	PrecacheScriptSound( "NPC_Assassin.ThrowKnife" );
	PrecacheScriptSound( "NPC_Assassin.ThrowHopWire" );
	PrecacheScriptSound( "NPC_Assassin.Stab" );
	PrecacheScriptSound( "NPC_Assassin.BallShoot" );
	PrecacheScriptSound( "NPC_Assassin.StabMiss" );
	PrecacheScriptSound( "NPC_Assassin.Death" );
	PrecacheScriptSound( "NPC_Assassin.Alert" );
	PrecacheScriptSound( "NPC_Assassin.FootStep.Left" );
	PrecacheScriptSound( "NPC_Assassin.FootStep.Right" );
}
void CAssassin::Spawn( void )
{
	Precache();

	if( HasSpawnFlags( SF_ASSASSIN_FERAL ))
	{
		SetModel("models/alien_assassin_feral.mdl");
	}
	else
		SetModel("models/alien_assassin.mdl");

	SetHullType( HULL_HUMAN );
	SetHullSizeNormal();
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	
	m_bloodColor		= BLOOD_COLOR_YELLOW;
	
	if( HasSpawnFlags( SF_ASSASSIN_FERAL ))
	{
		m_iHealth		= sk_assassin_feral_health.GetFloat();
	}
	else
		m_iHealth		= sk_assassin_health.GetFloat();
	m_flFieldOfView		= VIEW_FIELD_WIDE;
	m_NPCState			= NPC_STATE_NONE;
	m_iHopsAmount = 5;
	m_bUsedRegen = 0;
	m_bStartedRegen = 0;
	m_flRegenImmunityTime = gpGlobals->curtime;
	m_nBody = 0; // bodygroup 1, Assassin_reference_notessel.smd. The normal model.
	
	m_flNextInspectTime	= gpGlobals->curtime + random->RandomFloat( 20.0, 60.0 );
		
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_TURN_HEAD );
	CapabilitiesAdd( bits_CAP_MOVE_JUMP );
	CapabilitiesAdd( bits_CAP_INNATE_MELEE_ATTACK1 );
//	CapabilitiesAdd( bits_CAP_INNATE_RANGE_ATTACK1 );

	NPCInit();
	
	BaseClass::Spawn();
}
Class_T CAssassin::Classify( void )
{
	return CLASS_COMBINE;
}
void CAssassin::DeathSound(const CTakeDamageInfo &info)
{
}
void CAssassin::AlertSound( void )
{
}
void CAssassin::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_ASSASSIN_IDLE:
	{
		SetActivity( ACT_IDLE );
		TaskComplete();
	}
	break;

	case TASK_MELEE_ATTACK1:
	{
		SetActivity( ACT_MELEE_ATTACK1 );
	}
	case TASK_RANGE_ATTACK1:
	{
		SetActivity( ACT_RANGE_ATTACK1 );
	}
	break;

	case TASK_ASSASSIN_SMOKE_ATTACK1:
	{
		SetActivity( ACT_ASSASSIN_SMOKE );
	}
	break;

	case TASK_ASSASSIN_GET_COVER_PATH: // TODO: Improve this part! Right now the assassin will just run away for a short distance.
	{
		 Vector vGoalPos, vForward;
		 AngleVectors( GetLocalAngles(), &vForward );
		 
		 vGoalPos = GetAbsOrigin() + ( vForward * -128 );
		 
		 if ( GetNavigator()->SetGoal( vGoalPos ) == true )
		 {
			TaskComplete();
		 }
		 else
		 {
			TaskFail( FAIL_NO_GOAL );
		 }		  
	}
	break;

	case TASK_ASSASSIN_START_REGEN:
	{
		CAssassinRegenerator* pRegenerator = (CAssassinRegenerator *)CBaseEntity::Create( "npc_assassin_regenerator", 
			GetAbsOrigin(), 
			GetAbsAngles(), this );

		pRegenerator->StartRegen( this, sk_assassin_regen_period.GetFloat(), GetHealth(), 0 );
			
		m_bUsedRegen = 1;
		m_bStartedRegen = 1;
		m_flRegenImmunityTime = gpGlobals->curtime + sk_assassin_regen_period.GetFloat(); //the assassin will be 50% immune to damage for the next N seconds

		m_pMyRegenerator = dynamic_cast<CBaseEntity*>(pRegenerator);

		TaskComplete();
	}
	break;

		default:
		BaseClass::StartTask( pTask );
		break;
	}
}
void CAssassin::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		case TASK_MELEE_ATTACK1:
		{
			if( IsActivityFinished() )
			{
				DevMsg( "TMA1 FINISHED\n" );
				TaskComplete();
			}
		}
		break;
		case TASK_RANGE_ATTACK1:
		{
			if( IsActivityFinished() )
			{
				DevMsg( "TRA1 FINISHED\n" );
				TaskComplete();
			}
		}
		break;
		case TASK_ASSASSIN_SMOKE_ATTACK1:
		{
			if ( IsActivityFinished() )
			{		
				DevMsg( "TASA1 FINISHED\n" );
				TaskComplete();
			}
		}
		break;
	default:
	BaseClass::RunTask( pTask );
	break;
	}
}
int	 CAssassin::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	CTakeDamageInfo Info = info;
	if( m_bStartedRegen && gpGlobals->curtime < m_flRegenImmunityTime )
	{
		Info.ScaleDamage(0.5); //give assassin a 50% immunity to damage for given amount of time, in seconds
		Msg( "Assassin is partially immune to damage\n" );
	}	
	if ( m_iHealth < ASSASSIN_START_REGEN_THRESHOLD )
	{
		if( !HasCondition( COND_ASSASSIN_NEED_REGEN ) && !HasSpawnFlags( SF_ASSASSIN_FERAL ))
		{
			SetCondition( COND_ASSASSIN_NEED_REGEN );
		}
	}

	return BaseClass::OnTakeDamage_Alive( Info );
}
void CAssassin::Event_Killed(const CTakeDamageInfo &info)
{
	if( m_pMyRegenerator )
	{
		m_pMyRegenerator->SetThink( NULL );
		m_pMyRegenerator->SetNextThink( 0.025f );
	}
//	RemoveEFlags( EF_NOSHADOW );

	BaseClass::Event_Killed( info );
}
void CAssassin::HandleAnimEvent(animevent_t *pEvent)
{
	switch( pEvent->event )
	{
	case ASSASSIN_AE_STAB:
		{
			MeleeAttack();
		}
		break;
	case ASSASSIN_AE_FSLEFT:
		{
			EmitSound( "NPC_Assassin.FootStep.Left", pEvent->eventtime );
		}
		break;
	case ASSASSIN_AE_FSRIGHT:
		{
			EmitSound( "NPC_Assassin.FootStep.Right", pEvent->eventtime );
		}
		break;
	case ASSASSIN_AE_SMOKE:
		{
			ThrowSmoke();
		}
		break;
	case ASSASSIN_AE_THROWKNIFE:
		{
			ThrowKnife();
		}
		break;
	case ASSASSIN_AE_LAND:
		{
			LandEffect( GetAbsOrigin() );
		}
		break;
	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}
void CAssassin::MeleeAttack( void )
{
	CBaseEntity *pHurt;
	trace_t		tr;

	pHurt = CheckTraceHullAttack( 32, Vector(-16,-16,-16), Vector(16,16,16), sk_assassin_stab.GetFloat(), DMG_SLASH );

	if ( pHurt )
	{
		if (!pHurt->IsPlayer())
		{
			ClearMultiDamage();
			CTakeDamageInfo npcdamage( this, this, vec3_origin, GetAbsOrigin(), sk_assassin_stab_npc.GetFloat(), DMG_SLASH ); 
			pHurt->DispatchTraceAttack( npcdamage, vec3_origin, &tr );
		}

		if ( pHurt->GetFlags() & (FL_NPC|FL_CLIENT) )
		{
			pHurt->ViewPunch( QAngle( 5, 0, random->RandomInt(-10,10)) );
		}
				
		// Spawn some extra blood if we hit a BCC
		CBaseCombatCharacter* pBCC = ToBaseCombatCharacter( pHurt );
		if (pBCC)
		{
			SpawnBlood(pBCC->EyePosition(), g_vecAttackDir, pBCC->BloodColor(), sk_assassin_stab.GetFloat());
		}		
		EmitSound( "NPC_Assassin.Stab" );
	}
	else
	{		
		EmitSound( "NPC_Assassin.StabMiss" );
	}
}
void CAssassin::ThrowKnife( void )
{
}
void CAssassin::ThrowHopwire( float timer )
{
	if( m_iHopsAmount >= 1 )
	{
		Vector	vecSrc;
		Vector	vForward, vRight;

		Vector vecSpin;
		vecSpin.x = random->RandomFloat( -1000.0, 1000.0 );
		vecSpin.y = random->RandomFloat( -1000.0, 1000.0 );
		vecSpin.z = random->RandomFloat( -1000.0, 1000.0 );

		vecSrc = Weapon_ShootPosition();
		
		Vector forward, up, vecThrow;

		GetVectors( &forward, NULL, &up );
		vecThrow = forward * 350 + up * 50;

//		HopWire_Create( vecSrc, vec3_angle, vecThrow, vecSpin, this, timer );

		m_iHopsAmount = m_iHopsAmount - 1;

		RegenerateHopwire();
	}
}
void CAssassin::RegenerateHopwire( void )
{
	if( m_iHopsAmount < 1 )
	{
		if( m_flNextHopwireTime < gpGlobals->curtime )
		{
			m_iHopsAmount += 1;
			DevMsg( "Hopwire regenerated!\n" );
		}
	}
	SetNextThink( gpGlobals->curtime + 0.1f );	
}
void CAssassin::ThrowSmoke( void )
{
/*	Vector vecStart;
	vecStart = Weapon_ShootPosition();

	Vector forward, up, right, vecThrow;

	GetVectors( &forward, &right, &up );
	vecThrow = forward * 450 + up * 175 + right * random->RandomFloat(-15, 5);
	
	CGrenadeSmoke *pSmokeGrenade = (CGrenadeSmoke*)Create( "grenade_smoke", vecStart, vec3_angle, this );

	pSmokeGrenade->SetAbsVelocity( vecThrow );
	pSmokeGrenade->SetGravity( 0.5f );
	pSmokeGrenade->SetLocalAngularVelocity( RandomAngle( -400, 400 ) );
	*/

	m_nBody = 1;
	RemoveAllDecals();
	AddEFlags( EF_NOSHADOW );
	DevMsg( "PUFF\n" );
}
void CAssassin::LandEffect( const Vector &origin )
{
	trace_t tr;
	AI_TraceLine( origin, origin - Vector(0,0,0), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
	float yaw = random->RandomInt(0,0);
	for ( int i = 0; i < 2; i++ )
	{
		if ( UTIL_PointContents( tr.endpos + Vector( 0, 0, 1 ) ) & MASK_WATER )
		{
			float flWaterZ = UTIL_FindWaterSurface( tr.endpos, tr.endpos.z, tr.endpos.z + 100.0f );

			CEffectData	data;
			data.m_fFlags = 0;
			data.m_vOrigin = tr.endpos;
			data.m_vOrigin.z = flWaterZ;
			data.m_vNormal = Vector( 0, 0, 1 );
			data.m_flScale = random->RandomFloat( 10.0, 14.0 );

			DispatchEffect( "watersplash", data );
		}		
		else
		{
			Vector dir = UTIL_YawToVector( yaw + i*180 ) * 10;
			VectorNormalize( dir );
			dir.z = 0.25;
			VectorNormalize( dir );
			g_pEffects->Dust( tr.endpos, dir, 12, 50 );
		}
	}
}
NPC_STATE CAssassin::SelectIdealState( void )
{
	switch( m_NPCState )
	{
	case NPC_STATE_COMBAT:
		{
			if ( GetEnemy() == NULL )
			{
				if ( !HasCondition( COND_ENEMY_DEAD ) )
				{
					SetCondition( COND_ENEMY_DEAD ); // TODO: patrolling

				}
				return NPC_STATE_ALERT;
			}
			else if ( HasCondition( COND_ENEMY_DEAD ) )
			{
				//AnnounceEnemyKill(GetEnemy());
			}
		}
	default:
		{
			return BaseClass::SelectIdealState();
		}
	}
	return GetIdealState();
}
int CAssassin::MeleeAttack1Conditions ( float flDot, float flDist )
{
	if (flDist > ASSASSIN_STAB_DIST)
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.7)
	{
		return COND_NOT_FACING_ATTACK;
	}
	else if( HasStartedRegen() && !HasFinishedRegen() )
	{
		return COND_ASSASSIN_BUSY_REGEN;
	}
	return COND_CAN_MELEE_ATTACK1;
}
int CAssassin::RangeAttack1Conditions ( float flDot, float flDist )
{
	if (flDist > ASSASSIN_RANGE1_DIST_MAX )
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDist < ASSASSIN_RANGE1_DIST_MIN )
	{
		return COND_TOO_CLOSE_TO_ATTACK;
	}
	else if (flDot < 0.7)
	{
		return COND_NOT_FACING_ATTACK;
	}
	else if (gpGlobals->curtime < m_flNextHopwireTime )
	{
		return COND_NONE;
	}
	return COND_CAN_RANGE_ATTACK1;
}
int CAssassin::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_MELEE_ATTACK1:
		return SCHED_ASSASSIN_MELEE_ATTACK1;
		break;
	}
	return BaseClass::TranslateSchedule( scheduleType );
}
int CAssassin::SelectSchedule ( void )
{
	switch	( m_NPCState )
	{
	case NPC_STATE_IDLE:
	case NPC_STATE_ALERT:
		{
			if ( HasCondition ( COND_HEAR_DANGER ) )
				 return SCHED_TAKE_COVER_FROM_BEST_SOUND;
				
			if ( HasCondition ( COND_HEAR_COMBAT ) )
				return SCHED_INVESTIGATE_SOUND;
		}
		break;
	case NPC_STATE_COMBAT:
		{
			if( HasCondition( COND_ENEMY_DEAD ) )
			{
				return BaseClass::SelectSchedule();
			}
			// Punches, slashes
			if( HasCondition( COND_CAN_MELEE_ATTACK1 ) 
				&& !HasCondition( COND_CAN_RANGE_ATTACK1 ))
			{
				return SCHED_MELEE_ATTACK1; // translates into SCHED_ASSASSIN_MELEE_ATTACK1
			}
			// Throw knives
			if( HasCondition( COND_CAN_RANGE_ATTACK1 ) 
				&& !HasCondition( COND_CAN_MELEE_ATTACK1 ) 
				&& m_iHopsAmount >= 1)
			{
				return SCHED_ASSASSIN_RANGE_ATTACK1;
			}
			if( HasCondition( COND_ASSASSIN_BUSY_REGEN )
				&& HasCondition( COND_CAN_RANGE_ATTACK1 ))
			{
				return SCHED_ASSASSIN_RANGE_ATTACK1;
			}
			// Initiate Regen
			if( m_iHealth < ASSASSIN_START_REGEN_THRESHOLD 
				&& !HasUsedRegen() 
				&& !HasSpawnFlags( SF_ASSASSIN_FERAL ) )
			{
				return SCHED_ASSASSIN_SMOKE;
			}
			// No 'nades or too close to throw them - go into melee
			if( HasCondition( COND_TOO_CLOSE_TO_ATTACK) || m_iHopsAmount < 1 )
			{
				return SCHED_ASSASSIN_CHASE;
			}
			// 'nades are used up and still regenerating - go into melee
			while( m_flNextHopwireTime > gpGlobals->curtime )
			{
				return SCHED_ASSASSIN_CHASE;
			}
		}
		break;
	}
	return BaseClass::SelectSchedule();
}
//==================================================
//
//==================================================
AI_BEGIN_CUSTOM_NPC( npc_alien_assassin, CAssassin )

	DECLARE_TASK(TASK_ASSASSIN_IDLE)
	DECLARE_TASK(TASK_ASSASSIN_SMOKE_ATTACK1)
	DECLARE_TASK(TASK_ASSASSIN_GET_COVER_PATH)
	DECLARE_TASK(TASK_ASSASSIN_START_REGEN)
	DECLARE_ACTIVITY( ACT_ASSASSIN_SMOKE )

	DEFINE_SCHEDULE
	(
		SCHED_ASSASSIN_MELEE_ATTACK1,

		"	Tasks"
		"	TASK_STOP_MOVING 0"
		"	TASK_FACE_ENEMY 0"
		"	TASK_MELEE_ATTACK1 0"
//		"	TASK_PLAY_SEQUENCE	ACTIVITY:ACT_MELEE_ATTACK1"
		"	"
		"	Interrupts"
		"	COND_ENEMY_DEAD"
		"	COND_HEAVY_DAMAGE"
	)
	DEFINE_SCHEDULE
	(
		SCHED_RANGE_ATTACK1,

		"	Tasks"
		"	TASK_STOP_MOVING 0"
		"	TASK_FACE_ENEMY 0"
		"	TASK_RANGE_ATTACK1 0"
//		"	TASK_PLAY_SEQUENCE	ACTIVITY:ACT_MELEE_ATTACK1"
		"	"
		"	Interrupts"
		"	COND_ENEMY_DEAD"
		"	COND_CAN_MELEE_ATTACK1"	
		"	COND_HEAVY_DAMAGE"
	)
	DEFINE_SCHEDULE
	(
		SCHED_ASSASSIN_CHASE,

		"	Tasks"
		"	TASK_SET_TOLERANCE_DISTANCE			32"
		"	TASK_GET_CHASE_PATH_TO_ENEMY		400"
		"	TASK_RUN_PATH						0"
		"	TASK_WAIT_FOR_MOVEMENT				0"
		"	TASK_FACE_ENEMY						0"
		""
		"	Interrupts"
		"	COND_ENEMY_DEAD"
		"	COND_NEW_ENEMY"
		"	COND_CAN_MELEE_ATTACK1"	
		"	COND_CAN_RANGE_ATTACK1"
		"	COND_HEAVY_DAMAGE"
	)
	DEFINE_SCHEDULE
	(
		SCHED_ASSASSIN_IDLE,

		"	Tasks"
		"	TASK_PLAY_SEQUENCE	ACTIVITY:ACT_IDLE"
		""
		"	Interrupts"
		"	COND_NEW_ENEMY"
	)	
	DEFINE_SCHEDULE
	(
		SCHED_ASSASSIN_SMOKE,
		
		"	Tasks"
		"	TASK_ASSASSIN_SMOKE_ATTACK1			0"
	//	"	TASK_ASSASSIN_GET_COVER_PATH		0"
	//	"	TASK_RUN_PATH						0"
	//	"	TASK_WAIT_FOR_MOVEMENT				0"
		"	TASK_ASSASSIN_START_REGEN			0"
		""
		"	Interrupts"
		"	COND_ENEMY_DEAD"
	)
AI_END_CUSTOM_NPC()