//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//=============================================================================

#ifndef	GRENADEBRICKBAT_H
#define	GRENADEBRICKBAT_H

#include "basegrenade_shared.h"

enum BrickbatAmmo_t;

class CGrenade_Brickbat : public CBaseGrenade
{
public:
	DECLARE_CLASS(CGrenade_Brickbat, CBaseGrenade);

	virtual void	Spawn(void);
	virtual void	SpawnBrickbatWeapon(void);
	virtual void	Detonate(void) { return; };
	virtual bool	CreateVPhysics();
	void			BrickbatTouch(CBaseEntity *pOther);
	void			BrickbatUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void			BrickbatThink(void);

	BrickbatAmmo_t	m_nType;
	bool			m_bExplodes;
	bool			m_bBounceToFlat;	// Bouncing to flatten
public:
	DECLARE_DATADESC();
};
#endif	//GRENADEBRICKBAT_H
