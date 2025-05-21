//========= Copyright Insolence Corporation, All rights reserved. ============//
//
// Purpose: For some reasson, when defining more than default 
// ammo in hl2_gamerules.cpp zombies suddenly get ignited when fired with 
// any weapon
//
//=============================================================================
#include "cbase.h"
#include "ammodef.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//CAmmoDef* GetInsAmmoDef()
//{
//	static CAmmoDef ammo;
//
//	static bool bInitialising = true;
//	if (bInitialising)
//	{
//		// Call ammo.AddAmmoType(...) here
//		bInitialising = false;
//	}
//
//	return &ammo;
//}
