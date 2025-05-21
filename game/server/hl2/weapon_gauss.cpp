#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "basehlcombatweapon.h"
#include "decals.h"
#include "beam_shared.h"
#include "AmmoDef.h"
#include "IEffects.h"
#include "engine/IEngineSound.h"
#include "in_buttons.h"
#include "soundenvelope.h"
#include "soundent.h"
#include "shake.h"
#include "explode.h"
#include "weapon_gauss.h"
#include "physics_prop_ragdoll.h"
#include "RagdollBoogie.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Declarations
//-----------------------------------------------------------------------------

IMPLEMENT_SERVERCLASS_ST(CWeaponGauss, DT_WeaponGauss)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(weapon_gauss, CWeaponGauss);
PRECACHE_WEAPON_REGISTER(weapon_gauss);

acttable_t	CWeaponGauss::m_acttable[] =
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_AR2, true },
};

IMPLEMENT_ACTTABLE(CWeaponGauss);

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC(CWeaponGauss)

DEFINE_FIELD(m_hViewModel, FIELD_EHANDLE),
DEFINE_FIELD(m_flNextChargeTime, FIELD_TIME),
DEFINE_FIELD(m_flChargeStartTime, FIELD_TIME),
DEFINE_FIELD(m_bCharging, FIELD_BOOLEAN),
DEFINE_FIELD(m_bChargeIndicated, FIELD_BOOLEAN),

DEFINE_SOUNDPATCH(m_sndCharge),

END_DATADESC()


extern ConVar sk_plr_dmg_gauss;
extern ConVar sk_plr_max_dmg_gauss;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWeaponGauss::CWeaponGauss(void)
{
	m_hViewModel = NULL;
	m_flNextChargeTime = 0;
	m_flChargeStartTime = 0;
	m_sndCharge = NULL;
	m_bCharging = false;
	m_bChargeIndicated = false;
	m_bReloadsSingly = false;
	m_bFiresUnderwater = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGauss::Precache(void)
{
	enginesound->PrecacheSound("weapons/gauss/chargeloop.wav");
	PrecacheModel(GAUSS_RAGDOLL_BOOGIE_SPRITE);
	BaseClass::Precache();
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGauss::Spawn(void)
{
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGauss::Fire(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (!pOwner){ return; }

	m_bCharging = false;

	if (m_hViewModel == NULL)
	{
		CBaseViewModel *vm = pOwner->GetViewModel();

		if (vm)
		{
			m_hViewModel.Set(vm);
		}
	}

	Vector	startPos = pOwner->Weapon_ShootPosition();
	Vector	aimDir = pOwner->GetAutoaimVector(AUTOAIM_5DEGREES);

	Vector vecUp, vecRight;
	VectorVectors(aimDir, vecRight, vecUp);

	float x, y, z;

	// cone
	do {
		x = random->RandomFloat(-0.5, 0.5) + random->RandomFloat(-0.5, 0.5);
		y = random->RandomFloat(-0.5, 0.5) + random->RandomFloat(-0.5, 0.5);
		z = x*x + y*y;
	} while (z > 1);

	aimDir = aimDir + x * GetBulletSpread().x * vecRight + y * GetBulletSpread().y * vecUp;

	Vector	endPos = startPos + (aimDir * MAX_TRACE_LENGTH);

	// dist
	trace_t	tr;
	UTIL_TraceLine(startPos, endPos, MASK_SHOT, pOwner, COLLISION_GROUP_INTERACTIVE, &tr);

	ClearMultiDamage();

	CBaseEntity *pHit = tr.m_pEnt;

	CTakeDamageInfo dmgInfo(this, pOwner, sk_plr_dmg_gauss.GetFloat(), DMG_SHOCK);

	if (pHit != NULL)
	{
		CalculateBulletDamageForce(&dmgInfo, m_iPrimaryAmmoType, aimDir, tr.endpos);
		pHit->DispatchTraceAttack(dmgInfo, aimDir, &tr);
	}

	ShouldDrawWaterImpacts(tr);

	if (tr.DidHitWorld())
	{
		float hitAngle = -DotProduct(tr.plane.normal, aimDir);

		if (hitAngle < 0.5f)
		{
			Vector vReflection;

			vReflection = 2.0 * tr.plane.normal * hitAngle + aimDir;

			startPos = tr.endpos;
			endPos = startPos + (vReflection * MAX_TRACE_LENGTH);

			//Draw beam to reflection point
			DrawBeam(tr.startpos, tr.endpos, 1.6, true);

			CPVSFilter filter(tr.endpos);
			te->GaussExplosion(filter, 0.0f, tr.endpos, tr.plane.normal, 0);

			UTIL_ImpactTrace(&tr, GetAmmoDef()->DamageType(m_iPrimaryAmmoType), "ImpactGauss");

			//Find new reflection end position
			UTIL_TraceLine(startPos, endPos, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr);

			if (tr.m_pEnt != NULL)
			{
				dmgInfo.SetDamageForce(GetAmmoDef()->DamageForce(m_iPrimaryAmmoType) * vReflection);
				dmgInfo.SetDamagePosition(tr.endpos);
				tr.m_pEnt->DispatchTraceAttack(dmgInfo, vReflection, &tr);
			}

			//Connect reflection point to end
			DrawBeam(tr.startpos, tr.endpos, 0.4);
		}
		else
		{
			DrawBeam(tr.startpos, tr.endpos, 1.6, true);
		}
	}
	else
	{
		DrawBeam(tr.startpos, tr.endpos, 1.6, true);
	}

	ApplyMultiDamage();

	UTIL_ImpactTrace(&tr, GetAmmoDef()->DamageType(m_iPrimaryAmmoType), "ImpactGauss");
	UTIL_DecalTrace(&tr, "RedGlowFade");

	CPVSFilter filter(tr.endpos);
	te->GaussExplosion(filter, 0.0f, tr.endpos, tr.plane.normal, 0);

	m_flNextSecondaryAttack = gpGlobals->curtime + 1.5f;

	AddViewKick();


	pOwner->SetMuzzleFlashTime(gpGlobals->curtime + 0.5);
}

//-----------------------------------------------------------------------------
// charge
//-----------------------------------------------------------------------------
void CWeaponGauss::ChargedFire(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (!pOwner){ return; }

	bool penetrated = false;

	WeaponSound(SINGLE);
	WeaponSound(SPECIAL2);

	SendWeaponAnim(ACT_VM_SECONDARYATTACK);
	StopChargeSound();

	m_bCharging = false;
	m_bChargeIndicated = false;

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.2f;
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.5f;

	Vector	startPos = pOwner->Weapon_ShootPosition();
	Vector	aimDir = pOwner->GetAutoaimVector(AUTOAIM_5DEGREES);
	Vector	endPos = startPos + (aimDir * MAX_TRACE_LENGTH);

	trace_t	tr;
	UTIL_TraceLine(startPos, endPos, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr);
	ShouldDrawWaterImpacts(tr);
	ClearMultiDamage();


	float flChargeAmount = (gpGlobals->curtime - m_flChargeStartTime) / MAX_GAUSS_CHARGE_TIME;


	if (flChargeAmount > 1.0f){ flChargeAmount = 1.0f; }


	float flDamage = sk_plr_dmg_gauss.GetFloat() + ((sk_plr_max_dmg_gauss.GetFloat()/* - sk_plr_dmg_gauss.GetFloat()*/) * flChargeAmount);

	CBaseEntity *pHit = tr.m_pEnt;

	if (tr.DidHitWorld())
	{
		// wall
		UTIL_ImpactTrace(&tr, GetAmmoDef()->DamageType(m_iPrimaryAmmoType), "ImpactGauss");
		UTIL_DecalTrace(&tr, "RedGlowFade");

		CPVSFilter filter(tr.endpos);
		te->GaussExplosion(filter, 0.0f, tr.endpos, tr.plane.normal, 0);
		Vector  vStore = tr.endpos;
		Vector	testPos = tr.endpos + (aimDir * 48.0f);

		UTIL_TraceLine(testPos, tr.endpos, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr);

		if (!tr.allsolid)
		{
			UTIL_DecalTrace(&tr, "RedGlowFade");
			penetrated = true;

			trace_t backward_tr;
			UTIL_TraceLine(tr.endpos, vStore, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &backward_tr);
			if (backward_tr.DidHit()){
				UTIL_ImpactTrace(&backward_tr, GetAmmoDef()->DamageType(m_iPrimaryAmmoType), "ImpactGauss");
			}

		}
	}
	else if (pHit != NULL)
	{
		CTakeDamageInfo dmgInfo(this, pOwner, flDamage, DMG_SHOCK);
		CalculateBulletDamageForce(&dmgInfo, m_iPrimaryAmmoType, aimDir, tr.endpos);

		pHit->DispatchTraceAttack(dmgInfo, aimDir, &tr);
	}

	ApplyMultiDamage();

	UTIL_ImpactTrace(&tr, GetAmmoDef()->DamageType(m_iPrimaryAmmoType), "ImpactGauss");

	QAngle	viewPunch;

	viewPunch.x = random->RandomFloat(-4.0f, -8.0f);
	viewPunch.y = random->RandomFloat(-0.25f, 0.25f);
	viewPunch.z = 0;

	pOwner->ViewPunch(viewPunch);

	DrawBeam(startPos, tr.endpos, (1 + (flChargeAmount * 4.6)), true);

	Vector	recoilForce = pOwner->GetAbsVelocity() - pOwner->GetAutoaimVector(0) * (flDamage * 5.0f);
	recoilForce[2] += 12.80f;
	pOwner->SetAbsVelocity(recoilForce);

	CPVSFilter filter(tr.endpos);
	te->GaussExplosion(filter, 0.0f, tr.endpos, tr.plane.normal, 0);
	if (penetrated)
	{

		RadiusDamage(CTakeDamageInfo(this, this, flDamage, DMG_SHOCK), tr.endpos, 200.0f, CLASS_NONE, NULL);


		UTIL_TraceLine(tr.endpos, endPos, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr);

		UTIL_ImpactTrace(&tr, GetAmmoDef()->DamageType(m_iPrimaryAmmoType), "ImpactGauss");
		UTIL_DecalTrace(&tr, "RedGlowFade");

		RadiusDamage(CTakeDamageInfo(this, this, flDamage, DMG_SHOCK), tr.endpos, 200.0f, CLASS_NONE, NULL);
	}

	pOwner->DoMuzzleFlash();

	pOwner->SetMuzzleFlashTime(gpGlobals->curtime + 0.5);
}

//-----------------------------------------------------------------------------
// beam
//-----------------------------------------------------------------------------
void CWeaponGauss::DrawBeam(const Vector &startPos, const Vector &endPos, float width, bool useMuzzle)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (pOwner == NULL)
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


	CBeam *pBeam = CBeam::BeamCreate(PHYSGUN_BEAM_SPRITE, width);

	if (useMuzzle)
	{
		pBeam->PointEntInit(endPos, m_hViewModel);
		pBeam->SetEndAttachment(1);
		pBeam->SetWidth(width / 4.0f);
		pBeam->SetEndWidth(width);
	}
	else
	{
		pBeam->SetStartPos(startPos);
		pBeam->SetEndPos(endPos);
		pBeam->SetWidth(width);
		pBeam->SetEndWidth(width / 4.0f);
	}

	pBeam->SetBrightness(255);
	pBeam->SetColor(230, 230 + random->RandomInt(-16, 16), 250);
	pBeam->RelinkBeam();
	pBeam->LiveForTime(0.1f);
	pBeam->SetNoise(6.6f);


	for (int i = 0; i < 3; i++)
	{
		pBeam = CBeam::BeamCreate(PHYSGUN_BEAM_SPRITE, (width / 2.0f) + i);

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
//		pBeam->SetColor(40, 40, 150 + random->RandomInt(0, 64));
		pBeam->SetColor(255, 255, 150 + random->RandomInt(0, 64));
		pBeam->RelinkBeam();
		pBeam->LiveForTime(0.1f);
		pBeam->SetNoise(3.6f * i);
		pBeam->SetEndWidth(0.1f);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGauss::PrimaryAttack(void)
{
	//if( weapon_fire_mode.GetBool() == 0 )
	//{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (!pOwner){ return; }

	WeaponSound(SINGLE);
	WeaponSound(SPECIAL2);

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

	pOwner->DoMuzzleFlash();

	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();

	pOwner->RemoveAmmo(1, m_iPrimaryAmmoType);

	Fire();
	//}
	//else
	//{
	//	ChargeAttack();
	//}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGauss::IncreaseCharge(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (m_flNextChargeTime > gpGlobals->curtime || !pOwner){ return; }


	if ((gpGlobals->curtime - m_flChargeStartTime) > MAX_GAUSS_CHARGE_TIME)
	{

		if (m_bChargeIndicated == false)
		{
			WeaponSound(SPECIAL2);
			m_bChargeIndicated = true;
		}

		if ((gpGlobals->curtime - m_flChargeStartTime) > DANGER_GAUSS_CHARGE_TIME)
		{

			WeaponSound(SPECIAL2);


			pOwner->TakeDamage(CTakeDamageInfo(this, this, 25, DMG_SHOCK | DMG_CRUSH));

			color32 gaussDamage = { 255, 128, 0, 128 };
			UTIL_ScreenFade(pOwner, gaussDamage, 0.2f, 0.2f, FFADE_IN);

			m_flNextChargeTime = gpGlobals->curtime + random->RandomFloat(0.5f, 2.5f);
		}

		return;
	}


	pOwner->RemoveAmmo(1, m_iPrimaryAmmoType);

	int pitch = (gpGlobals->curtime - m_flChargeStartTime) * (150 / GetFullChargeTime()) + 100;
	if (pitch > 250){ pitch = 250; }
	if (m_sndCharge != NULL)
	{
		(CSoundEnvelopeController::GetController()).SoundChangePitch(m_sndCharge, pitch, 0);
	}


	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		ChargedFire();
		return;
	}

	m_flNextChargeTime = gpGlobals->curtime + GAUSS_CHARGE_TIME;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGauss::SecondaryAttack(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (!pOwner || pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0){ return; }

	if (pOwner->GetWaterLevel() == 3)
	{
		EmitSound("Weapon_Gauss.Zap1");
		SendWeaponAnim(ACT_VM_IDLE);
		m_flNextSecondaryAttack = gpGlobals->curtime + 3.0;

		pOwner->TakeDamage(CTakeDamageInfo(this, this, 25, DMG_SHOCK | DMG_CRUSH));
		return;
	}
	if (!m_bCharging)
	{

		SendWeaponAnim(ACT_VM_PULLBACK);


		if (!m_sndCharge)
		{
			CPASAttenuationFilter filter(this);
			m_sndCharge = (CSoundEnvelopeController::GetController()).SoundCreate(filter, entindex(), CHAN_STATIC, "weapons/gauss/chargeloop.wav", ATTN_NORM);
		}

		if (m_sndCharge != NULL)
		{
			(CSoundEnvelopeController::GetController()).Play(m_sndCharge, 1.0f, 50);
			(CSoundEnvelopeController::GetController()).SoundChangePitch(m_sndCharge, 250, 3.0f);
		}

		m_flChargeStartTime = gpGlobals->curtime;
		m_bCharging = true;
		m_bChargeIndicated = false;


		pOwner->RemoveAmmo(1, m_iPrimaryAmmoType);
	}

	IncreaseCharge();
}

//-----------------------------------------------------------------------------
// Purpose:  view punch
//-----------------------------------------------------------------------------
void CWeaponGauss::AddViewKick(void)
{
	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer == NULL)
		return;

	QAngle	viewPunch;

	viewPunch.x = random->RandomFloat(-0.5f, -0.2f);
	viewPunch.y = random->RandomFloat(-0.5f, 0.5f);
	viewPunch.z = 0;

	pPlayer->ViewPunch(viewPunch);
}

//-----------------------------------------------------------------------------
// Purpose: frames
//-----------------------------------------------------------------------------
void CWeaponGauss::ItemPostFrame(void)
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (!pPlayer){ return; }

	if (pPlayer->m_afButtonReleased & IN_ATTACK2)
	{
		if (m_bCharging){ ChargedFire(); }
	}

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose: charge sound
//-----------------------------------------------------------------------------
void CWeaponGauss::StopChargeSound(void)
{
	if (m_sndCharge != NULL)
	{
		(CSoundEnvelopeController::GetController()).SoundFadeOut(m_sndCharge, 0.1f);
	}
}

//-----------------------------------------------------------------------------
// Purpose: holster
// Input  : *pSwitchingTo - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponGauss::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	StopChargeSound(); // 
	m_bCharging = false;
	m_bChargeIndicated = false;

	return BaseClass::Holster(pSwitchingTo);
}
//-----------------------------------------------
// Purpose: charge time
//-----------------------------------------------
float CWeaponGauss::GetFullChargeTime(void)
{
	if (g_pGameRules->IsMultiplayer())
	{
		return 1.5;
	}
	else
	{
		return 4;
	}
}
//----------------------------------------------------------------------------------
// Purpose: splash
//----------------------------------------------------------------------------------
#define FSetBit(iBitVector, bits)	((iBitVector) |= (bits)) // LOLMEN : Set some bits to bit vec
#define FBitSet(iBitVector, bit)	((iBitVector) & (bit))	// LOLMEN : Do that bit setted in bit vec?
#define TraceContents( vec ) ( enginetrace->GetPointContents( vec ) ) // LOLMEN : Do some test?
#define WaterContents( vec ) ( FBitSet( TraceContents( vec ), CONTENTS_WATER|CONTENTS_SLIME ) ) // Lolmen : For water

bool CWeaponGauss::ShouldDrawWaterImpacts(const trace_t &shot_trace)
{
	//FIXME: This doesn't handle the case of trying to splash while being underwater, but that's not going to look good
	//		 right now anyway...

	// We must start outside the water
	if (WaterContents(shot_trace.startpos))
		return false;

	// We must end inside of water
	if (!WaterContents(shot_trace.endpos))
		return false;

	trace_t	waterTrace;

	UTIL_TraceLine(shot_trace.startpos, shot_trace.endpos, (CONTENTS_WATER | CONTENTS_SLIME), UTIL_GetLocalPlayer(), COLLISION_GROUP_NONE, &waterTrace);


	if (waterTrace.fraction < 1.0f)
	{
		CEffectData	data;

		data.m_fFlags = 0;
		data.m_vOrigin = waterTrace.endpos;
		data.m_vNormal = waterTrace.plane.normal;
		data.m_flScale = random->RandomFloat(2.0, 4.0f); // Lolmen : Регулируйте размер кругов/брызг тут

		// See if we hit slime
		if (FBitSet(waterTrace.contents, CONTENTS_SLIME))
		{
			FSetBit(data.m_fFlags, FX_WATER_IN_SLIME);
		}

		CPASFilter filter(data.m_vOrigin);
		te->DispatchEffect(filter, 0.0, data.m_vOrigin, "watersplash", data);
	}
	return true;
}
