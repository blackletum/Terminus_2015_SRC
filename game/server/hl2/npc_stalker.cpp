//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
#include "cbase.h"
#include "npc_stalker.h"

LINK_ENTITY_TO_CLASS( npc_stalker, CNPC_Stalker );
BEGIN_DATADESC( CNPC_Stalker )

	DEFINE_KEYFIELD( m_eBeamPower,			FIELD_INTEGER,	"BeamPower" ),
	DEFINE_KEYFIELD( m_bArmBeamEnable,		FIELD_BOOLEAN, "ArmBeamEnable" ),
	DEFINE_FIELD( m_vLaserDir,				FIELD_VECTOR),
	DEFINE_FIELD( m_vLaserTargetPos,		FIELD_POSITION_VECTOR),
	DEFINE_FIELD( m_fBeamEndTime,			FIELD_FLOAT),
	DEFINE_FIELD( m_fBeamRechargeTime,		FIELD_FLOAT),
	DEFINE_FIELD( m_fNextDamageTime,		FIELD_FLOAT),
	DEFINE_FIELD( m_bPlayingHitWall,		FIELD_FLOAT),
	DEFINE_FIELD( m_bPlayingHitFlesh,		FIELD_FLOAT),
	DEFINE_FIELD( m_pBeam,					FIELD_CLASSPTR),
	DEFINE_FIELD( m_pLightGlow,				FIELD_CLASSPTR),
	DEFINE_FIELD( m_flNextNPCThink,			FIELD_FLOAT),
	DEFINE_FIELD( m_pScriptedTarget,		FIELD_CLASSPTR),
	DEFINE_FIELD( m_vLaserCurPos,			FIELD_POSITION_VECTOR),
	DEFINE_FIELD( m_flNextAttackSoundTime,	FIELD_TIME ),
	DEFINE_FIELD( m_flNextBreatheSoundTime, FIELD_TIME ),
	DEFINE_FIELD( m_flNextScrambleSoundTime, FIELD_TIME ),
	DEFINE_FIELD( m_nextSmokeTime,			FIELD_TIME ),
	DEFINE_FIELD( m_iPlayerAggression,		FIELD_INTEGER ),
	DEFINE_FIELD( m_flNextScreamTime,		FIELD_TIME ),
	
	DEFINE_INPUTFUNC(FIELD_VOID, "ToggleArmBeam", InputToggleArmBeam),
	// Function Pointers
	DEFINE_THINKFUNC( StalkerThink ),

END_DATADESC()

float g_StalkerBeamThinkTime = 0.025; //0.025;

//------------------------------------------------------------------------------
CNPC_Stalker::CNPC_Stalker(void)
{
#ifdef _DEBUG
	m_vLaserDir.Init();
	m_vLaserTargetPos.Init();
	m_vLaserCurPos.Init();
#endif
	
	m_bArmBeamEnable = true;
}
//-----------------------------------------------------------------------------
float CNPC_Stalker::MaxYawSpeed( void )
{
//#ifdef HL2_EPISODIC
	//return 10.0f;
//#else
	switch( GetActivity() )
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		return 160;
		break;
	case ACT_WALK:
	case ACT_RUN:
	case ACT_RUN_HURT:
		return 280;
		break;
	default:
		return 280;
		break;
	}
//#endif
}
//-----------------------------------------------------------------------------
Class_T CNPC_Stalker::Classify( void )
{
	return CLASS_COMBINE;
}
//-----------------------------------------------------------------------------
void CNPC_Stalker::Precache( void )
{
	//PrecacheModel("models/stalker_test7.mdl");
	PrecacheModel("models/stalker_normal.mdl");
	PrecacheModel("models/stalker_tessel.mdl");
	PrecacheModel("sprites/laser.vmt");	

	PrecacheModel("sprites/redglow1.vmt");
	PrecacheModel("sprites/orangeglow1.vmt");
	PrecacheModel("sprites/glow02.vmt");
	enginesound->PrecacheSound("npc/stalker/laser_start.wav");			// Beam start

	PrecacheScriptSound( "NPC_Stalker.BurnFlesh" );
	PrecacheScriptSound( "NPC_Stalker.BurnWall" );
	PrecacheScriptSound( "NPC_Stalker.FootstepLeft" );
	PrecacheScriptSound( "NPC_Stalker.FootstepRight" );
	PrecacheScriptSound( "NPC_Stalker.Hit" );
	PrecacheScriptSound( "NPC_Stalker.Ambient01" );
	PrecacheScriptSound( "NPC_Stalker.Scream" );
	//PrecacheScriptSound( "NPC_Stalker.Breathing" );
	//PrecacheScriptSound( "NPC_Stalker.AlertSound" );
	PrecacheScriptSound( "NPC_Stalker.Pain" );
	PrecacheScriptSound( "NPC_Stalker.Die" );

	BaseClass::Precache();
}
//-----------------------------------------------------------------------------
void CNPC_Stalker::Spawn( void )
{
	Precache( );

	//SetModel( "models/stalker_test7.mdl" );
	SetModel( "models/stalker_tessel.mdl" );
	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	m_bloodColor		= BLOOD_COLOR_MECH;
	m_iHealth			= sk_stalker_health.GetFloat();
	m_flFieldOfView		= 0.5;// indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState			= NPC_STATE_NONE;
	//m_IdealNPCState		= NPC_STATE_NONE;

	SetIdealState( NPC_STATE_NONE );
	CapabilitiesAdd( bits_CAP_SQUAD | bits_CAP_MOVE_GROUND | bits_CAP_OPEN_DOORS );
	CapabilitiesAdd( bits_CAP_INNATE_MELEE_ATTACK1);
	CapabilitiesAdd( bits_CAP_INNATE_RANGE_ATTACK1);

	m_flNextAttackSoundTime		= 0;
	m_flNextBreatheSoundTime	= 0;
	m_flNextScrambleSoundTime	= 0;
	m_nextSmokeTime = 0;
	m_bPlayingHitWall			= false;
	m_bPlayingHitFlesh			= false;

	m_fBeamEndTime				= 0;
	m_fBeamRechargeTime			= 0;
	m_fNextDamageTime			= 0;
	m_pScriptedTarget			= NULL;

	NPCInit();

	char szMapName[256];
	Q_strncpy(szMapName, STRING(gpGlobals->mapname), sizeof(szMapName) );
	Q_strlower(szMapName);

	// exchange_02b tower stalkers LOS hack

	if( !Q_strnicmp( szMapName, "exchange_02b", 12 ) )
	{
		m_flDistTooFar = 3000;
		GetSenses()->SetDistLook(3000 - 1);
	}

	else
	{
		m_flDistTooFar	= MAX_STALKER_FIRE_RANGE;
		GetSenses()->SetDistLook(MAX_STALKER_FIRE_RANGE - 1);
	}

	m_iPlayerAggression = 10;

	InitArmBeam();

	switch( sk_stalker_beam_power.GetInt() )
	{
	case 1:
		{
			m_eBeamPower = STALKER_BEAM_LOW;
		}
		break;
	case 2:
		{
			m_eBeamPower = STALKER_BEAM_MED;
		}
		break;
	case 3:
		{
			m_eBeamPower = STALKER_BEAM_HIGH;
		}
		break;
	default:
		{
			m_eBeamPower = STALKER_BEAM_LOW;
		}
	}
}
//-----------------------------------------------------------------------------
void CNPC_Stalker::IdleSound ( void )
{
	EmitSound( "NPC_Stalker.Ambient01" );
}
//-----------------------------------------------------------------------------
void CNPC_Stalker::DeathSound( const CTakeDamageInfo &info )
{ 
	EmitSound( "NPC_Stalker.Die" );
};
//-----------------------------------------------------------------------------
void CNPC_Stalker::PainSound( const CTakeDamageInfo &info )
{ 
	EmitSound( "NPC_Stalker.Pain" );
	m_flNextScrambleSoundTime	= gpGlobals->curtime + 1.5;
	m_flNextBreatheSoundTime	= gpGlobals->curtime + 1.5;
	m_flNextAttackSoundTime		= gpGlobals->curtime + 1.5;
};
//-----------------------------------------------------------------------------
void CNPC_Stalker::InitArmBeam( void )
{
	m_pArmBeam = CBeam::BeamCreate( "sprites/laser.vmt", 2.0 );
	int	startAttachment = 2;
	int endAttachment	= 3;
		
	m_pArmBeam->AddSpawnFlags( SF_BEAM_TEMPORARY );
	m_pArmBeam->SetColor( 255, 40, 0 ); 
	m_pArmBeam->SetBrightness( 200 );
	m_pArmBeam->SetNoise( 3 );
	m_pArmBeam->EntsInit( this, this );
	m_pArmBeam->SetStartAttachment( startAttachment );
	m_pArmBeam->SetEndAttachment( endAttachment );
}
void CNPC_Stalker::KillArmBeam( void )
{
	if( m_pArmBeam )
	{
		if ( !m_bArmBeamEnable )
		{
			UTIL_Remove(m_pArmBeam);
			m_pArmBeam = NULL;
		}
	}
}
void CNPC_Stalker::InputToggleArmBeam( inputdata_t &inputdata )
{
	m_bArmBeamEnable = !m_bArmBeamEnable;
	SetNextThink( gpGlobals->curtime );
}
//-----------------------------------------------------------------------------
void CNPC_Stalker::SetScriptedTarget( CScriptedTarget *pScriptedTarget )
{
	if (pScriptedTarget)
	{
		// ---------------------------------------
		//	If I don't already have a burn target
		// ---------------------------------------
		if (m_pScriptedTarget == NULL)
		{
			// Wake the guy up
			SetCondition(COND_PROVOKED);

			// If first burn target make it my current position
			m_pScriptedTarget	= pScriptedTarget;
			m_vLaserCurPos		= m_pScriptedTarget->m_vLastPosition;
		}
		else
		{
			m_pScriptedTarget	= pScriptedTarget;
		}
		m_vLaserTargetPos	= m_pScriptedTarget->GetAbsOrigin();
	}
	else
	{
		// Break him out of burn schedule
		SetCondition(COND_ENEMY_DEAD);

		m_pScriptedTarget	= NULL;
	}
}
int	CNPC_Stalker::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	CTakeDamageInfo info = inputInfo;
	// --------------------------------------------
	//	Don't take a lot of damage from Vortigaunt
	// --------------------------------------------
	if (info.GetAttacker()->Classify() == CLASS_VORTIGAUNT)
	{
		info.ScaleDamage( 0.25 );
	}
	int ret = BaseClass::OnTakeDamage_Alive( info );

	// If player shot me make sure I'm mad at him even if I wasn't earlier
	if ( (info.GetAttacker()->GetFlags() & FL_CLIENT) )
	{
		AddClassRelationship( CLASS_PLAYER, D_HT, 10 );
	}
	return ret;
}
//-----------------------------------------------------------------------------
bool CNPC_Stalker::IsValidEnemy( CBaseEntity *pEnemy )
{
	Class_T enemyClass = pEnemy->Classify();

	if( enemyClass == CLASS_PLAYER || enemyClass == CLASS_PLAYER_ALLY || enemyClass == CLASS_PLAYER_ALLY_VITAL )
	{
		// Don't get angry at these folks unless provoked.
		if( m_iPlayerAggression < STALKER_PLAYER_AGGRESSION )
		{
			return false;
		}
	}
	if( enemyClass == CLASS_BULLSEYE && pEnemy->GetParent() )
	{
		// This bullseye is in heirarchy with something. If that
		// something is held by the physcannon, this bullseye is 
		// NOT a valid enemy.
		IPhysicsObject *pPhys = pEnemy->GetParent()->VPhysicsGetObject();
		if( pPhys && (pPhys->GetGameFlags() & FVPHYSICS_PLAYER_HELD) )
		{
			return false;
		}
	}
	if( GetEnemy() && HasCondition(COND_SEE_ENEMY) )
	{
		// Short attention span. If I have an enemy, stick with it.
		if( GetEnemy() != pEnemy )
		{
			return false;
		}
	}
	if( IsStrategySlotRangeOccupied( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) && !HasStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
	{
		return false;
	}
	if( !FVisible(pEnemy) )
	{
		// Don't take an enemy you can't see. Since stalkers move way too slowly to
		// establish line of fire, usually an enemy acquired by means other than
		// the Stalker's own eyesight will always get away while the stalker plods
		// slowly to their last known position. So don't take enemies you can't see.
		return false;
	}
	return BaseClass::IsValidEnemy(pEnemy);
}
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pInflictor - 
//			pAttacker - 
//			flDamage - 
//			bitsDamageType - 
//-----------------------------------------------------------------------------
void CNPC_Stalker::Event_Killed( const CTakeDamageInfo &info )
{
	if( IsInSquad() && info.GetAttacker()->IsPlayer() )
	{
		AISquadIter_t iter;
		for ( CAI_BaseNPC *pSquadMember = GetSquad()->GetFirstMember( &iter ); pSquadMember; pSquadMember = GetSquad()->GetNextMember( &iter ) )
		{
			if ( pSquadMember->IsAlive() && pSquadMember != this )
			{
				CNPC_Stalker *pStalker = dynamic_cast <CNPC_Stalker*>(pSquadMember);

				if( pStalker && pStalker->FVisible(info.GetAttacker()) )
				{
					pStalker->m_iPlayerAggression++;
				}
			}
		}
	}
	KillAttackBeam();
	KillArmBeam();
	BaseClass::Event_Killed( info );
}

//-----------------------------------------------------------------------------
// Purpose: Translates squad slot positions into schedules
// @TODO (toml 07-18-03): this function is never called. Presumably what it is trying to do still needs to be done...
#if 0
int CNPC_Stalker::GetSlotSchedule(int slotID)
{
	switch (slotID)
	{

		case SQUAD_SLOT_CHASE_ENEMY_1:
		case SQUAD_SLOT_CHASE_ENEMY_2:
			return SCHED_STALKER_CHASE_ENEMY;
			break;
	}
	return SCHED_NONE;
}
#endif

void CNPC_Stalker::UpdateAttackBeam( void )
{
	CBaseEntity *pEnemy = GetEnemy();
	// If not burning at a target 
	if (pEnemy)
	{
		/*if (gpGlobals->curtime > m_fBeamEndTime)
		{
			TaskComplete();
		}
		else 
		{*/
			Vector enemyLKP = GetEnemyLKP();
			m_vLaserTargetPos = enemyLKP + pEnemy->GetViewOffset();

			// Face my enemy
			GetMotor()->SetIdealYawToTargetAndUpdate( enemyLKP );

			// ---------------------------------------------
			//	Get beam end point
			// ---------------------------------------------
			Vector vecSrc = LaserStartPosition(GetAbsOrigin());
			Vector targetDir = m_vLaserTargetPos - vecSrc;
			VectorNormalize(targetDir);
			// --------------------------------------------------------
			//	If beam position and laser dir are way off, end attack
			// --------------------------------------------------------
			/*if ( DotProduct(targetDir,m_vLaserDir) < 0.5 )
			{
				TaskComplete();
				return;
			}*/
			trace_t tr;
			AI_TraceLine( vecSrc, vecSrc + m_vLaserDir * MAX_STALKER_FIRE_RANGE, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);
			// ---------------------------------------------
			//  If beam not long enough, stop attacking
			// ---------------------------------------------
			if (tr.fraction == 1.0)
			{
				TaskComplete();
				return;
			}
			CSoundEnt::InsertSound(SOUND_DANGER, tr.endpos, 60, 0.025, this);
		//}
	}
	// Face my burn target
	else if (m_pScriptedTarget)
	{
		GetMotor()->SetIdealYawToTargetAndUpdate( m_pScriptedTarget->GetAbsOrigin() );
		// ---------------------------------------------
		//  If can't see burn target, stop attacking
		// ---------------------------------------------
		trace_t tr;
		AI_TraceLine( LaserStartPosition(GetAbsOrigin()), m_vLaserCurPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
		CBaseEntity *pEntity = tr.m_pEnt;
		if (tr.fraction != 1.0 && pEntity!=m_pScriptedTarget)		
		{	
			SetDefaultFailSchedule( SCHED_ESTABLISH_LINE_OF_FIRE ); 
			TaskFail("No LOS");
		}
		else
		{
			CSoundEnt::InsertSound(SOUND_DANGER, tr.endpos, 60, 0.025, this);
		}
	}
	else
	{
		TaskFail(FAIL_NO_ENEMY);
	}
}
Vector CNPC_Stalker::LaserStartPosition(Vector vStalkerPos)
{
	// Get attachment position
	Vector vAttachPos;
	GetAttachment(STALKER_LASER_ATTACHMENT,vAttachPos);
	// Now convert to vStalkerPos
	vAttachPos = vAttachPos - GetAbsOrigin() + vStalkerPos;
	return vAttachPos;
}
//------------------------------------------------------------------------------
void CNPC_Stalker::CalcBeamPosition(void)
{
	Vector targetDir = m_vLaserTargetPos - LaserStartPosition(GetAbsOrigin());
	VectorNormalize(targetDir);
	// -----------------------------------------------
	//  If I'm burning towards a burn target
	// -----------------------------------------------
	if (GetEnemy() == NULL && m_pScriptedTarget != NULL)
	{
		//  Move towards burn target at linear rate
		m_vLaserDir		  = ScriptedBurnPosition() - LaserStartPosition(GetAbsOrigin());
		VectorNormalize(m_vLaserDir);

		// If I've reached by burn target I'm done
		float fDist = (m_vLaserDir - targetDir).Length();
		if ( fDist < 0.01)
		{
			// Update scripted target
			SetScriptedTarget( m_pScriptedTarget->NextScriptedTarget());
		}
	}
	// ---------------------------------------
	//  Otherwise if burning towards an enemy
	// ---------------------------------------
	else
	{
		// ---------------------------------------
		//  Integrate towards target position
		// ---------------------------------------
		float	iRate = 0.25;
		m_vLaserDir.x = (iRate * m_vLaserDir.x + (1-iRate) * targetDir.x);
		m_vLaserDir.y = (iRate * m_vLaserDir.y + (1-iRate) * targetDir.y);
		m_vLaserDir.z = (iRate * m_vLaserDir.z + (1-iRate) * targetDir.z);
		VectorNormalize( m_vLaserDir );
		// -----------------------------------------
		// Add time-coherent noise to the position
		// Must be scaled with distance 
		// -----------------------------------------
		float fTargetDist = (GetAbsOrigin() - m_vLaserTargetPos).Length();
		float noiseScale		= atan(0.2/fTargetDist);
		float m_fNoiseModX		= 5;
		float m_fNoiseModY		= 5;
		float m_fNoiseModZ		= 5;

		m_vLaserDir.x += 5*noiseScale*sin(m_fNoiseModX * gpGlobals->curtime + m_fNoiseModX);
		m_vLaserDir.y += 5*noiseScale*sin(m_fNoiseModY * gpGlobals->curtime + m_fNoiseModY);
		m_vLaserDir.z += 5*noiseScale*sin(m_fNoiseModZ * gpGlobals->curtime + m_fNoiseModZ);
	}
}
void CNPC_Stalker::StartAttackBeam( void )
{
	if ( m_fBeamEndTime > gpGlobals->curtime || m_fBeamRechargeTime > gpGlobals->curtime )
	{
		// UNDONE: Debug this and fix!?!?!
		m_fBeamRechargeTime = gpGlobals->curtime;
	}
	// ---------------------------------------------
	//  If I don't have a beam yet, create one
	// ---------------------------------------------
	if (!m_pBeam)
	{
		Vector vecSrc = LaserStartPosition(GetAbsOrigin());
		trace_t tr;
		AI_TraceLine ( vecSrc, vecSrc + m_vLaserDir * MAX_STALKER_FIRE_RANGE, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);
		if ( tr.fraction >= 1.0 )
		{
			// too far
			TaskComplete();
			return;
		}
		m_pBeam = CBeam::BeamCreate( "sprites/laser.vmt", 2.0 );
		m_pBeam->PointEntInit( tr.endpos, this );
		m_pBeam->SetEndAttachment( STALKER_LASER_ATTACHMENT );  
		m_pBeam->SetBrightness( 255 );
		m_pBeam->SetNoise( 0 );

		switch (m_eBeamPower)
		{
			case STALKER_BEAM_LOW:
				m_pBeam->SetColor( 255, 40, 0 );
				m_pLightGlow = CSprite::SpriteCreate( "sprites/redglow1.vmt", GetAbsOrigin(), FALSE );
				break;
			case STALKER_BEAM_MED:
				m_pBeam->SetColor( 255, 50, 0 );
				m_pLightGlow = CSprite::SpriteCreate( "sprites/orangeglow1.vmt", GetAbsOrigin(), FALSE );
				break;
			case STALKER_BEAM_HIGH:
				m_pBeam->SetColor( 60, 220, 255 );
				m_pLightGlow = CSprite::SpriteCreate( "sprites/stalker_blueglow1.vmt", GetAbsOrigin(), FALSE );
				m_pLightGlow->SetRenderColor( 60, 220, 255 );
				m_pLightGlow->SetScale( 0.25 );
				break;
		}
		// ----------------------------
		// Light myself in a red glow
		// ----------------------------
		m_pLightGlow->SetTransparency( kRenderGlow, 255, 200, 200, 0, kRenderFxNoDissipation );
		m_pLightGlow->SetAttachment( this, 1 );
		m_pLightGlow->SetBrightness( 255 );
		m_pLightGlow->SetScale( 0.65 );

		CBaseEntity *pEnemy = GetEnemy();
		// --------------------------------------------------------
		// Play start up sound - client should always hear this!
		// --------------------------------------------------------
		if (pEnemy != NULL && (pEnemy->IsPlayer()) ) 
		{
			EmitAmbientSound( 0, pEnemy->GetAbsOrigin(), "NPC_Stalker.AmbientLaserStart" );
		}
		else
		{
			EmitAmbientSound( 0, GetAbsOrigin(), "NPC_Stalker.AmbientLaserStart" );
		}
	}

	SetThink( &CNPC_Stalker::StalkerThink );
	m_flNextNPCThink = GetNextThink();
	SetNextThink( gpGlobals->curtime + g_StalkerBeamThinkTime );
	m_fBeamEndTime = gpGlobals->curtime + STALKER_LASER_DURATION;
}
//------------------------------------------------------------------------------
void CNPC_Stalker::DrawAttackBeam(void)
{
	if (!m_pBeam)
		return;
	// ---------------------------------------------
	//	Get beam end point
	// ---------------------------------------------
	Vector vecSrc = LaserStartPosition(GetAbsOrigin());
	trace_t tr;
	AI_TraceLine( vecSrc, vecSrc + m_vLaserDir * MAX_STALKER_FIRE_RANGE, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);
	// If I have a BurnTarget
	if (GetEnemy() == NULL && m_pScriptedTarget != NULL)
	{	
		// ------------------------------------------
		//  Update scripts last position
		// ------------------------------------------
		m_vLaserCurPos = ScriptedBurnPosition();
		m_pScriptedTarget->m_vLastPosition = m_vLaserCurPos;
	}
	CalcBeamPosition();

	bool bInWater = (UTIL_PointContents ( tr.endpos ) & MASK_WATER)?true:false;

	// ---------------------------------------------
	//	Update the beam position
	// ---------------------------------------------
	m_pBeam->SetStartPos( tr.endpos );
	m_pBeam->RelinkBeam();

	Vector vAttachPos;
	GetAttachment(STALKER_LASER_ATTACHMENT,vAttachPos);
	Vector vecAimDir = tr.endpos - vAttachPos;
	VectorNormalize( vecAimDir );
	SetAim( vecAimDir );

	// --------------------------------------------
	//  Play burn sounds
	// --------------------------------------------
	CBaseCombatCharacter *pBCC = ToBaseCombatCharacter( tr.m_pEnt );
	if (pBCC)
	{
		if (gpGlobals->curtime > m_fNextDamageTime)
		{
			ClearMultiDamage();

			float damage = 0.0;
			switch (m_eBeamPower)
			{
				case STALKER_BEAM_LOW:
					damage = 1;
					break;
				case STALKER_BEAM_MED:
					damage = 3;
					break;
				case STALKER_BEAM_HIGH:
					damage = 10;
					break;
			}
			CTakeDamageInfo info( this, this, damage, DMG_SHOCK );
			CalculateMeleeDamageForce( &info, m_vLaserDir, tr.endpos );
			pBCC->DispatchTraceAttack( info, m_vLaserDir, &tr );
			ApplyMultiDamage();
			m_fNextDamageTime = gpGlobals->curtime + 0.1;
		}
		if (pBCC->Classify()!=CLASS_BULLSEYE)
		{
			if (!m_bPlayingHitFlesh)
			{
				CPASAttenuationFilter filter( m_pBeam,"NPC_Stalker.BurnFlesh" );
				filter.MakeReliable();

				EmitSound( filter, m_pBeam->entindex(),"NPC_Stalker.BurnFlesh" );
				m_bPlayingHitFlesh = true;
			}
			if (m_bPlayingHitWall)
			{
				StopSound( m_pBeam->entindex(), "NPC_Stalker.BurnWall" );
				m_bPlayingHitWall = false;
			}
			tr.endpos.z -= 24.0f;
			if (!bInWater)
			{
				DoSmokeEffect(tr.endpos + tr.plane.normal * 8);
			}
		}
	}	
	if (!pBCC || pBCC->Classify()==CLASS_BULLSEYE)
	{
		if (!m_bPlayingHitWall)
		{
			CPASAttenuationFilter filter( m_pBeam, "NPC_Stalker.BurnWall" );
			filter.MakeReliable();

			EmitSound( filter, m_pBeam->entindex(), "NPC_Stalker.BurnWall" );
			m_bPlayingHitWall = true;
		}
		if (m_bPlayingHitFlesh)
		{
			StopSound(m_pBeam->entindex(), "NPC_Stalker.BurnFlesh" );
			m_bPlayingHitFlesh = false;
		}
		UTIL_DecalTrace( &tr, "RedGlowFade");
		UTIL_DecalTrace( &tr, "FadingScorch" );
		
		tr.endpos.z -= 24.0f;
		if (!bInWater)
		{
			DoSmokeEffect(tr.endpos + tr.plane.normal * 8);
		}
	}
	if (bInWater)
	{
		UTIL_Bubbles(tr.endpos-Vector(3,3,3),tr.endpos+Vector(3,3,3),10);
	}
	/*
	CBroadcastRecipientFilter filter;
	TE_DynamicLight( filter, 0.0, EyePosition(), 255, 0, 0, 5, 0.2, 0 );
	*/
}
//------------------------------------------------------------------------------
void CNPC_Stalker::DoSmokeEffect( const Vector &position )
{
	if ( gpGlobals->curtime > m_nextSmokeTime )
	{
		m_nextSmokeTime = gpGlobals->curtime + 0.5;
		UTIL_Smoke(position, random->RandomInt(5, 10), 10);
	}
}
//------------------------------------------------------------------------------
void CNPC_Stalker::KillAttackBeam(void)
{
	if ( !m_pBeam )
		return;
	// Kill sound
	StopSound(m_pBeam->entindex(), "NPC_Stalker.BurnWall" );
	StopSound(m_pBeam->entindex(), "NPC_Stalker.BurnFlesh" );

	UTIL_Remove( m_pLightGlow );
	UTIL_Remove( m_pBeam);
	m_pBeam = NULL;
	m_bPlayingHitWall = false;
	m_bPlayingHitFlesh = false;

	SetThink(&CNPC_Stalker::CallNPCThink);
	if ( m_flNextNPCThink > gpGlobals->curtime )
	{
		SetNextThink( m_flNextNPCThink );
	}
	// Beam has to recharge
	m_fBeamRechargeTime = gpGlobals->curtime + STALKER_LASER_RECHARGE;

	ClearCondition( COND_CAN_RANGE_ATTACK1 );

	RelaxAim();
}
//------------------------------------------------------------------------------
// Purpose : Update beam more often then regular NPC think so it doesn't
//			 move so jumpily over the ground
void CNPC_Stalker::StalkerThink(void)
{
	DrawAttackBeam();
	if (gpGlobals->curtime >= m_flNextNPCThink)
	{
		NPCThink();
		m_flNextNPCThink = GetNextThink();
	}

	if ( m_pBeam )
	{
		SetNextThink( gpGlobals->curtime + g_StalkerBeamThinkTime );
		
		// sanity check?!
		const Task_t *pTask = GetTask();
		if ( !pTask || pTask->iTask != TASK_RANGE_ATTACK1 || !TaskIsRunning() )
		{
			KillAttackBeam();
		}
	}
	else
	{
		DevMsg( 2, "In StalkerThink() but no stalker beam found?\n" );
		SetNextThink( m_flNextNPCThink );
	}
}
//=========================================================
void CNPC_Stalker::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_STALKER_SCREAM:
	{
		if( gpGlobals->curtime > m_flNextScreamTime )
		{
			EmitSound( "NPC_Stalker.Scream" );
			m_flNextScreamTime = gpGlobals->curtime + random->RandomFloat( 10.0, 15.0 );
		}
		TaskComplete();
	}
	case TASK_ANNOUNCE_ATTACK:
	{
		// If enemy isn't facing me and I haven't attacked in a while
		// annouce my attack before I start wailing away
		CBaseCombatCharacter *pBCC = GetEnemyCombatCharacterPointer();

		if	(pBCC && (!pBCC->FInViewCone ( this )) &&
			 (gpGlobals->curtime - m_flLastAttackTime > 1.0) )
		{
				m_flLastAttackTime = gpGlobals->curtime;

				// Always play this sound
				EmitSound( "NPC_Stalker.Scream" );
				m_flNextScrambleSoundTime = gpGlobals->curtime + 2;
				m_flNextBreatheSoundTime = gpGlobals->curtime + 2;

				// Wait two seconds
				SetWait( 0.5 );
				SetActivity(ACT_IDLE);
		}
		break;
	}
	case TASK_STALKER_ZIGZAG:
			break;
	case TASK_RANGE_ATTACK1:
		{
			CBaseEntity *pEnemy = GetEnemy();
			if (pEnemy)
			{
				m_vLaserTargetPos = GetEnemyLKP() + pEnemy->GetViewOffset();
				// Never hit target on first try
				Vector missPos = m_vLaserTargetPos;
				
				if( pEnemy->Classify() == CLASS_BULLSEYE && hl2_episodic.GetBool() )
				{
					missPos.x += 60 + 120*random->RandomInt(-1,1);
					missPos.y += 60 + 120*random->RandomInt(-1,1);
				}
				else
				{
					missPos.x += 80*random->RandomInt(-1,1);
					missPos.y += 80*random->RandomInt(-1,1);
				}
				// ----------------------------------------------------------------------
				// If target is facing me and not running towards me shoot below his feet
				// so he can see the laser coming
				// ----------------------------------------------------------------------
				CBaseCombatCharacter *pBCC = ToBaseCombatCharacter(pEnemy);
				if (pBCC)
				{
					Vector targetToMe = (pBCC->GetAbsOrigin() - GetAbsOrigin());
					Vector vBCCFacing = pBCC->BodyDirection2D( );
					if ((DotProduct(vBCCFacing,targetToMe) < 0) &&
						(pBCC->GetSmoothedVelocity().Length() < 50))
					{
						missPos.z -= 150;
					}
					// --------------------------------------------------------
					// If facing away or running towards laser,
					// shoot above target's head 
					// --------------------------------------------------------
					else
					{
						missPos.z += 60;
					}
				}
				m_vLaserDir = LaserStartPosition(GetAbsOrigin());
				VectorNormalize(m_vLaserDir);	
			}
			// --------------------------------------
			//  Do I have a target position to burn
			// --------------------------------------
			else if (m_pScriptedTarget)
			{
				trace_t tr;
				AI_TraceLine(LaserStartPosition(GetAbsOrigin()), m_vLaserCurPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);	
				CBaseEntity *pEntity = tr.m_pEnt;
				if (tr.fraction != 1.0 && pEntity != m_pScriptedTarget)
				{	
					SetDefaultFailSchedule( SCHED_ESTABLISH_LINE_OF_FIRE ); 
					TaskFail("No LOS");
					return;
				}
				m_vLaserDir	= m_vLaserCurPos - LaserStartPosition(GetAbsOrigin());
				VectorNormalize(m_vLaserDir);	
			}
			else
			{
				TaskFail(FAIL_NO_ENEMY);
				return;
			}

			StartAttackBeam();
			SetActivity(ACT_RANGE_ATTACK1);
			break;
		}
	case TASK_GET_PATH_TO_ENEMY_LOS:
		{
			if ( GetEnemy() != NULL )
			{
				BaseClass:: StartTask( pTask );
				return;
			}
			Vector posLos;

			if (GetTacticalServices()->FindLos(m_vLaserCurPos, m_vLaserCurPos, MIN_STALKER_FIRE_RANGE, MAX_STALKER_FIRE_RANGE, 1.0, &posLos))
			{
				AI_NavGoal_t goal( posLos, ACT_RUN, AIN_HULL_TOLERANCE );
				GetNavigator()->SetGoal( goal );
			}
			else
			{
				TaskFail(FAIL_NO_SHOOT);
			}
			break;
		}
	case TASK_STALKER_MELEE_ATTACK1:
		{
			Vector flEnemyLKP = GetEnemyLKP();
			GetMotor()->SetIdealYawToTarget( flEnemyLKP );
		}
		Msg("STALKER_MELEE_ATTACK1\n" );
		return;
		break;
	case TASK_FACE_ENEMY:
		{
			if ( GetEnemy() != NULL )
			{
				BaseClass:: StartTask( pTask );
				return;
			}
			GetMotor()->SetIdealYawToTarget( m_vLaserCurPos );
			break;
		}
	default: 
		BaseClass:: StartTask( pTask );
		break;
	}
}
//=========================================================
void CNPC_Stalker::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_ANNOUNCE_ATTACK:
	{
		// Stop waiting if enemy facing me or lost enemy
		CBaseCombatCharacter* pBCC = GetEnemyCombatCharacterPointer();
		if	(!pBCC || pBCC->FInViewCone( this ))
		{
			TaskComplete();
		}

		if ( IsWaitFinished() )
		{
			TaskComplete();
		}
		break;
	}
	case TASK_STALKER_ZIGZAG :
		{

			if (GetNavigator()->GetGoalType() == GOALTYPE_NONE)
			{
				TaskComplete();
				GetNavigator()->StopMoving();		// Stop moving
			}
			else if (!GetNavigator()->IsGoalActive())
			{
				SetIdealActivity( GetStoppedActivity() );
			}
			else if (ValidateNavGoal())
			{
				SetIdealActivity( GetNavigator()->GetMovementActivity() );
				AddZigZagToPath();
			}
			break;
		}
	case TASK_RANGE_ATTACK1:
		UpdateAttackBeam();
		if ( !TaskIsRunning() || HasCondition( COND_TASK_FAILED ))
		{
			KillAttackBeam();
		}
		break;

	case TASK_STALKER_MELEE_ATTACK1:
		{
			Vector flEnemyLKP = GetEnemyLKP();
			GetMotor()->SetIdealYawToTargetAndUpdate( flEnemyLKP );
			if ( IsActivityFinished() )
			{
				TaskComplete();
				return;
			}
		}
		break;
	case TASK_FACE_ENEMY:
		{
			if ( GetEnemy() != NULL )
			{
				BaseClass:: RunTask( pTask );
				return;
			}
			GetMotor()->SetIdealYawToTargetAndUpdate( m_vLaserCurPos );

			if ( FacingIdeal() )
			{
				TaskComplete();
			}
			break;
		}
	default:
		{
			BaseClass::RunTask( pTask );
			break;
		}
	}
}
//-----------------------------------------------------------------------------
void CNPC_Stalker::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
		case NPC_EVENT_LEFTFOOT:
			{
				EmitSound( "NPC_Stalker.FootstepLeft", pEvent->eventtime );
			}
			break;
		case NPC_EVENT_RIGHTFOOT:
			{
				EmitSound( "NPC_Stalker.FootstepRight", pEvent->eventtime );
			}
			break;

		case STALKER_AE_MELEE_HIT:
		{
			CBaseEntity *pHurt;

			pHurt = CheckTraceHullAttack( 32, Vector(-16,-16,-16), Vector(16,16,16), sk_stalker_melee_dmg.GetFloat(), DMG_SLASH );

			if ( pHurt )
			{
				if ( pHurt->GetFlags() & (FL_NPC|FL_CLIENT) )
				{
					pHurt->ViewPunch( QAngle( 5, 0, random->RandomInt(-10,10)) );
				}				
				// Spawn some extra blood if we hit a BCC
				CBaseCombatCharacter* pBCC = ToBaseCombatCharacter( pHurt );
				if (pBCC)
				{
					SpawnBlood(pBCC->EyePosition(), g_vecAttackDir, pBCC->BloodColor(), sk_stalker_melee_dmg.GetFloat());
				}
				// Play a attack hit sound
				EmitSound( "NPC_Stalker.Hit" );
			}
			break;	
		}
		default:
			BaseClass::HandleAnimEvent( pEvent );
			break;
	}
}
Activity CNPC_Stalker::NPC_TranslateActivity( Activity baseAct ) // always run
{
	if( baseAct == ACT_WALK )
	{
		return (Activity)ACT_RUN;
	}
	return BaseClass::NPC_TranslateActivity( baseAct );
}
//-----------------------------------------------------------------------------
int CNPC_Stalker::MeleeAttack1Conditions ( float flDot, float flDist )
{
	if (flDist > 36)
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.7)
	{
		return COND_NOT_FACING_ATTACK;
	}
	return COND_CAN_MELEE_ATTACK1;
}
//-----------------------------------------------------------------------------
int CNPC_Stalker::RangeAttack1Conditions( float flDot, float flDist )
{
	if (gpGlobals->curtime < m_fBeamRechargeTime )
	{
		return COND_NONE;
	}
	if( IsStrategySlotRangeOccupied( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
	{
		// Couldn't attack if I wanted to.
		return COND_NONE;
	}
	if (flDist <= MIN_STALKER_FIRE_RANGE)
	{
		return COND_TOO_CLOSE_TO_ATTACK;
	}
	else if (flDist > (MAX_STALKER_FIRE_RANGE * 0.66f) )
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.7)
	{
		return COND_NOT_FACING_ATTACK;
	}
	return COND_CAN_RANGE_ATTACK1;
}
//-----------------------------------------------------------------------------
bool CNPC_Stalker::InnateWeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions )
{
	// --------------------
	// Check for occlusion
	// --------------------
	// Base class version assumes innate weapon position is at eye level
	Vector barrelPos = LaserStartPosition(ownerPos);
	trace_t tr;
	AI_TraceLine( barrelPos, targetPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

	if ( tr.fraction == 1.0 )
	{
		return true;
	}
	CBaseEntity *pBE = tr.m_pEnt;
	CBaseCombatCharacter *pBCC = ToBaseCombatCharacter( pBE );
	if ( pBE == GetEnemy() )
	{
		return true;
	}
	else if (pBCC) 
	{
		if (IRelationType( pBCC ) == D_HT)
		{
			return true;
		}
		else if (bSetConditions)
		{
			SetCondition(COND_WEAPON_BLOCKED_BY_FRIEND);
		}
	}
	else if (bSetConditions)
	{
		SetCondition(COND_WEAPON_SIGHT_OCCLUDED);
		SetEnemyOccluder(pBE);
	}
	return false;
}
//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Stalker::CreateBehaviors()
{
	AddBehavior( &m_ActBusyBehavior );

	return BaseClass::CreateBehaviors();
}
//-----------------------------------------------------------------------------
void CNPC_Stalker::PrescheduleThink()
{
	if (gpGlobals->curtime > m_flNextBreatheSoundTime)
	{
		IdleSound();
		m_flNextBreatheSoundTime = gpGlobals->curtime + 6.0 + random->RandomFloat( 0.0, 5.0 );
	}

	if( m_pArmBeam == NULL && m_bArmBeamEnable )
	{
		InitArmBeam();
	}
	else if( m_pArmBeam != NULL && !m_bArmBeamEnable )
	{
		KillArmBeam();
	}
}
//-----------------------------------------------------------------------------
int CNPC_Stalker::SelectSchedule( void )
{
	if ( BehaviorSelectSchedule() )
	{
		return BaseClass::SelectSchedule();
	}
	switch	( m_NPCState )
	{
		case NPC_STATE_IDLE:
		{
			if ( HasCondition ( COND_HEAR_DANGER ) ||
				 HasCondition ( COND_HEAR_COMBAT ) ||
				 HasCondition ( COND_HEAR_WORLD  ) ||
				 HasCondition ( COND_HEAR_PLAYER ) )
			{
				return SCHED_ALERT_FACE;
			}
			else if (m_pScriptedTarget!=NULL)
			{
				// Check if I have a line of sight 
				trace_t tr;
				AI_TraceLine(LaserStartPosition(GetAbsOrigin()), m_vLaserCurPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);	
				CBaseEntity *pEntity = tr.m_pEnt;
				if (tr.fraction != 1.0 && pEntity != m_pScriptedTarget)
				{	
					return SCHED_ESTABLISH_LINE_OF_FIRE;
				}
				else
				{
					return SCHED_RANGE_ATTACK1;
				}
			}
			break;
		}
		case NPC_STATE_ALERT:
		{
			if( HasCondition( COND_TOO_FAR_TO_ATTACK ) )
			{
				return SCHED_STALKER_CHASE_ENEMY;
			}
			if ( HasCondition( COND_ENEMY_DEAD ) )
			{
				// Scan around for new enemies
				return SCHED_ALERT_SCAN;
			}
			if ( HasCondition(COND_LIGHT_DAMAGE) ||
				 HasCondition(COND_HEAVY_DAMAGE) )
			{
				return SCHED_TAKE_COVER_FROM_ORIGIN;
			}
			else if ( HasCondition ( COND_HEAR_DANGER ) ||
					  HasCondition ( COND_HEAR_PLAYER ) ||
					  HasCondition ( COND_HEAR_WORLD  ) ||
					  HasCondition ( COND_HEAR_COMBAT ) )
			{
				return SCHED_ALERT_FACE;
			}			
			else if (m_pScriptedTarget!=NULL)
			{
				// Check if I have a line of sight
				trace_t tr;
				AI_TraceLine(LaserStartPosition(GetAbsOrigin()), m_vLaserCurPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);	
				CBaseEntity *pEntity = tr.m_pEnt;
				if (tr.fraction != 1.0 && pEntity != m_pScriptedTarget)
				{	
					return SCHED_ESTABLISH_LINE_OF_FIRE;
				}
				else
				{
					return SCHED_RANGE_ATTACK1;
				}
			}
			else
			{
				return SCHED_RANGE_ATTACK1;
			}
			break;
		}
		case NPC_STATE_COMBAT:
		{
			
			if( HasCondition( COND_TOO_FAR_TO_ATTACK ) )
			{
				return SCHED_STALKER_CHASE_ENEMY;
			}
			// -----------
			// dead enemy
			// -----------
			if ( HasCondition( COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return BaseClass::SelectSchedule();
			}
			// -------------------------------------------
			// If I can't range attack and not ready to beam
			// -------------------------------------------
			if ( !HasCondition ( COND_CAN_RANGE_ATTACK1 ) && gpGlobals->curtime < m_fBeamRechargeTime)
			{
				return SCHED_MELEE_ATTACK1;
			}
			// -------------------------------------------
			// If I can't range attack and not ready to beam
			// -------------------------------------------
			if ( HasCondition( COND_ENEMY_TOO_FAR )				||
				 HasCondition( COND_TOO_FAR_TO_ATTACK )			||	 
				 HasCondition( COND_TOO_CLOSE_TO_ATTACK )		||
				 HasCondition( COND_ENEMY_OCCLUDED )			|| 
				 HasCondition( COND_WEAPON_SIGHT_OCCLUDED )		|| 
				 HasCondition( COND_WEAPON_BLOCKED_BY_FRIEND )	)
			{
				return SCHED_ESTABLISH_LINE_OF_FIRE;
			}
			// --------------------------------------------------------------
			//  If I can't see my enemy, I'm not facing him as I have a
			//  line of sight
			// --------------------------------------------------------------
			if ( !HasCondition( COND_SEE_ENEMY ))
			{
				return SCHED_COMBAT_FACE;
			}
			// --------------------------------------------------------------
			// If there aren't any attack slots, go to work go dormant unless
			// my enemy just hurt
			// --------------------------------------------------------------
			if (!HasCondition(COND_LOST_ENEMY) &&
				 (CBaseEntity*)GetEnemy()		  &&
				 (OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) ||
				 HasCondition(COND_LIGHT_DAMAGE)	|| 
				 HasCondition(COND_HEAVY_DAMAGE) ))
			{
				// --------------------------------------------------------------
				// If I can't attack go for it!
				// --------------------------------------------------------------
				if (HasCondition(COND_CAN_RANGE_ATTACK1))
				{
					/*if (gpGlobals->curtime > m_flNextAttackSoundTime)
					{
//						SENTENCEG_PlayRndSz( edict(), "STALKER_ATTACK", STALKER_SENTENCE_VOLUME, ATTN_NORM, 0, 100);
						m_flNextScrambleSoundTime	= gpGlobals->curtime + 0.5;
						m_flNextBreatheSoundTime	= gpGlobals->curtime + 0.5;
						m_flNextAttackSoundTime		= gpGlobals->curtime + 0.5;	
					}*/
					return SCHED_RANGE_ATTACK1;
				}
				else if (HasCondition(COND_CAN_MELEE_ATTACK1))
				{
					if (gpGlobals->curtime > m_flNextAttackSoundTime)
					{
//						SENTENCEG_PlayRndSz( edict(), "STALKER_ATTACK", STALKER_SENTENCE_VOLUME, ATTN_NORM, 0, 100);
						m_flNextScrambleSoundTime	= gpGlobals->curtime + 0.5;
						m_flNextBreatheSoundTime	= gpGlobals->curtime + 0.5;
						m_flNextAttackSoundTime		= gpGlobals->curtime + 0.5;	
					}
					return SCHED_MELEE_ATTACK1;
				}
			}
			// --------------------------------------
			// Otherwise go for cover
			// --------------------------------------
			return SCHED_TAKE_COVER_FROM_ENEMY;
			break;
		}
	}
	// no special cases here, call the base class
	return BaseClass::SelectSchedule();
}
//-----------------------------------------------------------------------------
int CNPC_Stalker::TranslateSchedule( int scheduleType ) 
{
	switch( scheduleType )
	{
	case SCHED_RANGE_ATTACK1:
		{
			return SCHED_STALKER_RANGE_ATTACK;
		}
	case SCHED_MELEE_ATTACK1:
		{
			return SCHED_STALKER_MELEE_ATTACK;
		}
	case SCHED_FAIL_ESTABLISH_LINE_OF_FIRE:
		{	
			if (GetEnemy() != NULL)
			{
				return SCHED_TAKE_COVER_FROM_ENEMY;
			}
			else
			{
				return SCHED_IDLE_STAND;
			}
			break;
		}
	case SCHED_FAIL_TAKE_COVER:
		{
			return SCHED_RUN_RANDOM;
			break;
		}
	}

	return BaseClass::TranslateSchedule( scheduleType );
}
void CNPC_Stalker::OnScheduleChange()
{
	KillAttackBeam();

	BaseClass::OnScheduleChange();
}
Vector CNPC_Stalker::ScriptedBurnPosition(void)
{
	// Make sure I don't overshoot
	Vector	vTargetDir  = m_vLaserTargetPos - m_vLaserCurPos;
	float	fTargetDist	= VectorNormalize(vTargetDir);
	float	fBurnSpeed	= m_pScriptedTarget->MoveSpeed();

	if (fTargetDist < fBurnSpeed)
	{
		return m_vLaserTargetPos;
	}
	else
	{
		return m_vLaserCurPos + fBurnSpeed*vTargetDir;
	}
}
//------------------------------------------------------------------------------
// Purpose : Returns position of laser for any given position of the staler
//------------------------------------------------------------------------------
void CNPC_Stalker::NotifyDeadFriend( CBaseEntity *pFriend )
{
	BaseClass::NotifyDeadFriend(pFriend);
}


//-----------------------------------------------------------------------------
// Purpose: Tells use whether or not the NPC cares about a given type of hint node.
// Input  : sHint - 
// Output : TRUE if the NPC is interested in this hint type, FALSE if not.
//-----------------------------------------------------------------------------
bool CNPC_Stalker::FValidateHintType(CAI_Hint *pHint)
{
	return(pHint->HintType() == HINT_WORLD_WORK_POSITION);
}
//-----------------------------------------------------------------------------
// Purpose: Override in subclasses to associate specific hint types
//			with activities
Activity CNPC_Stalker::GetHintActivity( short sHintType, Activity HintsActivity )
{
	if (sHintType == HINT_WORLD_WORK_POSITION)
	{
		return ( Activity )ACT_STALKER_WORK;
	}

	return BaseClass::GetHintActivity( sHintType, HintsActivity );
}
//-----------------------------------------------------------------------------
// Purpose: Override in subclasses to give specific hint types delays
//			before they can be used again
float	CNPC_Stalker::GetHintDelay( short sHintType )
{
	if (sHintType == HINT_WORLD_WORK_POSITION)
	{
		return 2.0;
	}
	return 0;
}
//-----------------------------------------------------------------------------
#define ZIG_ZAG_SIZE 3600
void CNPC_Stalker::AddZigZagToPath(void) 
{
	// If already on a detour don't add a zigzag
	if (GetNavigator()->GetCurWaypointFlags() & bits_WP_TO_DETOUR)
	{
		return;
	}

	// If enemy isn't facing me or occluded, don't add a zigzag
	if (HasCondition(COND_ENEMY_OCCLUDED) || !HasCondition ( COND_ENEMY_FACING_ME ))
	{
		return;
	}

	Vector waypointPos = GetNavigator()->GetCurWaypointPos();
	Vector waypointDir = (waypointPos - GetAbsOrigin());

	// If the distance to the next node is greater than ZIG_ZAG_SIZE
	// then add a random zig/zag to the path
	if (waypointDir.LengthSqr() > ZIG_ZAG_SIZE)
	{
		// Pick a random distance for the zigzag (less that sqrt(ZIG_ZAG_SIZE)
		float distance = random->RandomFloat( 30, 60 );

		// Get me a vector orthogonal to the direction of motion
		VectorNormalize( waypointDir );
		Vector vDirUp(0,0,1);
		Vector vDir;
		CrossProduct( waypointDir, vDirUp, vDir);

		// Pick a random direction (left/right) for the zigzag
		if (random->RandomInt(0,1))
		{
			vDir = -1 * vDir;
		}

		// Get zigzag position in direction of target waypoint
		Vector zigZagPos = GetAbsOrigin() + waypointDir * 60;

		// Now offset 
		zigZagPos = zigZagPos + (vDir * distance);

		// Now make sure that we can still get to the zigzag position and the waypoint
		AIMoveTrace_t moveTrace1, moveTrace2;
		GetMoveProbe()->MoveLimit( NAV_GROUND, GetAbsOrigin(), zigZagPos, MASK_NPCSOLID, NULL, &moveTrace1);
		GetMoveProbe()->MoveLimit( NAV_GROUND, zigZagPos, waypointPos, MASK_NPCSOLID, NULL, &moveTrace2);
		if ( !IsMoveBlocked( moveTrace1 ) && !IsMoveBlocked( moveTrace2 ) )
		{
			GetNavigator()->PrependWaypoint( zigZagPos, NAV_GROUND, bits_WP_TO_DETOUR );
		}
	}
}
//------------------------------------------------------------------------------
AI_BEGIN_CUSTOM_NPC( npc_stalker, CNPC_Stalker )

	DECLARE_TASK(TASK_STALKER_ZIGZAG)
	DECLARE_TASK(TASK_STALKER_SCREAM)
	DECLARE_TASK(TASK_STALKER_MELEE_ATTACK1)
	DECLARE_ACTIVITY(ACT_STALKER_WORK)
	DECLARE_SQUADSLOT(SQUAD_SLOT_CHASE_ENEMY_1)
	DECLARE_SQUADSLOT(SQUAD_SLOT_CHASE_ENEMY_2)

	DEFINE_SCHEDULE
	(
		SCHED_STALKER_RANGE_ATTACK,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_FACE_ENEMY					0"
		"		TASK_RANGE_ATTACK1				0"
		""
		"	Interrupts"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_HEAVY_DAMAGE"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		//"		COND_ENEMY_OCCLUDED"	// Don't break on this.  Keep shooting at last location
	)
	DEFINE_SCHEDULE
	(
		SCHED_STALKER_MELEE_ATTACK,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_FACE_ENEMY				0"
		//"		TASK_ANNOUNCE_ATTACK		1"
		"		TASK_STALKER_MELEE_ATTACK1	0"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_MELEE_ATTACK1"
		""
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_HEAVY_DAMAGE"
	)
	DEFINE_SCHEDULE
	(
		SCHED_STALKER_CHASE_ENEMY,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY_FAILED"
		"		TASK_SET_TOLERANCE_DISTANCE		24"
		"		TASK_GET_PATH_TO_ENEMY			0"
		"		TASK_WAIT						0.5"
		"		TASK_RUN_PATH					0"
		"		TASK_STALKER_ZIGZAG				0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_TOO_CLOSE_TO_ATTACK"
		"		COND_TASK_FAILED"
		"		COND_HEAR_DANGER"
	)
/*
	DEFINE_SCHEDULE
	(
	SCHED_STALKER_ACQUIRE_PLAYER,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_FACE_ENEMY					0"
	"		TASK_WAIT_RANDOM				0.5"
	"		TASK_STALKER_SCREAM				0"
	"		TASK_WAIT						0.5"
	"		TASK_WAIT_RANDOM				0.5"
	""
	"	Interrupts"
	)

	DEFINE_SCHEDULE	
	(
	SCHED_STALKER_PATROL,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_WAIT						0.5"// This makes them look a bit more vigilant, instead of INSTANTLY patrolling after some other action.
	"		TASK_WAIT_RANDOM				0.5"
	"		TASK_WANDER						18000600" 
	"		TASK_FACE_PATH					0"
	"		TASK_WALK_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_STOP_MOVING				0"
	"		TASK_FACE_REASONABLE			0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_SEE_ENEMY"
	)
*/
AI_END_CUSTOM_NPC()
