//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_ai_basenpc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_Leader : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS( C_Leader, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

					C_Leader();
	virtual			~C_Leader();

private:
	C_Leader( const C_Leader & ); // not defined, not accessible
};

IMPLEMENT_CLIENTCLASS_DT(C_Leader, DT_NPC_Leader, CNPC_Leader)
END_RECV_TABLE()

C_Leader::C_Leader()
{
}


C_Leader::~C_Leader()
{
}


