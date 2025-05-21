//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "basehlcombatweapon.h"
#include "basecombatcharacter.h"
#include "player.h"
#include "soundent.h"
#include "te_particlesystem.h"
#include "te_effect_dispatch.h"
#include "ndebugoverlay.h"
#include "in_buttons.h"
#include "ai_basenpc.h"
#include "ai_memory.h"
#include "particle_parse.h"
#include "beam_shared.h"
#include "beam_flags.h"
#include "fire.h"
#include "NPCEvent.h"
#include "in_buttons.h"

#define MAX_BURN_RADIUS		512
#define MIN_BURN_RADIUS		0
#define RADIUS_GROW_RATE	50.0	// units/sec 
#define IMMO_BEAM_SPRITE		"sprites/crystal_beam1.vmt"
#define IMMO_BEAM2_SPRITE		"sprites/bluelaser1.vmt"
#define	IMMO_SPRAY					( 1 )

ConVar sk_immolator_consumption_full("sk_immolator_consumption_full", "0", FCVAR_NONE, "Defines ammo consumption in full attack mode");
ConVar sk_immolator_consumption_base("sk_immolator_consumption_base", "0", FCVAR_NONE, "Defines ammo consumption in base attack mode");

class CWeaponImmolator : public CBaseHLCombatWeapon
{
public:

	DECLARE_CLASS(CWeaponImmolator, CBaseHLCombatWeapon);

	DECLARE_SERVERCLASS();

	CWeaponImmolator(void);

	void			Precache(void);
	void			PrimaryAttack(void);
	void			ItemPostFrame(void);
	virtual void	Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator);
	void			DispatchSpray(void);
	virtual bool	Deploy(void);
	virtual void	Drop(const Vector &velocity);
	bool			Holster(CBaseCombatWeapon *pSwitchingTo = NULL);

	int				CapabilitiesGet(void) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	void			ImmolationDamage(const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore);

	void			Update();
	void			UpdateThink();

	void			StartImmolating();
	void			StopImmolating();
	bool			IsImmolating() { return m_flBurnRadius != 0.0; }
	bool			HasAmmo(void);
	void			UseAmmo(int count);
	void			ImmolationTouch(CBaseEntity *pOther);

	DECLARE_ACTTABLE();
	DECLARE_DATADESC();

	int				m_beamIndex;
	int				m_beam2Index;

	float			m_flBurnRadius;
	float			m_flStopAttackTime;
	float			m_flTimeLastUpdatedRadius;

	Vector			m_vecImmolatorTarget;

	const			Vector &GetAbsStartPos(void) const;
	const			Vector &GetAbsEndPos(void) const;
	void			SetStartAttachment(int attachment);

private:

	CHandle<CBeam>		pBeam;
	float				m_flAmmoUseTime;
	EHANDLE				m_hViewModel;

	void	EndAttack(void);

protected:

	void	DrawBeam(const Vector &startPos, const Vector &endPos, float width, bool useMuzzle = false);
};

IMPLEMENT_SERVERCLASS_ST(CWeaponImmolator, DT_WeaponImmolator)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(weapon_immolator, CWeaponImmolator);
PRECACHE_WEAPON_REGISTER(weapon_immolator);

BEGIN_DATADESC(CWeaponImmolator)

DEFINE_FIELD(m_beamIndex, FIELD_INTEGER),
DEFINE_FIELD(m_beam2Index, FIELD_INTEGER),
DEFINE_FIELD(m_flBurnRadius, FIELD_FLOAT),
DEFINE_FIELD(m_flTimeLastUpdatedRadius, FIELD_TIME),
DEFINE_FIELD(m_vecImmolatorTarget, FIELD_VECTOR),
DEFINE_FIELD(m_flAmmoUseTime, FIELD_TIME),

DEFINE_ENTITYFUNC(UpdateThink),
END_DATADESC()
//-----------------------------------------------------------------------------
// Maps base activities to weapons-specific ones so our characters do the right things.
//-----------------------------------------------------------------------------
acttable_t CWeaponImmolator::m_acttable[] =
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SHOTGUN, true }
};
IMPLEMENT_ACTTABLE(CWeaponImmolator);

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponImmolator::CWeaponImmolator(void)
{
	m_fMaxRange1 = 512;
	m_fMinRange1 = 0;
	StopImmolating();
	StopSound("Weapon_Immolator.Stop");
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponImmolator::Precache(void)
{
	m_beamIndex = engine->PrecacheModel("sprites/crystal_beam1.vmt");
	m_beam2Index = engine->PrecacheModel("sprites/bluelaser1.vmt");
	PrecacheScriptSound("Weapon_Immolator.Single");
	PrecacheScriptSound("Weapon_Immolator.Stop");
	PrecacheParticleSystem( "flamethrower" );
	BaseClass::Precache();
}
void CWeaponImmolator::PrimaryAttack(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (pOwner->IsAlive() && (pOwner->m_nButtons & IN_ATTACK))
	{
		StartImmolating();
		return;
	}
	//else if ( pOwner->m_nButtons & (IN_ATTACK2 ) )
	//{
	//	StopImmolating();
	//	return;
	//}
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponImmolator::StartImmolating()
{
	m_flBurnRadius = 0.01;
	m_flTimeLastUpdatedRadius = gpGlobals->curtime;
	SetThink(&CWeaponImmolator::UpdateThink);
	SetNextThink(gpGlobals->curtime);
	EmitSound("Weapon_Immolator.Single");
	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

}
void CWeaponImmolator::StopImmolating()
{
	m_flBurnRadius = 0.0;
	SetThink(NULL);
	m_flNextPrimaryAttack = gpGlobals->curtime + 3.0;
	StopSound("Weapon_Immolator.Single");
	StopParticleEffects(this);
	EmitSound("Weapon_Immolator.Stop");
	SendWeaponAnim(ACT_VM_IDLE);
}
void CWeaponImmolator::UpdateThink(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (pOwner && !pOwner->IsAlive())
	{
		StopImmolating();
		return;
	}
	if (pOwner && !pOwner->m_nButtons & (IN_ATTACK))
	{
		StopImmolating();
		return;
	}
	else
	{
		Update();
		SetNextThink(gpGlobals->curtime + 0.05);
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponImmolator::Update()
{
	float flDuration = gpGlobals->curtime - m_flTimeLastUpdatedRadius;
	if (flDuration != 0.0)
	{
		m_flBurnRadius += RADIUS_GROW_RATE * flDuration;
	}

	m_flBurnRadius = min(m_flBurnRadius, MAX_BURN_RADIUS);

	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	Vector vecSrc;

	Vector vecBeamStart;
	vecBeamStart = pOwner->Weapon_ShootPosition();
	Vector vecBeamDir;
	vecBeamDir = pOwner->GetAutoaimVector(AUTOAIM_2DEGREES);
	Vector vecBeamEnd;
	vecBeamEnd = vecBeamStart + (vecBeamDir * MAX_BURN_RADIUS);
	QAngle	angShootDir;

	if (pOwner)
	{
		GetAttachment(LookupAttachment("muzzle"), vecBeamStart, angShootDir);
	}

	trace_t	tr;
	UTIL_TraceLine(vecBeamStart, vecBeamEnd, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr);

	DrawBeam(tr.startpos, tr.endpos, 0, true);
//	DispatchParticleEffect("flamethrower", PATTACH_POINT_FOLLOW, this, "muzzle");
//	DispatchParticleEffect("flamethrower", PATTACH_POINT_FOLLOW, angShootDir, "muzzle");
	DispatchParticleEffect("flamethrower", vecBeamStart, angShootDir, this);
	DispatchParticleEffect("flamethrower", PATTACH_POINT_FOLLOW, angShootDir, this, "muzzle");
	//if( tr.DidHit() | tr.DidHitNonWorldEntity() )
	//{
	int beams;

	for (beams = 0; beams < 5; beams++)
	{
		Vector vecDest;

		// Random unit vector
		vecDest.x = random->RandomFloat(-1, 1);
		vecDest.y = random->RandomFloat(-1, 1);
		vecDest.z = random->RandomFloat(0, 1);

		// Push out to radius dist.
		vecDest = tr.endpos + vecDest * m_flBurnRadius;

		UTIL_Beam(tr.endpos,
			vecDest,
			m_beam2Index,
			0,		//halo index
			0,		//frame start
			2.0f,	//framerate
			0.15f,	//life
			20,		// width
			1.75,	// endwidth
			0.75,	// fadelength,
			15,		// noise

			40,		// red
			255,	// green
			0,		// blue,

			100, // bright
			100  // speed
			);
		//}
		if (pOwner)
		{
			ImmolationDamage(CTakeDamageInfo(this, this, 2, DMG_BURN), tr.endpos, m_flBurnRadius, CLASS_PLAYER);
		}
		else
		{
			ImmolationDamage(CTakeDamageInfo(this, this, 2, DMG_BURN | DMG_DISSOLVE), tr.endpos, m_flBurnRadius, CLASS_NONE);
		}
		Vector	ofsDir = (tr.endpos - GetAbsOrigin());
		float	offset = VectorNormalize(ofsDir);

		if (offset > 128)
			offset = 128;

		float scale = 0.1f + (0.75f * (1.0f - (offset / 128.0f)));
		float growth = 0.1f + (0.75f * (offset / 128.0f));

//		if (tr.DidHitWorld())
//		{
			FireSystem_StartFire(tr.endpos, scale, growth, 10.0f, (SF_FIRE_START_ON | SF_FIRE_START_FULL | SF_FIRE_SMOKELESS), (CBaseEntity*) this, FIRE_NATURAL);
//		}

	}
	//else
	//{
	// The attack beam struck some kind of entity directly.
	//}
	m_flTimeLastUpdatedRadius = gpGlobals->curtime;

	if (m_flBurnRadius >= MAX_BURN_RADIUS)
	{
		StopImmolating();
	}
	if (gpGlobals->curtime >= m_flAmmoUseTime)
	{
		if (pOwner->m_nButtons & IN_ATTACK2)
		{
			UseAmmo(sk_immolator_consumption_full.GetBool());
		}
		UseAmmo(sk_immolator_consumption_base.GetBool());
		m_flAmmoUseTime = gpGlobals->curtime + 0.2;
	}

	if (!HasAmmo())
	{
		StopImmolating();
	}
}
void CWeaponImmolator::DrawBeam(const Vector &startPos, const Vector &endPos, float width, bool useMuzzle)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (!pOwner)
		return;

	//Check to store off our view model index
	if (m_hViewModel == NULL)
	{
		CBaseViewModel *vm = pOwner->GetViewModel();

		if (vm)
		{
			m_hViewModel.Set(vm);
		}
	}

	CBeam *pBeam = CBeam::BeamCreate(IMMO_BEAM_SPRITE, width);

	if (useMuzzle)
	{
		pBeam->PointEntInit(endPos, m_hViewModel);
		pBeam->SetEndAttachment(1);
		pBeam->SetWidth(width / 4.0f);
		pBeam->SetEndWidth(width);
	}
	else
	{
		pBeam->SetEndAttachment(1);
		pBeam->SetStartPos(startPos);
		pBeam->SetEndPos(endPos);
		pBeam->SetWidth(width);
		pBeam->SetEndWidth(width / 4.0f);
	}

	pBeam->AddSpawnFlags(SF_BEAM_TEMPORARY);
	pBeam->SetBrightness(255);
	pBeam->SetColor(230, 100, 0);
	pBeam->RelinkBeam();
	pBeam->LiveForTime(0.1f);
	pBeam->SetNoise(1.0f);

	for (int i = 0; i < 3; i++)
	{
		pBeam = CBeam::BeamCreate(IMMO_BEAM_SPRITE, (width / 2.0f) + i);

		if (useMuzzle)
		{
			pBeam->PointEntInit(endPos, m_hViewModel);
			pBeam->SetEndAttachment(1);
		}
		else
		{
			pBeam->SetStartPos(startPos);
			pBeam->SetEndPos(endPos);
		}

		pBeam->SetBrightness(random->RandomInt(64, 255));
		pBeam->SetColor(0, 255, 0);
		pBeam->RelinkBeam();
		pBeam->LiveForTime(0.1f);
		pBeam->SetNoise(3.6f * i);
		pBeam->SetEndWidth(0.1f);
	}
}
void CWeaponImmolator::DispatchSpray()
{

}
void CWeaponImmolator::ImmolationDamage(const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore)
{
	CBaseEntity *pEntity = NULL;
	trace_t		tr;
	Vector		vecSpot;
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	Vector vecSrc = vecSrcIn;

	// iterate on all entities in the vicinity.
	for (CEntitySphereQuery sphere(vecSrc, flRadius); pEntity = sphere.GetCurrentEntity(); sphere.NextEntity())
	{
		if (pEntity->m_takedamage != DAMAGE_NO && pEntity != pPlayer)
		{
			// UNDONE: this should check a damage mask, not an ignore
			if (iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore)
			{
				continue;
			}

			pEntity->TakeDamage(info);
		}
	}

	SetTouch(&CWeaponImmolator::ImmolationTouch);
}
void CWeaponImmolator::ImmolationTouch(CBaseEntity *pOther)
{
	CBaseAnimating *pAnim;

	pAnim = dynamic_cast<CBaseAnimating*>(pOther);
	if (pAnim)
	{
		pAnim->Ignite(30.0f);
	}
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponImmolator::Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator)
{
	switch (pEvent->event)
	{
	case IMMO_SPRAY:
	{
					   DispatchSpray();
	}
		break;
	default:
		CBaseCombatWeapon::Operator_HandleAnimEvent(pEvent, pOperator);
		break;
	}
}
void CWeaponImmolator::ItemPostFrame(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	//	if (pOwner->IsAlive() && (pOwner->m_nButtons & IN_ATTACK) && m_iClip1 > 0)
	if (pOwner->IsAlive() && (pOwner->m_nButtons & IN_ATTACK))
	{
		PrimaryAttack();
	}
	BaseClass::ItemPostFrame();
}
bool CWeaponImmolator::Deploy(void)
{
	StopImmolating();
	StopSound("Weapon_Immolator.Stop");

	return BaseClass::Deploy();
}
void CWeaponImmolator::Drop(const Vector &velocity)
{
	if (IsImmolating())
	{
		StopImmolating();
		StopSound("Weapon_Immolator.Stop");
	}
	BaseClass::Drop(velocity);
}
void CWeaponImmolator::EndAttack(void)
{
	StopImmolating();
	StopSound("Weapon_Immolator.Stop");

	if (pBeam)
	{
		UTIL_Remove(pBeam);
		pBeam = NULL;
	}
}
bool CWeaponImmolator::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	EndAttack();

	return BaseClass::Holster(pSwitchingTo);
}
bool CWeaponImmolator::HasAmmo(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
	{
		return false;
	}

	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
		return false;

	return true;
}
void CWeaponImmolator::UseAmmo(int count)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
	{
		return;
	}

	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) >= count)
		pOwner->RemoveAmmo(count, m_iPrimaryAmmoType);
	else
		pOwner->RemoveAmmo(pOwner->GetAmmoCount(m_iPrimaryAmmoType), m_iPrimaryAmmoType);
}

