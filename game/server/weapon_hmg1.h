#ifndef WEAPON_GR9_H
#define WEAPON_GR9_H
#ifdef _WIN32
#pragma once
#endif

#define	GR9_BURST_EVENT				( 10 )

class CWeaponHMG1 : public CHLMachineGun
{

private:

	float m_flNextBurstFireTime;
	bool m_bIronsight;

protected:

	bool	m_bZoomed;

public:
	DECLARE_CLASS(CWeaponHMG1, CHLMachineGun);

	DECLARE_SERVERCLASS();

	CWeaponHMG1();

	void	Precache(void);
	bool	Deploy(void);
	void	AddViewKick(void);

	virtual void	Drop(const Vector &velocity);
	bool	Holster(CBaseCombatWeapon *pSwitchingTo = NULL);
	bool	Reload(void);

	void	PrimaryAttack(void);

	void	Zoom(void);
	float	GetFireRate(void);

	void	ItemPostFrame(void);
	void	Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator);

	void	FireNPCPrimaryAttack(CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir);
	void	FireNPCSecondaryAttack(CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir);
	void	Operator_ForceNPCFire(CBaseCombatCharacter  *pOperator, bool bSecondary);

	int CapabilitiesGet(void) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	virtual const Vector& GetBulletSpread(void)
	{
		static Vector cone = VECTOR_CONE_10DEGREES;
		return cone;
	}
	Activity GetPrimaryAttackActivity(void);

	DECLARE_DATADESC();
	DECLARE_ACTTABLE();
};

#endif