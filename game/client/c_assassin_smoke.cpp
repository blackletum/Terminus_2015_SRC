//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "baseparticleentity.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_AssassinSmoke : public C_BaseParticleEntity
{
public:
	DECLARE_CLASS(C_AssassinSmoke, C_BaseParticleEntity);
	DECLARE_CLIENTCLASS();

	C_AssassinSmoke();
	virtual			~C_AssassinSmoke();

private:
	C_AssassinSmoke(const C_AssassinSmoke &); // not defined, not accessible
};

IMPLEMENT_CLIENTCLASS_DT(C_AssassinSmoke, DT_AssassinSmoke, CAssassinSmoke)
END_RECV_TABLE()

C_AssassinSmoke::C_AssassinSmoke()
{
}


C_AssassinSmoke::~C_AssassinSmoke()
{
}


