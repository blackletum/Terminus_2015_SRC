//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Flare gun (fffsssssssssss!!)
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "basehlcombatweapon.h"
#include "decals.h"
#include "soundenvelope.h"
#include "IEffects.h"
#include "engine/IEngineSound.h"
#include "env_flaregun.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


/********************************************************************
NOTE: if you are looking at this file becase you would like flares
to be considered as fires (and thereby trigger gas traps), be aware
that the env_flare class is actually found in weapon_flaregun.cpp
and is really a repurposed piece of ammunition. (env_flare isn't the
rod-like safety flare prop, but rather the bit of flame on the end.)

You will have some difficulty making it work here, because CFlare
does not inherit from CFire and will thus not be enumerated by
CFireSphere::EnumElement(). In order to have flares be detected and
used by this system, you will need to promote certain member functions
of CFire into an interface class from which both CFire and CFlare
inherit. You will also need to modify CFireSphere::EnumElement so that
it properly disambiguates between fires and flares.

For some partial work towards this end, see changelist 192474.

********************************************************************/


#define	FLARE_LAUNCH_SPEED	1500


IMPLEMENT_SERVERCLASS_ST(CFlaregun, DT_Flaregun)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(weapon_flaregun, CFlaregun);
PRECACHE_WEAPON_REGISTER(weapon_flaregun);

//-----------------------------------------------------------------------------
// Purpose: Precache
//-----------------------------------------------------------------------------
void CFlaregun::Precache(void)
{
	BaseClass::Precache();

	PrecacheScriptSound("Flare.Touch");

	PrecacheScriptSound("Weapon_FlareGun.Burn");

	UTIL_PrecacheOther("env_flare");
}

//-----------------------------------------------------------------------------
// Purpose: Main attack
//-----------------------------------------------------------------------------
void CFlaregun::PrimaryAttack(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (pOwner == NULL)
		return;

	if (m_iClip1 <= 0)
	{
		SendWeaponAnim(ACT_VM_DRYFIRE);
		pOwner->m_flNextAttack = gpGlobals->curtime + SequenceDuration();
		return;
	}

	m_iClip1 = m_iClip1 - 1;

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);
	pOwner->m_flNextAttack = gpGlobals->curtime + 1;

	CFlare *pFlare = CFlare::Create(pOwner->Weapon_ShootPosition(), pOwner->EyeAngles(), pOwner, FLARE_DURATION);

	if (pFlare == NULL)
		return;

	Vector forward;
	pOwner->EyeVectors(&forward);

	pFlare->SetAbsVelocity(forward * 1500);

	WeaponSound(SINGLE);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFlaregun::SecondaryAttack(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (pOwner == NULL)
		return;

	if (m_iClip1 <= 0)
	{
		SendWeaponAnim(ACT_VM_DRYFIRE);
		pOwner->m_flNextAttack = gpGlobals->curtime + SequenceDuration();
		return;
	}

	m_iClip1 = m_iClip1 - 1;

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);
	pOwner->m_flNextAttack = gpGlobals->curtime + 1;

	CFlare *pFlare = CFlare::Create(pOwner->Weapon_ShootPosition(), pOwner->EyeAngles(), pOwner, FLARE_DURATION);

	if (pFlare == NULL)
		return;

	Vector forward;
	pOwner->EyeVectors(&forward);

	pFlare->SetAbsVelocity(forward * 500);
	pFlare->SetGravity(1.0f);
	pFlare->SetFriction(0.85f);
	pFlare->SetMoveType(MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE);

	WeaponSound(SINGLE);
}


