//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: The AK-Whatever. 
//
// TODO: I gotta find a way to fix the problem where the NPCs single-fire this, the HMG, and SMG2
//=============================================================================//

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "NPCevent.h"
#include "basecombatcharacter.h"
#include "AI_BaseNPC.h"
#include "npc_combine.h"
#include "player.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define DAMAGE_PER_SECOND 10 //Are these necessary?

#define MAX_SETTINGS	2

float RateOfFire[ MAX_SETTINGS ] = 
{
//	0.07, //0.1 by default
	0.1, //0.2 by default
	0.2, //0.5 by default
//	0.5, //0.7 by default
//	1.0, //1.0 by default
};

float Damage[ MAX_SETTINGS ] =
{
//	2,
	4,
	10,
//	14,
//	20,
};


//=========================================================
//=========================================================
class CWeaponAR1 : public CHLMachineGun
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CWeaponAR1, CHLMachineGun );

	DECLARE_SERVERCLASS();

	CWeaponAR1();

	int m_ROF;

	void	Precache( void );
	bool	Deploy( void );

	float GetFireRate( void ) {return RateOfFire[ m_ROF ];}

	int CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	void SecondaryAttack( void );

	virtual void FireBullets( const FireBulletsInfo_t &info );

	virtual const Vector& GetBulletSpread( void )
	{
		static const Vector cone = VECTOR_CONE_3DEGREES;
		return cone;
	}

	void FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles );

	void Operator_ForceNPCFire( CBaseCombatCharacter  *pOperator, bool bSecondary );

	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	DECLARE_ACTTABLE();
};

IMPLEMENT_SERVERCLASS_ST(CWeaponAR1, DT_WeaponAR1)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_ar1, CWeaponAR1 );
PRECACHE_WEAPON_REGISTER(weapon_ar1);

acttable_t	CWeaponAR1::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_AR2,			true },
	{ ACT_RELOAD,					ACT_RELOAD_SMG1,				true },		// FIXME: hook to OICW unique
	{ ACT_IDLE,						ACT_IDLE_SMG1,					true },		// FIXME: hook to OICW unique
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SMG1,			true },		// FIXME: hook to OICW unique

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
	{ ACT_COVER_LOW,				ACT_COVER_SMG1_LOW,				false },		// FIXME: hook to OICW unique
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_AR2_LOW,			false },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SMG1_LOW,		true },		// FIXME: hook to OICW unique
	{ ACT_RELOAD_LOW,				ACT_RELOAD_SMG1_LOW,			false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SMG1,		true },
//	{ ACT_RANGE_ATTACK2, ACT_RANGE_ATTACK_OICW_GRENADE, true },
};


IMPLEMENT_ACTTABLE(CWeaponAR1);

//===========================================================
//void CWeaponAR1::Equip( CBaseCombatCharacter *pOwner )
//{
//	if( pOwner->Classify() == CLASS_PLAYER_ALLY )
//	{
//		m_fMaxRange1 = 3000;
//	}
//	else
//	{
//		m_fMaxRange1 = 1400;
//	}
//
//	BaseClass::Equip( pOwner );
//}
// NPC usage, Y U NO WORK?!?
//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------

void CWeaponAR1::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles )
{
	Vector vecShootOrigin, vecShootDir;

	CAI_BaseNPC *npc = pOperator->MyNPCPointer();
	ASSERT( npc != NULL );

	if ( bUseWeaponAngles )
	{
		QAngle	angShootDir;
		GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
		AngleVectors( angShootDir, &vecShootDir );
	}
	else 
	{
		vecShootOrigin = pOperator->Weapon_ShootPosition();
		vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );
	}

	WeaponSoundRealtime( SINGLE_NPC );

	CSoundEnt::InsertSound( SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy() );

	pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_3DEGREES, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2 );

	// NOTENOTE: This is overriden on the client-side
	// pOperator->DoMuzzleFlash();

	m_iClip1 = m_iClip1 - 1;
}

void CWeaponAR1::Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary ) //This ain't needed right now, but it might have something to do with automatic fire.
{
	// Ensure we have enough rounds in the clip
//	m_iClip1++;
//
//	Vector vecShootOrigin, vecShootDir;
//	QAngle	angShootDir;
//	GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
//	AngleVectors( angShootDir, &vecShootDir );
//	FireNPCPrimaryAttack( pOperator, vecShootOrigin, vecShootDir );
}

void CWeaponAR1::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{ 
		case EVENT_WEAPON_AR2:
			{
				FireNPCPrimaryAttack( pOperator, false );
			}
			break;

//		case EVENT_WEAPON_AR2_ALTFIRE:
//			{
//				FireNPCSecondaryAttack( pOperator, false );
//			}
//			break;

		default:
			CBaseCombatWeapon::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}
}

BEGIN_DATADESC( CWeaponAR1 )

	DEFINE_FIELD( m_ROF,			FIELD_INTEGER ),

END_DATADESC()


CWeaponAR1::CWeaponAR1( )
{
	m_ROF = 0;
}

void CWeaponAR1::Precache( void )
{
	BaseClass::Precache();
}

bool CWeaponAR1::Deploy( void )
{
	//CBaseCombatCharacter *pOwner  = m_hOwner;
	return BaseClass::Deploy();
}


//=========================================================
//=========================================================
void CWeaponAR1::FireBullets( const FireBulletsInfo_t &info )
{
	if(CBasePlayer *pPlayer = ToBasePlayer( GetOwner() ))
	{
		pPlayer->FireBullets( info );
	}
}


void CWeaponAR1::SecondaryAttack( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( pPlayer )
	{
		pPlayer->m_nButtons &= ~IN_ATTACK2;
	}

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.1;

	m_ROF += 1;

	if( m_ROF == MAX_SETTINGS )
	{
		m_ROF = 0;
	}

}
