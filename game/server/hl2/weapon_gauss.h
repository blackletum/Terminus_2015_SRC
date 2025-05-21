#include "basehlcombatweapon.h"

#ifndef WEAPON_GAUSS_H
#define WEAPON_GAUSS_H
#ifdef _WIN32
#pragma once
#endif

#include "te_particlesystem.h"
#include "effect_dispatch_data.h"
#include "dlight.h"

#include "weapon_physcannon.h"

#define GAUSS_BEAM_SPRITE		"sprites/laserbeam.vmt"
#define PHYSGUN_BEAM_SPRITE		"sprites/laserbeam.vmt"

#define	GAUSS_CHARGE_TIME		0.2f
#define	MAX_GAUSS_CHARGE		16
#define	MAX_GAUSS_CHARGE_TIME		3
#define	DANGER_GAUSS_CHARGE_TIME	10

#define GAUSS_RAGDOLL_BOOGIE_SPRITE "sprites/lgtning_noz.vmt"


//=============================================================================
// Tau cannon
//=============================================================================

class CWeaponGauss : public CBaseHLCombatWeapon
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS(CWeaponGauss, CBaseHLCombatWeapon);

	CWeaponGauss(void);

	DECLARE_SERVERCLASS();

	void	Spawn(void);
	void	Precache(void);
	void	PrimaryAttack(void);
	void	SecondaryAttack(void);
	void	AddViewKick(void);

	bool	Holster(CBaseCombatWeapon *pSwitchingTo = NULL);

	void	ItemPostFrame(void);

	float	GetFireRate(void) { return 0.2f; }

	virtual const Vector &GetBulletSpread(void)
	{
		static Vector cone = VECTOR_CONE_1DEGREES;
		return cone;
	}

protected:

	void	Fire(void);
	void	ChargedFire(void);
	float	GetFullChargeTime(void);
	void	StartFire(void);
	void	StopChargeSound(void);

	void	DrawBeam(const Vector &startPos, const Vector &endPos, float width, bool useMuzzle = false);
	void	IncreaseCharge(void);
	bool	ShouldDrawWaterImpacts(const trace_t &shot_trace);

private:
	EHANDLE			m_hViewModel;
	float			m_flNextChargeTime;
	float			m_flNextSecondaryAttack;
	CSoundPatch		*m_sndCharge;

	float			m_flChargeStartTime;
	bool			m_bCharging;
	bool			m_bChargeIndicated;

	DECLARE_ACTTABLE();
};

#endif // WEAPON_GAUSS_H
