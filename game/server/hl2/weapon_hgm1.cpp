//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: Heavy machine gun (currently an AK-47).
//
//=============================================================================

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "basecombatweapon.h"
#include "in_buttons.h"
#include "NPCevent.h"
#include "basecombatcharacter.h"
#include "AI_BaseNPC.h"
#include "weapon_hmg1.h"

IMPLEMENT_SERVERCLASS_ST(CWeaponHMG1, DT_WeaponHMG1)
END_SEND_TABLE()



BEGIN_DATADESC(CWeaponHMG1)

DEFINE_FIELD(m_flNextBurstFireTime, FIELD_FLOAT),
DEFINE_FIELD(m_bZoomed, FIELD_BOOLEAN),

END_DATADESC()

LINK_ENTITY_TO_CLASS(weapon_hmg1, CWeaponHMG1);

acttable_t	CWeaponHMG1::m_acttable[] =
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_AR2, true },
	{ ACT_RELOAD, ACT_RELOAD_SMG1, true },		// FIXME: hook to AR2 unique
	{ ACT_IDLE, ACT_IDLE_SMG1, true },		// FIXME: hook to AR2 unique
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_SMG1, true },		// FIXME: hook to AR2 unique

	{ ACT_WALK, ACT_WALK_RIFLE, true },

	// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED, ACT_IDLE_SMG1_RELAXED, false },//never aims
	{ ACT_IDLE_STIMULATED, ACT_IDLE_SMG1_STIMULATED, false },
	{ ACT_IDLE_AGITATED, ACT_IDLE_ANGRY_SMG1, false },//always aims

	{ ACT_WALK_RELAXED, ACT_WALK_RIFLE_RELAXED, false },//never aims
	{ ACT_WALK_STIMULATED, ACT_WALK_RIFLE_STIMULATED, false },
	{ ACT_WALK_AGITATED, ACT_WALK_AIM_RIFLE, false },//always aims

	{ ACT_RUN_RELAXED, ACT_RUN_RIFLE_RELAXED, false },//never aims
	{ ACT_RUN_STIMULATED, ACT_RUN_RIFLE_STIMULATED, false },
	{ ACT_RUN_AGITATED, ACT_RUN_AIM_RIFLE, false },//always aims

	// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED, ACT_IDLE_SMG1_RELAXED, false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED, ACT_IDLE_AIM_RIFLE_STIMULATED, false },
	{ ACT_IDLE_AIM_AGITATED, ACT_IDLE_ANGRY_SMG1, false },//always aims

	{ ACT_WALK_AIM_RELAXED, ACT_WALK_RIFLE_RELAXED, false },//never aims
	{ ACT_WALK_AIM_STIMULATED, ACT_WALK_AIM_RIFLE_STIMULATED, false },
	{ ACT_WALK_AIM_AGITATED, ACT_WALK_AIM_RIFLE, false },//always aims

	{ ACT_RUN_AIM_RELAXED, ACT_RUN_RIFLE_RELAXED, false },//never aims
	{ ACT_RUN_AIM_STIMULATED, ACT_RUN_AIM_RIFLE_STIMULATED, false },
	{ ACT_RUN_AIM_AGITATED, ACT_RUN_AIM_RIFLE, false },//always aims
	//End readiness activities

	{ ACT_WALK_AIM, ACT_WALK_AIM_RIFLE, true },
	{ ACT_WALK_CROUCH, ACT_WALK_CROUCH_RIFLE, true },
	{ ACT_WALK_CROUCH_AIM, ACT_WALK_CROUCH_AIM_RIFLE, true },
	{ ACT_RUN, ACT_RUN_RIFLE, true },
	{ ACT_RUN_AIM, ACT_RUN_AIM_RIFLE, true },
	{ ACT_RUN_CROUCH, ACT_RUN_CROUCH_RIFLE, true },
	{ ACT_RUN_CROUCH_AIM, ACT_RUN_CROUCH_AIM_RIFLE, true },
	{ ACT_GESTURE_RANGE_ATTACK1, ACT_GESTURE_RANGE_ATTACK_AR2, false },
	{ ACT_COVER_LOW, ACT_COVER_SMG1_LOW, false },		// FIXME: hook to AR2 unique
	{ ACT_RANGE_AIM_LOW, ACT_RANGE_AIM_AR2_LOW, false },
	{ ACT_RANGE_ATTACK1_LOW, ACT_RANGE_ATTACK_SMG1_LOW, true },		// FIXME: hook to AR2 unique
	{ ACT_RELOAD_LOW, ACT_RELOAD_SMG1_LOW, false },
	{ ACT_GESTURE_RELOAD, ACT_GESTURE_RELOAD_SMG1, true },
	//	{ ACT_RANGE_ATTACK2, ACT_RANGE_ATTACK_AR2_GRENADE, true },
};

IMPLEMENT_ACTTABLE(CWeaponHMG1);

//=========================================================
CWeaponHMG1::CWeaponHMG1()
{
	m_fMinRange1 = 65;
	m_fMaxRange1 = 2048;

	m_fMinRange2 = 256;
	m_fMaxRange2 = 1024;

	m_nShotsFired = 0;

}

void CWeaponHMG1::Precache(void)
{
	engine->PrecacheModel("models/weapons/insolence/v_hmg.mdl"); // Are these necessary?.. Doubt that!
	engine->PrecacheModel("models/weapons/insolence/w_hmg.mdl");
	BaseClass::Precache();
}

bool CWeaponHMG1::Deploy(void)
{
	//CBaseCombatCharacter *pOwner  = m_hOwner;
	m_nShotsFired = 0;
	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : NULL - 
//-----------------------------------------------------------------------------
bool CWeaponHMG1::Holster(CBaseCombatWeapon *pSwitchingTo)
{

	return BaseClass::Holster(pSwitchingTo);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponHMG1::Reload(void)
{

	//	EmitSound( "Weapon_OICW.Reload" ); 
	//	There is no need in this. In fact, it causes the weapon to play the sound with a reload button pressed even if the clip is full.

	return BaseClass::Reload();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponHMG1::Drop(const Vector &velocity)
{

	BaseClass::Drop(velocity);
}

void CWeaponHMG1::ItemPostFrame(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
		return;

	if ((pOwner->m_nButtons & IN_ATTACK) == false)
	{
		m_nShotsFired = 0;

	}

	////Zoom in
	//if (pOwner->m_afButtonPressed & IN_ATTACK2)
	//{
	//	Zoom();
	//}

	BaseClass::ItemPostFrame();
}

void CWeaponHMG1::AddViewKick(void)
{
#define	EASY_DAMPEN			0.5f
#define	MAX_VERTICAL_KICK	14.0f	//Degrees
#define	SLIDE_LIMIT			3.0f	//Seconds

	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (!pPlayer)
		return;

	DoMachineGunKick(pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT);
}

Activity CWeaponHMG1::GetPrimaryAttackActivity(void)
{
	return ACT_VM_PRIMARYATTACK;
}

float CWeaponHMG1::GetFireRate(void)
{
	if (m_bZoomed)
		return 0.3f;

	return 0.1f;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CWeaponHMG1::PrimaryAttack(void)
{
	//pOperator->DoMuzzleFlash();
	m_nShotsFired++;
	BaseClass::PrimaryAttack();
}

void CWeaponHMG1::Zoom(void)
{
	//CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	//if (pPlayer == NULL)
	//	return;

	//// TODO: first, make some decent effect, darken screen corners, graduations, distance meters. Second, make the overlay for NPCs.
	//color32 lightGreen = { 50, 255, 170, 32 };


	//if (m_bZoomed)
	//{
	//	pPlayer->ShowViewModel(true);

	//	// Zoom out to the default zoom level
	//	WeaponSound(SPECIAL2);
	//	pPlayer->SetFOV(this, 0, 0.1f);
	//	m_bZoomed = false;

	//	UTIL_ScreenFade(pPlayer, lightGreen, 0.2f, 0, (FFADE_IN | FFADE_PURGE));
	//}
	//else
	//{
	//	pPlayer->ShowViewModel(false);

	//	WeaponSound(SPECIAL1);
	//	pPlayer->SetFOV(this, 35, 0.1f);
	//	m_bZoomed = true;

	//	UTIL_ScreenFade(pPlayer, lightGreen, 0.2f, 0, (FFADE_OUT | FFADE_PURGE | FFADE_STAYOUT));
	//}
}

void CWeaponHMG1::Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator)
{
	switch (pEvent->event)
	{
	case EVENT_WEAPON_AR2: // Requires that the weapon has "ar2" as its anim_prefix, and the AR2-oriented acttable
	{
							   Vector vecShootOrigin, vecShootDir;
							   vecShootOrigin = pOperator->Weapon_ShootPosition();

							   CAI_BaseNPC *npc = pOperator->MyNPCPointer();
							   ASSERT(npc != NULL);
							   vecShootDir = npc->GetActualShootTrajectory(vecShootOrigin);

							   FireNPCPrimaryAttack(pOperator, vecShootOrigin, vecShootDir);
	}

	case EVENT_WEAPON_AR2_ALTFIRE:
	{
									 Vector vecShootOrigin, vecShootDir;
									 vecShootOrigin = pOperator->Weapon_ShootPosition();

									 CAI_BaseNPC *npc = pOperator->MyNPCPointer();
									 ASSERT(npc != NULL);
									 vecShootDir = npc->GetActualShootTrajectory(vecShootOrigin);

									 FireNPCSecondaryAttack(pOperator, vecShootOrigin, vecShootDir);
	}

		break;
	default:
		CBaseCombatWeapon::Operator_HandleAnimEvent(pEvent, pOperator);
		break;
	}
}

void CWeaponHMG1::FireNPCPrimaryAttack(CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir)
{
	WeaponSoundRealtime(SINGLE_NPC);

	CSoundEnt::InsertSound(SOUND_COMBAT | SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy());

	pOperator->FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_3DEGREES, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2);

	// NOTENOTE: This is overriden on the client-side
	//pOperator->m_fEffects |= MUZZLEFLASH_SMG1;

	m_iClip1 = m_iClip1 - 1;
}

// Since the primary and the secondary attack are practically the same (the same ammo type, the weapon fires on the same principal,
// just fires 3 bullets instead of 1 - so I practically cloned the FireNPCPrimaryAttack() here.
void CWeaponHMG1::FireNPCSecondaryAttack(CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir)
{
	WeaponSoundRealtime(WPN_DOUBLE);

	CSoundEnt::InsertSound(SOUND_COMBAT | SOUND_CONTEXT_GUNFIRE,
		pOperator->GetAbsOrigin(),
		SOUNDENT_VOLUME_MACHINEGUN, 0.2,
		pOperator, SOUNDENT_CHANNEL_WEAPON,
		pOperator->GetEnemy());


	pOperator->FireBullets(1, vecShootOrigin,
		vecShootDir,
		VECTOR_CONE_3DEGREES,
		MAX_TRACE_LENGTH,
		m_iPrimaryAmmoType, 2);
	m_iClip1 = m_iClip1 - 1;

	DevMsg("BurstFire 1\n");

	SetNextThink(gpGlobals->curtime + 2.0);

	pOperator->FireBullets(1, vecShootOrigin,
		vecShootDir,
		VECTOR_CONE_3DEGREES,
		MAX_TRACE_LENGTH,
		m_iPrimaryAmmoType, 2);
	m_iClip1 = m_iClip1 - 1;

	DevMsg("BurstFire 2\n");

	SetNextThink(gpGlobals->curtime + 2.0);

	pOperator->FireBullets(1, vecShootOrigin,
		vecShootDir,
		VECTOR_CONE_3DEGREES,
		MAX_TRACE_LENGTH,
		m_iPrimaryAmmoType, 2);
	m_iClip1 = m_iClip1 - 1;

	DevMsg("BurstFire 3\n");

	// NOTENOTE: This is overriden on the client-side
	//pOperator->m_fEffects |= MUZZLEFLASH_SMG1;


	m_flNextSecondaryAttack = gpGlobals->curtime + 5.0;
}

void CWeaponHMG1::Operator_ForceNPCFire(CBaseCombatCharacter *pOperator, bool bSecondary)
{
	if (bSecondary)
	{
		m_iClip1++;

		Vector vecShootOrigin, vecShootDir;
		QAngle	angShootDir;
		GetAttachment(LookupAttachment("muzzle"), vecShootOrigin, angShootDir);
		AngleVectors(angShootDir, &vecShootDir);

		FireNPCSecondaryAttack(pOperator, vecShootOrigin, vecShootDir);
	}
	else
	{
		// Ensure we have enough rounds in the clip
		m_iClip1++;

		Vector vecShootOrigin, vecShootDir;
		QAngle	angShootDir;
		GetAttachment(LookupAttachment("muzzle"), vecShootOrigin, angShootDir);
		AngleVectors(angShootDir, &vecShootDir);

		FireNPCSecondaryAttack(pOperator, vecShootOrigin, vecShootDir);
	}
}