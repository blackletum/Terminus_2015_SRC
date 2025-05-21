#include "cbase.h"
#include "npc_combine_synth_elite.h"

#include "ammodef.h"

LINK_ENTITY_TO_CLASS(npc_combine_synth_elite, CNPC_Combine_synth_elite);

void CNPC_Combine_synth_elite::Precache(void)
{
	engine->PrecacheModel(COMBINEGUARD_MODEL);

	engine->PrecacheModel("sprites/blueflare1.vmt");

	PrecacheScriptSound("NPC_CombineGuard.FootstepLeft");
	PrecacheScriptSound("NPC_CombineGuard.FootstepRight");
	PrecacheScriptSound("NPC_CombineGuard.Fire");
	PrecacheScriptSound("NPC_CombineGuard.Charging");
	enginesound->PrecacheSound("weapons/cguard/cguard_fire.wav");

	BaseClass::Precache();
}
bool CNPC_Combine_synth_elite::IsArmorPiece(int iArmorPiece)
{
	switch (iArmorPiece)
	{
	case CGUARD_BG_INVALID:
	case CGUARD_BG_MAINBODY:
	case CGUARD_BG_GUARDGUN:
	case CGUARD_BG_RBICEPS:
		return false;
		break;

	default:
		return true;
		break;
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Combine_synth_elite::InitArmorPieces(void)
{
	//Turn on all body groups
	for (int i = 1; i < NUM_CGUARD_BODYGROUPS; i++)
	{
		SetBodygroup(i, 0);
		SetBodygroup(CGUARD_BG_GUARDGUN, 1);
		SetBodygroup(CGUARD_BG_RBICEPS, 1);

		m_armorPieces[i].m_iHealth = CGUARD_ARMORPIECE_HEALTH;

		if (IsArmorPiece(i))
		{
			m_armorPieces[i].m_bDestroyed = false;
		}
		else
		{
			// Start all non-armor bodygroups as destroyed so
			// that the code to pick and destroy random armor pieces
			// doesn't destroy pieces that aren't represented visually
			// (such as the head, torso, etc).
			m_armorPieces[i].m_bDestroyed = true;
		}
	}
}
int CNPC_Combine_synth_elite::GetReferencePointForBodyGroup(int bodyGroup)
{
	switch (bodyGroup)
	{
	case CGUARD_BG_INVALID:
	case CGUARD_BG_MAINBODY:
	case CGUARD_BG_GUARDGUN:
	case CGUARD_BG_RBICEPS:
		return CGUARD_REF_INVALID;
		break;

	case CGUARD_BG_LSHOULDER:
		return CGUARD_REF_LSHOULDER;
		break;

	case CGUARD_BG_LELBOW:
		return CGUARD_REF_LELBOW;
		break;

	case CGUARD_BG_LFOREARM:
		return CGUARD_REF_LFOREARM;
		break;

	case CGUARD_BG_LSHIELD:
		return CGUARD_REF_LSHIELD;
		break;

	case CGUARD_BG_RSHOULDER:
		return CGUARD_REF_RSHOULDER;
		break;

		//	case CGUARD_BG_RBICEPS:
		//		return CGUARD_REF_RBICEPS;		
		//		break;

	case CGUARD_BG_RKNEE:
		return CGUARD_REF_RKNEE;
		break;

	case CGUARD_BG_LKNEE:
		return CGUARD_REF_LKNEE;
		break;
	}
	return CGUARD_REF_INVALID;
}
void CNPC_Combine_synth_elite::Spawn(void)
{
	Precache();
	SetModel(COMBINEGUARD_MODEL);

	//HACKHACK - e3_terminal Combine parade hack. When on e3_term[inal] map, cguards have smaller hulls, allowing them
	//to not being blocked by the strider marching above them. Otherwise, they have normal large hulls.
	char szMapName[256];
	Q_strncpy(szMapName, STRING(gpGlobals->mapname), sizeof(szMapName));
	Q_strlower(szMapName);

	if (!Q_strnicmp(szMapName, "e3_term", 7))
	{
		SetHullType(HULL_HUMAN);
	}
	else
	{
		SetHullType(HULL_LARGE);
	}

	SetNavType(NAV_GROUND);
	m_NPCState = NPC_STATE_NONE;
	SetBloodColor(DONT_BLEED);
	m_iHealth = sk_combine_synth_elite_health.GetFloat();
	m_iArmorHealth = sk_combine_synth_elite_armor_health.GetFloat();
	m_iMaxHealth = m_iHealth;
	m_flFieldOfView = -0.707;

	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_NOT_STANDABLE);
	SetMoveType(MOVETYPE_STEP);

	m_flGlowTime = gpGlobals->curtime;
	m_flLastRangeTime = gpGlobals->curtime;
	m_flRangeAnimTime = 0.5;
	m_aimYaw = 0;
	m_aimPitch = 0;
	m_flNextClobberTime = gpGlobals->curtime;

	CapabilitiesClear();
	CapabilitiesAdd(bits_CAP_TURN_HEAD | bits_CAP_MOVE_GROUND | bits_CAP_MOVE_SHOOT | bits_CAP_INNATE_MELEE_ATTACK1);
	CapabilitiesAdd(bits_CAP_USE_WEAPONS);

	NPCInit();

	//Turn the gun off
	InitArmorPieces();

	m_YawControl = LookupPoseParameter("aim_yaw");
	m_PitchControl = LookupPoseParameter("aim_pitch");
	m_MuzzleAttachment = LookupAttachment("muzzle");

	BaseClass::Spawn();
	m_fOffBalance = false;
	Msg("CGuard spawned\n");
}
void CNPC_Combine_synth_elite::HandleAnimEvent(animevent_t *pEvent)
{
	switch (pEvent->event)
	{
	case CGUARD_AE_SHAKEIMPACT:
		//UTIL_RotorWash( WorldSpaceCenter(), Vector( 0, 0, -1 ), 128 );
		UTIL_ScreenShake(GetAbsOrigin(), 25.0, 150.0, 1.0, 750, SHAKE_START);
		break;
	case CGUARD_AE_LEFTFOOT:
	{
							   EmitSound("NPC_CombineGuard.FootstepLeft");
	}
		break;
	case CGUARD_AE_RIGHTFOOT:
	{
								EmitSound("NPC_CombineGuard.FootstepRight");
	}
		break;
	case CGUARD_AE_SHOVE:
	{
							Shove();
	}
		break;
		/*	case EVENT_WEAPON_CGUARD_FIRE:
		{
		m_flLastRangeTime = gpGlobals->curtime + 6.0f;
		//	FireRangeWeapon();
		//	EmitSound( "weapons/cguard/cguard_fire.wav" );
		EntityMessageBegin( this, true );
		WRITE_BYTE( CGUARD_MSG_SHOT );
		MessageEnd();
		}
		break;
		case EVENT_WEAPON_CGUARD_STARTFIRE:
		{
		EmitSound( "NPC_CombineGuard.Charging" );
		EntityMessageBegin( this, true );
		WRITE_BYTE( CGUARD_MSG_SHOT_START );
		MessageEnd();
		}
		break; */
	case CGUARD_AE_GLOW:
		m_flGlowTime = gpGlobals->curtime + CGUARD_GLOW_TIME;
		break;
	default:
		BaseClass::HandleAnimEvent(pEvent);
		return;
	}
}
#if 0
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_Combine_synth_elite::FireRangeWeapon(void)
{
	Vector vecSrc, vecAiming;

	GetVectors(&vecAiming, NULL, NULL);
	vecSrc = WorldSpaceCenter() + vecAiming * 64;

	Vector	impactPoint = vecSrc + (vecAiming * MAX_TRACE_LENGTH);

	trace_t	tr;
	AI_TraceLine(vecSrc, impactPoint, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

	float flTracerDist;
	Vector vecDir;
	Vector vecEndPos;

	vecDir = tr.endpos - vecSrc;
	flTracerDist = VectorNormalize(vecDir);
	//UTIL_Tracer( vecSrc, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, 8000, true, "GunshipTracer" );
	UTIL_Tracer(tr.startpos, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, 6000 + random->RandomFloat(0, 2000), true, "GunshipTracer");
	UTIL_Tracer(tr.startpos, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, 6000 + random->RandomFloat(0, 3000), true, "GunshipTracer");
	UTIL_Tracer(tr.startpos, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, 6000 + random->RandomFloat(0, 4000), true, "GunshipTracer");

	CreateConcussiveBlast(tr.endpos, tr.plane.normal, this, 1.0);
}
void CNPC_Combine_synth_elite::WarmupRangeWeapon(void)
{
	Msg("WARMUP\n");
}
#endif
void CNPC_Combine_synth_elite::Shove(void)
{
	if (GetEnemy() == NULL)
		return;
	CBaseEntity *pHurt = NULL;
	Vector forward;
	AngleVectors(GetLocalAngles(), &forward);
	float		flDist = (GetEnemy()->GetAbsOrigin() - GetAbsOrigin()).Length();
	Vector2D	v2LOS = (GetEnemy()->GetAbsOrigin() - GetAbsOrigin()).AsVector2D();
	Vector2DNormalize(v2LOS);
	float		flDot = DotProduct2D(v2LOS, forward.AsVector2D());
	flDist -= GetEnemy()->WorldAlignSize().x * 0.5f;
	if (flDist < COMBINEGUARD_MELEE1_RANGE && flDot >= COMBINEGUARD_MELEE1_CONE)
	{
		Vector vStart = GetAbsOrigin();
		vStart.z += WorldAlignSize().z * 0.5;
		Vector vEnd = GetEnemy()->GetAbsOrigin();
		vEnd.z += GetEnemy()->WorldAlignSize().z * 0.5;
		pHurt = CheckTraceHullAttack(vStart, vEnd, Vector(-16, -16, 0), Vector(16, 16, 24), 15, DMG_CLUB);
		//	SetActivity( ACT_MELEE_ATTACK1 );
	}
	if (pHurt)
	{
		pHurt->ViewPunch(QAngle(-20, 0, 20));
		UTIL_ScreenShake(pHurt->GetAbsOrigin(), 100.0, 1.5, 1.0, 2, SHAKE_START);
		color32 red = { 255, 0, 0, 16 };
		UTIL_ScreenFade(pHurt, red, 0.5f, 0.1f, FFADE_IN);
		if (pHurt->IsPlayer())
		{
			Vector forward, up;
			AngleVectors(GetLocalAngles(), &forward, NULL, &up);
			pHurt->ApplyAbsVelocityImpulse(forward * 500 + up * 250);
		}
	}
	//	SetActivity( ACT_MELEE_ATTACK1 );
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_Combine_synth_elite::StartTask(const Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_COMBINEGUARD_SET_BALANCE:
		if (pTask->flTaskData == 0.0)
		{
			m_fOffBalance = false;
		}
		else
		{
			m_fOffBalance = true;
		}
		TaskComplete();
		break;

	case TASK_RANGE_ATTACK1:
	{
							   m_flShotDelay = GetActiveWeapon()->GetFireRate();

							   m_flNextAttack = gpGlobals->curtime + m_flShotDelay - 0.1;
							   ResetIdealActivity(ACT_RANGE_ATTACK1);
							   m_flLastAttackTime = gpGlobals->curtime;
	}
		break;

		/*	case TASK_CGUARD_WARMUP_ATTACK1:
		{
		Vector flEnemyLKP = GetEnemyLKP();
		GetMotor()->SetIdealYawToTarget( flEnemyLKP );
		}
		WarmupRangeWeapon();
		return;
		break;*/

	case TASK_CGUARD_MELEE_ATTACK1:
	{
									  DevMsg("StartTask Melee\n");
									  Vector flEnemyLKP = GetEnemyLKP();
									  GetMotor()->SetIdealYawToTarget(flEnemyLKP);
	}
		return;
	default:
		BaseClass::StartTask(pTask);
		break;
	}
}
void CNPC_Combine_synth_elite::RunTask(const Task_t *pTask)
{
	/*	switch ( pTask->iTask )
	{
	case TASK_CGUARD_RANGE_ATTACK1:
	{
	Vector flEnemyLKP = GetEnemyLKP();
	GetMotor()->SetIdealYawToTargetAndUpdate( flEnemyLKP );
	if ( IsActivityFinished() )
	{
	TaskComplete();
	return;
	}
	}
	return;
	}
	BaseClass::RunTask( pTask ); */
	switch (pTask->iTask)
	{
	case TASK_CGUARD_MELEE_ATTACK1:
	{
									  DevMsg("RunTask Melee\n");
									  Vector flEnemyLKP = GetEnemyLKP();
									  GetMotor()->SetIdealYawToTargetAndUpdate(flEnemyLKP);

									  SetActivity(ACT_MELEE_ATTACK1);

									  TaskComplete();
	}
		break;

	case TASK_RANGE_ATTACK1:
	{
							   AutoMovement();

							   Vector vecEnemyLKP = GetEnemyLKP();
							   if (!FInAimCone(vecEnemyLKP))
							   {
								   GetMotor()->SetIdealYawToTargetAndUpdate(vecEnemyLKP, AI_KEEP_YAW_SPEED);
							   }
							   else
							   {
								   GetMotor()->SetIdealYawAndUpdate(GetMotor()->GetIdealYaw(), AI_KEEP_YAW_SPEED);
							   }

							   if (gpGlobals->curtime >= m_flNextAttack)
							   {
								   if (IsActivityFinished())
								   {
									   DevMsg("TASK_RANGE_ATTACK1 complete\n");
									   TaskComplete();
								   }
							   }
							   else
							   {
								   DevMsg("Wait\n");
							   }
	}
		break;

	default:
		BaseClass::RunTask(pTask);
		break;
	}
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int	CNPC_Combine_synth_elite::OnTakeDamage_Alive(const CTakeDamageInfo &info)
{
	CTakeDamageInfo newInfo = info;

	float flDmgScale = 1.0;
	if (info.GetDamageType() & DMG_BULLET)
	{

		if (newInfo.GetAmmoType() == GetAmmoDef()->Index("LargeRound"))
		{
			flDmgScale = 0.85;
		}
		if (newInfo.GetAmmoType() == GetAmmoDef()->Index("SniperRound"))
		{
			flDmgScale = 1.85;
		}
		if (newInfo.GetAmmoType() == GetAmmoDef()->Index("Pistol"))
		{
			flDmgScale = 0.05;
		}
		if (newInfo.GetAmmoType() == GetAmmoDef()->Index("MediumRound"))
		{
			flDmgScale = 0.25;
		}

		else
		{
			flDmgScale = 0.035;
		}
	}

	if (info.GetDamageType() & DMG_SLASH)
	{
		flDmgScale = 0.0; // CGuard doesn't take damage from common melee attacks
	}

	if (flDmgScale != 0)
	{
		float flGlobalDamage = newInfo.GetDamage() * flDmgScale;
		newInfo.SetDamage(flGlobalDamage);
	}
	return BaseClass::OnTakeDamage_Alive(newInfo);
}
#define	DEBUG_AIMING	0

void CNPC_Combine_synth_elite::TraceAttack(const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr)
{
	if (ptr->hitgroup != 0)
	{
		DamageArmorPiece(ptr->hitgroup, info.GetDamage(), ptr->endpos, vecDir);
	}

	BaseClass::TraceAttack(info, vecDir, ptr, 0);
}

void CNPC_Combine_synth_elite::Event_Killed(const CTakeDamageInfo &info)
{
	BaseClass::Event_Killed(info);
}
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pieceID - 
//-----------------------------------------------------------------------------
void CNPC_Combine_synth_elite::DestroyArmorPiece(int pieceID)
{
	int	refPoint = GetReferencePointForBodyGroup(pieceID);

	if (refPoint == CGUARD_REF_INVALID)
		return;

	Vector	vecDamagePoint;
	QAngle	vecDamageAngles;

	GetAttachment(refPoint, vecDamagePoint, vecDamageAngles);

	CPVSFilter filter(vecDamagePoint);

	ExplosionCreate(vecDamagePoint, vecDamageAngles, this, 4.0f, 0, false);

	m_armorPieces[pieceID].m_bDestroyed = true;
	SetBodygroup(pieceID, 1);

	DevMsg("Armor Piece %i destroyed\n", pieceID);

	//For interrupting our behavior
	SetCondition(COND_LIGHT_DAMAGE);
}
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pieceID - 
//			damage - 
//-----------------------------------------------------------------------------
void CNPC_Combine_synth_elite::DamageArmorPiece(int pieceID, float damage, const Vector &vecOrigin, const Vector &vecDir)
{
	//Destroyed pieces take no damage
	if (m_armorPieces[pieceID].m_bDestroyed)
	{
		if ((random->RandomInt(0, 8) == 0))
		{
			g_pEffects->Ricochet(vecOrigin, (vecDir*-1.0f));
		}

		return;
	}
	//Take the damage
	m_armorPieces[pieceID].m_iHealth -= damage;

	//See if we've destroyed this piece
	if (m_armorPieces[pieceID].m_iHealth <= 0.0f)
	{
		DestroyArmorPiece(pieceID);
		return;
	}
	// Otherwise just spark
	g_pEffects->Sparks(vecOrigin);
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CNPC_Combine_synth_elite::AimGunAt(CBaseEntity *pEntity, float flInterval)
{
	if (!pEntity)
		return true;
	matrix3x4_t gunMatrix;
	GetAttachment(m_MuzzleAttachment, gunMatrix);
	// transform the enemy into gun space
	Vector localEnemyPosition;
	VectorITransform(pEntity->GetAbsOrigin(), gunMatrix, localEnemyPosition);
	// do a look at in gun space (essentially a delta-lookat)
	QAngle localEnemyAngles;
	VectorAngles(localEnemyPosition, localEnemyAngles);
	// convert to +/- 180 degrees
	localEnemyAngles.x = UTIL_AngleDiff(localEnemyAngles.x, 0);
	localEnemyAngles.y = UTIL_AngleDiff(localEnemyAngles.y, 0);
	float	targetYaw = m_aimYaw + localEnemyAngles.y;
	float	targetPitch = m_aimPitch + localEnemyAngles.x;
	QAngle	unitAngles = localEnemyAngles;
	float	angleDiff = sqrt(localEnemyAngles.y * localEnemyAngles.y + localEnemyAngles.x * localEnemyAngles.x);
	const float aimSpeed = 1;
	// Exponentially approach the target
	float yawSpeed = fabsf(aimSpeed*flInterval*localEnemyAngles.y);
	float pitchSpeed = fabsf(aimSpeed*flInterval*localEnemyAngles.x);

	yawSpeed = max(yawSpeed, 15);
	pitchSpeed = max(pitchSpeed, 15);
	m_aimYaw = UTIL_Approach(targetYaw, m_aimYaw, yawSpeed);
	m_aimPitch = UTIL_Approach(targetPitch, m_aimPitch, pitchSpeed);

	SetPoseParameter(m_YawControl, m_aimYaw);
	SetPoseParameter(m_PitchControl, m_aimPitch);
	// read back to avoid drift when hitting limits
	// as long as the velocity is less than the delta between the limit and 180, this is fine.
	m_aimPitch = GetPoseParameter(m_PitchControl);
	m_aimYaw = GetPoseParameter(m_YawControl);
	// UNDONE: Zero out any movement past the limit and go ahead and fire if the strider hit its 
	// target except for clamping.  Need to clamp targets to limits and compare?
	if (angleDiff < 1)
	{
		return true;
	}

	return false;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CNPC_Combine_synth_elite::MaxYawSpeed(void)
{
	if (GetActivity() == ACT_RANGE_ATTACK1)
	{
		return 1.0f;
	}
	return 60.0f;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
/*
int CNPC_Combine_synth_elite::TranslateSchedule( int type )
{
switch( type )
{
case SCHED_RANGE_ATTACK1:
return SCHED_CGUARD_RANGE_ATTACK1;
break;
}
return BaseClass::TranslateSchedule( type );
}
*/
void CNPC_Combine_synth_elite::PrescheduleThink(void)
{
	BaseClass::PrescheduleThink();
	if (GetEnemy() != NULL)
	{
		AimGunAt(GetEnemy(), 0.1f);
	}
}
int CNPC_Combine_synth_elite::MeleeAttack1Conditions(float flDot, float flDist)
{
	if (flDist > COMBINEGUARD_MELEE1_RANGE)
		return COND_TOO_FAR_TO_ATTACK;
	if (flDot < COMBINEGUARD_MELEE1_CONE)
		return COND_NOT_FACING_ATTACK;
	return COND_CAN_MELEE_ATTACK1;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CNPC_Combine_synth_elite::SelectSchedule(void)
{
	if (HasCondition(COND_CAN_MELEE_ATTACK1))
	{
		return SCHED_CGUARD_MELEE_ATTACK1;
	}
	/*	if( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
	{
	return SCHED_CGUARD_RANGE_ATTACK1;
	} */
	if (HasCondition(COND_TOO_CLOSE_TO_ATTACK))
	{
		return SCHED_CGUARD_RUN_TO_MELEE;
	}
	return BaseClass::SelectSchedule();
}
Activity CNPC_Combine_synth_elite::NPC_TranslateActivity(Activity baseAct)
{
	if (baseAct == ACT_RUN)
	{
		// Don't run at all.
		//return (Activity)ACT_WALK;
		return (Activity)ACT_RUN;
	}
	//Translate our idle if we're angry
	if (baseAct == ACT_IDLE && m_NPCState != NPC_STATE_IDLE)
	{
		return (Activity)ACT_IDLE_ANGRY;
	}
	return baseAct;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
AI_BEGIN_CUSTOM_NPC(npc_combine_synth_elite, CNPC_Combine_synth_elite)
//Interaction	DECLARE_INTERACTION( g_interactionAntlionAttacked )
//Tasks	
//	DECLARE_TASK( TASK_CGUARD_RANGE_ATTACK1 )
DECLARE_TASK(TASK_CGUARD_MELEE_ATTACK1)
DECLARE_TASK(TASK_COMBINEGUARD_SET_BALANCE)
//Conditions	
DECLARE_CONDITION(COND_COMBINEGUARD_CLOBBERED)
//Activities	
DECLARE_ACTIVITY(ACT_CGUARD_IDLE_TO_ANGRY)
DECLARE_ACTIVITY(ACT_COMBINEGUARD_CLOBBERED)
DECLARE_ACTIVITY(ACT_COMBINEGUARD_TOPPLE)
DECLARE_ACTIVITY(ACT_COMBINEGUARD_GETUP)
DECLARE_ACTIVITY(ACT_COMBINEGUARD_HELPLESS)
//==================================================
// SCHED_CGUARD_RANGE_ATTACK1
//==================================================
/*	DEFINE_SCHEDULE
(
SCHED_CGUARD_RANGE_ATTACK1,

"	Tasks"
"		TASK_STOP_MOVING			0"
"		TASK_FACE_ENEMY				0"
"		TASK_ANNOUNCE_ATTACK		1"
//	"		TASK_CGUARD_WARMUP_ATTACK1	0"
"		TASK_CGUARD_RANGE_ATTACK1	0"
"	"
"	Interrupts"
"		COND_NEW_ENEMY"
"		COND_ENEMY_DEAD"
"		COND_NO_PRIMARY_AMMO"
"		COND_CAN_MELEE_ATTACK1"
) */
//==================================================
// SCHED_CGUARD_MELEE_ATTACK1
//==================================================	
DEFINE_SCHEDULE
(
SCHED_CGUARD_MELEE_ATTACK1,

"	Tasks"
"		TASK_STOP_MOVING			0"
"		TASK_FACE_ENEMY				0"
"		TASK_ANNOUNCE_ATTACK		1"
"		TASK_CGUARD_MELEE_ATTACK1	0"
"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_MELEE_ATTACK1"
"	"
"	Interrupts"
"		COND_ENEMY_DEAD"
)
DEFINE_SCHEDULE
(
SCHED_CGUARD_RUN_TO_MELEE,

"	Tasks"
"		TASK_SET_TOLERANCE_DISTANCE		92"
"		TASK_GET_PATH_TO_ENEMY			0"
"		TASK_RUN_PATH					0"
"	"
"	Interrupts"
"		COND_ENEMY_DEAD"
"		COND_CAN_RANGE_ATTACK1"
)
DEFINE_SCHEDULE
(
SCHED_COMBINEGUARD_CLOBBERED,

"	Tasks"
"		TASK_STOP_MOVING						0"
"		TASK_COMBINEGUARD_SET_BALANCE			1"
"		TASK_PLAY_SEQUENCE						ACTIVITY:ACT_COMBINEGUARD_CLOBBERED"
"		TASK_COMBINEGUARD_SET_BALANCE			0"
"	"
"	Interrupts"
)
//==================================================
//==================================================
DEFINE_SCHEDULE
(
SCHED_COMBINEGUARD_TOPPLE,

"	Tasks"
"		TASK_STOP_MOVING				0"
"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_COMBINEGUARD_TOPPLE"
"		TASK_WAIT						1"
"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_COMBINEGUARD_GETUP"
"		TASK_COMBINEGUARD_SET_BALANCE	0"
"	"
"	Interrupts"
)
//==================================================
//==================================================
DEFINE_SCHEDULE
(
SCHED_COMBINEGUARD_HELPLESS,

"	Tasks"
"	TASK_STOP_MOVING				0"
"	TASK_PLAY_SEQUENCE				ACTIVITY:ACT_COMBINEGUARD_TOPPLE"
"	TASK_WAIT						2"
"	TASK_PLAY_SEQUENCE				ACTIVITY:ACT_COMBINEGUARD_HELPLESS"
"	TASK_WAIT_INDEFINITE			0"
"	"
"	Interrupts"
)
AI_END_CUSTOM_NPC()


