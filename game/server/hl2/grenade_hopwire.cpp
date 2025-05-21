//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "baseentity.h"
#include "grenade_hopwire.h"
#include "rope.h"
#include "rope_shared.h"
#include "beam_shared.h"
#include "physics.h"
#include "physics_saverestore.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define TETHERHOOK_MODEL	"models/weapons/tripmine/tripmine_clamp.mdl"

//-----------------------------------------------------------------------------
// Tether hook
//-----------------------------------------------------------------------------

class CTetherHook : public CBaseAnimating
{
	DECLARE_CLASS(CTetherHook, CBaseAnimating);
public:
	typedef CBaseAnimating BaseClass;

	bool	CreateVPhysics(void);
	void	Spawn(void);
	virtual void Precache();
	void	SetVelocity(const Vector &velocity, const AngularImpulse &angVelocity);
	void	StartTouch(CBaseEntity *pOther);

	static CTetherHook	*Create(const Vector &origin, const QAngle &angles, CGrenadeHopWire *pOwner);

	void	CreateRope(void);
	void	HookThink(void);
	void	EntityTouch(CBaseEntity *pOther);

	DECLARE_DATADESC();

private:
	CHandle<CGrenadeHopWire>	m_hTetheredOwner;
	IPhysicsSpring				*m_pSpring;
	CRopeKeyframe				*m_pRope;
	CSprite						*m_pGlow;
	CBeam						*m_pBeam;
	bool						m_bAttached;
};

BEGIN_DATADESC(CTetherHook)
DEFINE_FIELD(m_hTetheredOwner, FIELD_EHANDLE),
DEFINE_PHYSPTR(m_pSpring),
DEFINE_FIELD(m_pRope, FIELD_CLASSPTR),
DEFINE_FIELD(m_pGlow, FIELD_CLASSPTR),
DEFINE_FIELD(m_pBeam, FIELD_CLASSPTR),
DEFINE_FIELD(m_bAttached, FIELD_BOOLEAN),
DEFINE_FUNCTION(HookThink),
DEFINE_ENTITYFUNC(EntityTouch),
END_DATADESC()

LINK_ENTITY_TO_CLASS(tetherhook, CTetherHook);

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTetherHook::CreateVPhysics()
{
	// Create the object in the physics system
	IPhysicsObject *pPhysicsObject = VPhysicsInitNormal(SOLID_BBOX, 0, false);

	// Make sure I get touch called for static geometry
	if (pPhysicsObject)
	{
		int flags = pPhysicsObject->GetCallbackFlags();
		flags |= CALLBACK_GLOBAL_TOUCH_STATIC;
		pPhysicsObject->SetCallbackFlags(flags);
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTetherHook::Spawn(void)
{
	m_bAttached = false;

	Precache();
	SetModel(TETHERHOOK_MODEL);
	SetCollisionGroup(COLLISION_GROUP_PROJECTILE);
	UTIL_SetSize(this, vec3_origin, vec3_origin);

	CreateVPhysics();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTetherHook::CreateRope(void)
{
	//Make sure it's not already there
	if (m_pRope == NULL)
	{
		//Create it between ourselves and the owning grenade
		m_pRope = CRopeKeyframe::Create(this, m_hTetheredOwner, 0, 0);

		if (m_pRope != NULL)
		{
			m_pRope->m_Width = 0.75;
			m_pRope->m_RopeLength = 32;
			m_pRope->m_Slack = 64;
			m_pRope->EnableCollision();
			CPASAttenuationFilter filter(this, "TripwireGrenade.ShootRope");
			EmitSound(filter, entindex(), "TripwireGrenade.ShootRope");
		}
	}
}

void CTetherHook::EntityTouch(CBaseEntity *pOther)
{
	Msg("LEL");

}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CTetherHook::StartTouch(CBaseEntity *pOther)
{
	if (m_bAttached == false)
	{
		m_bAttached = true;

		SetVelocity(vec3_origin, vec3_origin);
		SetMoveType(MOVETYPE_NONE);
		VPhysicsDestroyObject();

		EmitSound("TripwireGrenade.Hook");

		StopSound(entindex(), "TripwireGrenade.ShootRope");

		//Make a physics constraint between us and the owner
		if (m_pSpring == NULL)
		{
			springparams_t spring;

			//FIXME: Make these real
			spring.constant = 150.0f;
			spring.damping = 24.0f;
			spring.naturalLength = 32;
			spring.relativeDamping = 0.1f;
			spring.startPosition = GetAbsOrigin();
			spring.endPosition = m_hTetheredOwner->WorldSpaceCenter();
			spring.useLocalPositions = false;

			IPhysicsObject *pEnd = m_hTetheredOwner->VPhysicsGetObject();

			m_pSpring = physenv->CreateSpring(g_PhysWorldObject, pEnd, &spring);
		}

		SetThink(&CTetherHook::HookThink);
		SetNextThink(gpGlobals->curtime + 0.1f);
		//UTIL_Remove(this);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &velocity - 
//			&angVelocity - 
//-----------------------------------------------------------------------------
void CTetherHook::SetVelocity(const Vector &velocity, const AngularImpulse &angVelocity)
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

	if (pPhysicsObject != NULL)
	{
		pPhysicsObject->AddVelocity(&velocity, &angVelocity);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTetherHook::HookThink(void)
{
	if (m_pBeam == NULL)
	{
		m_pBeam = CBeam::BeamCreate("sprites/rollermine_shock.vmt", 1.0f);
		m_pBeam->EntsInit(this, m_hTetheredOwner);

		m_pBeam->SetNoise(0.5f);
		m_pBeam->SetColor(255, 255, 255);
		m_pBeam->SetScrollRate(25);
		m_pBeam->SetBrightness(128);
		m_pBeam->SetWidth(4.0f);
		m_pBeam->SetEndWidth(1.0f);
	}

	if (m_pGlow == NULL)
	{
		m_pGlow = CSprite::SpriteCreate("sprites/blueflare1.vmt", GetLocalOrigin(), false);

		if (m_pGlow != NULL)
		{
			m_pGlow->SetParent(this);
			m_pGlow->SetTransparency(kRenderTransAdd, 255, 255, 255, 255, kRenderFxNone);
			m_pGlow->SetBrightness(128, 0.1f);
			m_pGlow->SetScale(0.5f, 0.1f);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&angles - 
//			*pOwner - 
//-----------------------------------------------------------------------------
CTetherHook	*CTetherHook::Create(const Vector &origin, const QAngle &angles, CGrenadeHopWire *pOwner)
{
	CTetherHook *pHook = CREATE_ENTITY(CTetherHook, "tetherhook");

	if (pHook != NULL)
	{
		pHook->m_hTetheredOwner = pOwner;
		pHook->SetAbsOrigin(origin);
		pHook->SetAbsAngles(angles);
		pHook->SetOwnerEntity((CBaseEntity *)pOwner);
		pHook->Spawn();
	}

	return pHook;
}

//-----------------------------------------------------------------------------

#define GRENADE_MODEL "models/weapons/tripmine/tripmine.mdl"

BEGIN_DATADESC(CGrenadeHopWire)
DEFINE_FIELD(m_nHooksShot, FIELD_INTEGER),
DEFINE_FIELD(m_pGlow, FIELD_CLASSPTR),

DEFINE_THINKFUNC(TetherThink),
DEFINE_THINKFUNC(CombatThink),
END_DATADESC()

LINK_ENTITY_TO_CLASS(npc_grenade_hopwire, CGrenadeHopWire);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGrenadeHopWire::Spawn(void)
{
	Precache();

	SetModel(GRENADE_MODEL);
	SetCollisionGroup(COLLISION_GROUP_PROJECTILE);
	m_nHooksShot = 0;
	m_pGlow = NULL;

	CreateVPhysics();
}

//-----------------------------------------------------------------------------
// Purpose:		
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGrenadeHopWire::CreateVPhysics()
{
	// Create the object in the physics system
	VPhysicsInitNormal(SOLID_BBOX, 0, false);
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGrenadeHopWire::Precache(void)
{
	PrecacheModel(GRENADE_MODEL);
	PrecacheModel(TETHERHOOK_MODEL);
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : timer - 
//-----------------------------------------------------------------------------
void CGrenadeHopWire::SetTimer(float timer)
{
	SetThink(&CGrenadeHopWire::PreDetonate);
	SetNextThink(gpGlobals->curtime + timer);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGrenadeHopWire::CombatThink(void)
{
	if (m_pGlow == NULL)
	{
		m_pGlow = CSprite::SpriteCreate("sprites/blueflare1.vmt", GetLocalOrigin(), false);

		if (m_pGlow != NULL)
		{
			m_pGlow->SetParent(this);
			m_pGlow->SetTransparency(kRenderTransAdd, 255, 255, 255, 255, kRenderFxNone);
			m_pGlow->SetBrightness(128, 0.1f);
			m_pGlow->SetScale(1.0f, 0.1f);
		}
	}

	//TODO: Go boom... or something	
}

void CTetherHook::Precache()
{
	BaseClass::Precache();
	PrecacheScriptSound("TripwireGrenade.ShootRope");
	PrecacheScriptSound("TripwireGrenade.Hook");
	PrecacheModel("sprites/rollermine_shock.vmt");
	PrecacheModel("sprites/blueflare1.vmt");
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGrenadeHopWire::TetherThink(void)
{
	CTetherHook *pHook = NULL;
	static Vector velocity = RandomVector(-1.0f, 1.0f);

	//Create a tether hook
	pHook = (CTetherHook *)CTetherHook::Create(GetLocalOrigin(), GetLocalAngles(), this);

	if (pHook == NULL)
		return;

	pHook->CreateRope();


	if (m_nHooksShot % 2)
	{
		velocity.Negate();
	}
	else
	{
		velocity = RandomVector(-1.0f, 1.0f);
	}

	pHook->SetVelocity(velocity * 1500.0f, vec3_origin);

	m_nHooksShot++;

	if (m_nHooksShot == 8)
	{
		//TODO: Play a startup noise
		trace_t tr;
		UTIL_TraceLine(GetAbsOrigin(), pHook->GetAbsOrigin(), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr);

		// If can't see hook
		CBaseEntity *pEntity = tr.m_pEnt;
		if (tr.fraction != 1.0 && pEntity != pHook)
		{
			// Shoot missiles at place where rope was intersected
			m_iHealth = 0;
			//	BreakRope();
			Vector m_vTargetPos = tr.endpos;
			CrossProduct(velocity, Vector(0, 0, 1));
		//	m_vTargetOffset *= TGRENADE_MISSILE_OFFSET;
		//	SetThink(&CTripwireGrenade::FireThink);
		//	FireThink();
			Msg("YEY");
		}

//		SetThink(&CGrenadeHopWire::CombatThink);
//		SetNextThink(gpGlobals->curtime + 0.1f);
	}
	else
	{
		SetThink(&CGrenadeHopWire::TetherThink);
		SetNextThink(gpGlobals->curtime + random->RandomFloat(0.1f, 0.3f));
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &velocity - 
//			&angVelocity - 
//-----------------------------------------------------------------------------
void CGrenadeHopWire::SetVelocity(const Vector &velocity, const AngularImpulse &angVelocity)
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

	if (pPhysicsObject != NULL)
	{
		pPhysicsObject->AddVelocity(&velocity, &angVelocity);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGrenadeHopWire::Detonate(void)
{

	Vector	hopVel = RandomVector(-8, 8);
	hopVel[2] += 800.0f;

	AngularImpulse	hopAngle = RandomAngularImpulse(-300, 300);

	//FIXME: We should find out how tall the ceiling is and always try to hop halfway

	//Add upwards velocity for the "hop"
	SetVelocity(hopVel, hopAngle);

	//Shoot 4-8 cords out to grasp the surroundings
	SetThink(&CGrenadeHopWire::TetherThink);
	SetNextThink(gpGlobals->curtime + 0.6f);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseGrenade *HopWire_Create(const Vector &position, const QAngle &angles, const Vector &velocity, const AngularImpulse &angVelocity, CBaseEntity *pOwner, float timer)
{
	CGrenadeHopWire *pGrenade = (CGrenadeHopWire *)CBaseEntity::Create("npc_grenade_hopwire", position, angles, pOwner);

	pGrenade->SetTimer(timer);
	pGrenade->SetVelocity(velocity, angVelocity);
	pGrenade->SetThrower(ToBaseCombatCharacter(pOwner));

	return pGrenade;
}
