//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "basecombatweapon.h"
#include "NPCevent.h"
#include "basecombatcharacter.h"
#include "AI_BaseNPC.h"
#include "player.h"
#include "weapon_OICW.h"
#include "grenade_ar2.h"
#include "gamerules.h"
#include "game.h"
#include "in_buttons.h"
#include "AI_Memory.h"
#include "shake.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterialvar.h"
#include "particle_parse.h"
//#include "env_screenoverlay.h"
#include "gamestats.h"

extern ConVar    sk_plr_dmg_smg1_grenade;	
extern ConVar    sk_npc_dmg_smg1_grenade;
extern ConVar    sk_npc_dmg_ar2_grenade;
extern ConVar    sk_max_ar2_grenade;
extern ConVar	 sk_ar2_grenade_radius;
#define AR2_ZOOM_RATE	0.5f	// Interval between zoom levels in seconds.

//=========================================================
//=========================================================

BEGIN_DATADESC( CWeaponOICW )

	DEFINE_FIELD( m_nShotsFired,	FIELD_INTEGER ),
	DEFINE_FIELD( m_bZoomed,		FIELD_BOOLEAN ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CWeaponOICW, DT_WeaponOICW)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_oicw, CWeaponOICW );
PRECACHE_WEAPON_REGISTER(weapon_oicw);

acttable_t	CWeaponOICW::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_AR2,			true },
	{ ACT_RELOAD,					ACT_RELOAD_SMG1,				true },		
	{ ACT_IDLE,						ACT_IDLE_SMG1,					true },		
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SMG1,			true },		

	{ ACT_WALK,						ACT_WALK_RIFLE,					true },

// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_SMG1_RELAXED,			false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_SMG1_STIMULATED,		false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_SMG1,			false },//always aims

	{ ACT_WALK_RELAXED,				ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_RIFLE_STIMULATED,		false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_RELAXED,				ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_RIFLE_STIMULATED,		false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_RIFLE,				false },//always aims

// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_SMG1_RELAXED,			false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_RIFLE_STIMULATED,	false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_SMG1,			false },//always aims

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_RIFLE_STIMULATED,	false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_RIFLE_STIMULATED,	false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_RIFLE,				false },//always aims
//End readiness activities

	{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				true },
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,			true },
	{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,		true },
	{ ACT_RUN,						ACT_RUN_RIFLE,					true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_RIFLE,				true },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,			true },
	{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,		true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_AR2,	false },
	{ ACT_COVER_LOW,				ACT_COVER_SMG1_LOW,				false },		
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_AR2_LOW,			false },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SMG1_LOW,		true },		
	{ ACT_RELOAD_LOW,				ACT_RELOAD_SMG1_LOW,			false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SMG1,		true },
};


IMPLEMENT_ACTTABLE(CWeaponOICW);

CWeaponOICW::CWeaponOICW( )
{
	m_fMinRange1	= 65;
	m_fMaxRange1	= 2048;

	m_fMinRange2	= 256;
	m_fMaxRange2	= 1024;

	m_nShotsFired	= 0;
}
void CWeaponOICW::Precache( void )
{
	UTIL_PrecacheOther("grenade_ar2");
	UTIL_PrecacheOther("env_screenoverlay");
	PrecacheParticleSystem("weapon_muzzle_flash_smoke_small2");
	PrecacheParticleSystem("weapon_muzzle_flash_assaultrifle");
	PrecacheScriptSound("Weapon_AR2.Reload");
	BaseClass::Precache();

}
//-----------------------------------------------------------------------------
// Purpose: Offset the autoreload
//-----------------------------------------------------------------------------
bool CWeaponOICW::Deploy( void )
{
	m_nShotsFired = 0;
	m_flRandomAnimate = gpGlobals->curtime + random->RandomFloat(5.0, 10.0);

	return BaseClass::Deploy();

	CBaseCombatCharacter* myOwner = ToBaseCombatCharacter(GetOwner());
	if (myOwner->IsNPC())
	{
		m_iClip2 = 6;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handle grenade detonate in-air (even when no ammo is left)
//-----------------------------------------------------------------------------
void CWeaponOICW::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if (!pOwner)
		return;

	if ( ( pOwner->m_nButtons & IN_ATTACK ) == false )
	{
		m_nShotsFired = 0;
	}


	if (pOwner->m_afButtonPressed & IN_ATTACK3)
	{
		Zoom();
	}
	if (pOwner->m_afButtonPressed & IN_ATTACK2 )
	{
		GLaunch();
	}



	//Zoom in
	//if ( pOwner->m_afButtonPressed & IN_ATTACK2 )
	//{
	//	Zoom();
	//}

	//Don't kick the same when we're zoomed in
	if ( m_bZoomed )
	{
		m_fFireDuration = 0.05f;
	}

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CWeaponOICW::GetPrimaryAttackActivity( void )
{
	if ( m_nShotsFired < 2 )
		return ACT_VM_PRIMARYATTACK;

	if ( m_nShotsFired < 3 )
		return ACT_VM_HITLEFT;
	
	if ( m_nShotsFired < 4 )
		return ACT_VM_HITLEFT2;

	return ACT_VM_HITRIGHT;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CWeaponOICW::PrimaryAttack( void )
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	// FIXME: probably an unnecessary feature?..
	m_nShotsFired++;
	DispatchParticleEffect("weapon_muzzle_flash_smoke_small2", PATTACH_POINT_FOLLOW, pPlayer->GetViewModel(), "muzzle", true);
	if (m_nShotsFired >= 1) // 15
		// if we don't fire half the clip in a row, the barrel doesn't get hot enough to produce muzzleflashes.
		// A weapon produces just some smoke if the shooter is not crazy enough
		// to flood an enemy in a stream of bullet, thus overheating a weapon itself (that's when the fire starts to come out of a barrel)
	{
		DispatchParticleEffect("weapon_muzzle_flash_assaultrifle", PATTACH_POINT_FOLLOW, pPlayer->GetViewModel(), "muzzle", true);
	}
	if ((pPlayer->m_nButtons & IN_ATTACK) == false)
	{
		m_nShotsFired = 0;
	}
	BaseClass::PrimaryAttack();

}

void CWeaponOICW::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch (pEvent->event)
	{
	case EVENT_WEAPON_SMG1:
	{
							  Vector vecShootOrigin, vecShootDir;
							  vecShootOrigin = pOperator->Weapon_ShootPosition();
							  QAngle	angShootDir;


							  CAI_BaseNPC *npc = pOperator->MyNPCPointer();
							  ASSERT(npc != NULL);
							  vecShootDir = npc->GetActualShootTrajectory(vecShootOrigin);

							  FireNPCPrimaryAttack(pOperator, vecShootOrigin, vecShootDir);
	}
		break;
	case EVENT_WEAPON_AR2_GRENADE:
	{
									 FireNPCSecondaryAttack(pOperator, true);
	}
		break;
	default:
	{
			   CBaseCombatWeapon::Operator_HandleAnimEvent(pEvent, pOperator);
			   break;
	}
	}
}

/*
==================================================
AddViewKick
==================================================
*/

void CWeaponOICW::AddViewKick( void )
{
	#define	EASY_DAMPEN			0.5f
	#define	MAX_VERTICAL_KICK	5.0f	//Degrees
	#define	SLIDE_LIMIT			3.0f	//Seconds
	
	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer)
		return;

	DoMachineGunKick( pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponOICW::Zoom( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	color32 lightGreen = { 50, 255, 170, 32 };

	if ( m_bZoomed )
	{
		pPlayer->ShowViewModel( true );

		// Zoom out to the default zoom level
		WeaponSound(SPECIAL2);
		pPlayer->SetFOV( this, 0, 0.1f );
		m_bZoomed = false;

		UTIL_ScreenFade( pPlayer, lightGreen, 0.2f, 0, (FFADE_IN|FFADE_PURGE) );
	}
	else
	{
		pPlayer->ShowViewModel( false );

		WeaponSound(SPECIAL1);
		pPlayer->SetFOV( this, 35, 0.1f );
		m_bZoomed = true;

		UTIL_ScreenFade( pPlayer, lightGreen, 0.2f, 0, (FFADE_OUT|FFADE_PURGE|FFADE_STAYOUT) );	
	}
}


void CWeaponOICW::GLaunch( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	if (pPlayer == NULL)
		return;
	//Must have ammo
	if ((pPlayer->GetAmmoCount(m_iSecondaryAmmoType) <= 0) || (pPlayer->GetWaterLevel() == 3))
	{
		SendWeaponAnim(ACT_VM_DRYFIRE);
		BaseClass::WeaponSound(EMPTY);
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;
		return;
	}
	if (m_bInReload)
		m_bInReload = false;

	// MUST call sound before removing a round from the clip of a CMachineGun
	BaseClass::WeaponSound(WPN_DOUBLE);

	//pPlayer->RumbleEffect(RUMBLE_357, 0, RUMBLE_FLAGS_NONE);

	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector	vecThrow;
	// Don't autoaim on grenade tosses
	AngleVectors(pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), &vecThrow);
	VectorScale(vecThrow, 1000.0f, vecThrow);

	//Create the grenade
	QAngle angles;
	VectorAngles(vecThrow, angles);
	CGrenadeAR2 *pGrenade = (CGrenadeAR2*)Create("grenade_ar2", vecSrc, angles, pPlayer);
	pGrenade->SetAbsVelocity(vecThrow);
	pGrenade->SetLocalAngularVelocity(RandomAngle(-400, 400));
	pGrenade->SetMoveType(MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE);
	pGrenade->SetThrower(GetOwner());
	pGrenade->SetDamage(sk_plr_dmg_smg1_grenade.GetFloat());

	SendWeaponAnim(ACT_VM_SECONDARYATTACK);

	CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), 1000, 0.2, GetOwner(), SOUNDENT_CHANNEL_WEAPON);

	// player "shoot" animation
	pPlayer->SetAnimation(PLAYER_ATTACK1);
	// Can shoot again immediately
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;
	// Can blow up after a short delay (so have time to release mouse button)
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.0f;
	// Register a muzzleflash for the AI.
	pPlayer->SetMuzzleFlashTime(gpGlobals->curtime + 0.5);
	m_iSecondaryAttacks++;
	gamestats->Event_WeaponFired(pPlayer, false, GetClassname());
	pPlayer->RemoveAmmo(1, m_iSecondaryAmmoType);

	BaseClass::SecondaryAttack();

}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CWeaponOICW::GetFireRate( void )
{ 
	if ( m_bZoomed )
		return 0.3f;

	return 0.1f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : NULL - 
//-----------------------------------------------------------------------------
bool CWeaponOICW::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( m_bZoomed )
	{
		Zoom();
	}

	return BaseClass::Holster( pSwitchingTo );
}

//npc attack start
void CWeaponOICW::FireNPCPrimaryAttack(CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir)
{
	WeaponSoundRealtime(SINGLE_NPC);

	CSoundEnt::InsertSound(SOUND_COMBAT | SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy());

	pOperator->FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_3DEGREES, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2);

	// NOTENOTE: This is overriden on the client-side
	//pOperator->m_fEffects |= MUZZLEFLASH_SMG1;

	m_iClip1 = m_iClip1 - 1;
}
void CWeaponOICW::FireNPCSecondaryAttack(CBaseCombatCharacter *pOperator, bool bUseWeaponAngles)
{
	WeaponSound(WPN_DOUBLE);

	if (!GetOwner())
		return;

	CAI_BaseNPC *pNPC = GetOwner()->MyNPCPointer();
	if (!pNPC)
		return;

	Vector vecTarget;

	CNPC_Combine *pSoldier = dynamic_cast<CNPC_Combine *>(pNPC);
	if (pSoldier)
	{
		// In the distant misty past, elite soldiers tried to use bank shots.
		// Therefore, we must ask them specifically what direction they are shooting.
		vecTarget = pSoldier->GetAltFireTarget();
	}
	else
	{
		// All other users of the AR2 alt-fire shoot directly at their enemy.
		if (!pNPC->GetEnemy())
			return;
	}

	// Fire!
	Vector vecSrc;
	Vector vecThrow;

	// Don't autoaim on grenade tosses
	AngleVectors(pNPC->EyeAngles(), &vecThrow);
	VectorScale(vecThrow, 1000.0f, vecThrow);

	//Create the grenade
	QAngle angles;
	VectorAngles(vecThrow, angles);

	vecSrc = pNPC->Weapon_ShootPosition();
	//vecSrc.y += 10.0f;
	//vecSrc.z += 40.0f;		

	CGrenadeAR2 *pGrenade = (CGrenadeAR2*)Create("grenade_ar2", vecSrc, angles, pNPC);
	pGrenade->SetAbsVelocity(vecThrow);
	pGrenade->SetLocalAngularVelocity(RandomAngle(-400, 400));
	pGrenade->SetMoveType(MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE);
	pGrenade->SetThrower(pNPC);
	pGrenade->SetDamage(sk_npc_dmg_smg1_grenade.GetFloat());

	// FIXME: arrgg ,this is hard coded into the weapon???
	SetNextThink(gpGlobals->curtime + 6);// wait six seconds before even looking again to see if a grenade can be thrown.

	m_iClip2 = m_iClip2 - 1;
}


void CWeaponOICW::Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary )
{
	if (bSecondary)
	{
		m_iClip2++;

		FireNPCSecondaryAttack(pOperator, false);
	}
	else
	{
		// Ensure we have enough rounds in the clip
		m_iClip1++;

		Vector vecShootOrigin, vecShootDir;
		QAngle	angShootDir;
		GetAttachment(LookupAttachment("muzzle"), vecShootOrigin, angShootDir);
		AngleVectors(angShootDir, &vecShootDir);

		FireNPCPrimaryAttack(pOperator, vecShootOrigin, vecShootDir);
	}
}


//npc attack end
//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponOICW::Reload( void )
{
	if ( m_bZoomed )
	{
		Zoom();
	}
	bool fRet;
	float fCacheTime = m_flNextSecondaryAttack;
//	fRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
//	bool fRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
	if ( fRet )
	{
		// Undo whatever the reload process has done to our secondary
		// attack timer. We allow you to interrupt reloading to fire
		// a grenade.
		m_flNextSecondaryAttack = GetOwner()->m_flNextAttack = fCacheTime;

		WeaponSound( RELOAD );
	}
	return BaseClass::Reload();

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponOICW::Drop( const Vector &velocity )
{
	if ( m_bZoomed )
	{
		Zoom();
	}

	BaseClass::Drop( velocity );
}
