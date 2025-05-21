#include "cbase.h"
#include "entityinput.h"
#include "entityoutput.h"
#include "eventqueue.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const int SF_REMOVE_ON_FIRE	= 0x001;
const int SF_ALLOW_FAST_RETRIGGER = 0x002;

class CLogicRLDiff : public CLogicalEntity
{
public:
	DECLARE_CLASS( CLogicRLDiff, CLogicalEntity );

	CLogicRLDiff();

	void Activate();

	// Input handlers
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );
	void InputTrigger( inputdata_t &inputdata );
	void InputCancelPending( inputdata_t &inputdata );

	DECLARE_DATADESC();

	// Outputs
	COutputEvent m_OnTriggerEasy;
	COutputEvent m_OnTriggerNormal;	
	COutputEvent m_OnTriggerHard;
	COutputEvent m_OnTriggerDefault;
	
private:

	bool m_bDisabled;
};

LINK_ENTITY_TO_CLASS( logic_relay_difficulty, CLogicRLDiff );

BEGIN_DATADESC( CLogicRLDiff )

	DEFINE_KEYFIELD(m_bDisabled, FIELD_BOOLEAN, "StartDisabled"),

	// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "Enable", InputEnable),
	DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),
	DEFINE_INPUTFUNC(FIELD_VOID, "Toggle", InputToggle),
	DEFINE_INPUTFUNC(FIELD_VOID, "Trigger", InputTrigger),
	DEFINE_INPUTFUNC(FIELD_VOID, "CancelPending", InputCancelPending),

	// Outputs
	DEFINE_OUTPUT(m_OnTriggerEasy, "OnEasy"),
	DEFINE_OUTPUT(m_OnTriggerNormal, "OnNormal"),
	DEFINE_OUTPUT(m_OnTriggerHard, "OnHard"),
	DEFINE_OUTPUT(m_OnTriggerDefault, "OnDefault"),

END_DATADESC()

//-----------------------------------------------------------------------------
CLogicRLDiff::CLogicRLDiff(void)
{
}
//------------------------------------------------------------------------------
void CLogicRLDiff::Activate()
{
	BaseClass::Activate();
}
//------------------------------------------------------------------------------
void CLogicRLDiff::InputEnable( inputdata_t &inputdata )
{
	m_bDisabled = false;
}
//------------------------------------------------------------------------------
void CLogicRLDiff::InputCancelPending( inputdata_t &inputdata )
{ 
	g_EventQueue.CancelEvents( this );
}
//------------------------------------------------------------------------------
void CLogicRLDiff::InputDisable( inputdata_t &inputdata )
{ 
	m_bDisabled = true;
}
//------------------------------------------------------------------------------
void CLogicRLDiff::InputToggle( inputdata_t &inputdata )
{ 
	m_bDisabled = !m_bDisabled;
}
//-----------------------------------------------------------------------------
// Purpose: Input handler that triggers the relay.
//-----------------------------------------------------------------------------
void CLogicRLDiff::InputTrigger( inputdata_t &inputdata )
{
	if ((!m_bDisabled))
	{
		if( g_pGameRules->IsSkillLevel(SKILL_EASY) )
		{
			m_OnTriggerEasy.FireOutput( inputdata.pActivator, this );
		}
		else if( g_pGameRules->IsSkillLevel( SKILL_HARD ) )
		{
			m_OnTriggerHard.FireOutput( inputdata.pActivator, this );
		}
		else
		{
			m_OnTriggerNormal.FireOutput( inputdata.pActivator, this );
		}
		
		if (m_spawnflags & SF_REMOVE_ON_FIRE)
		{
			UTIL_Remove(this);
		}
	}
}