#include "cbase.h"
#include "shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define ENV_PROJECTEDTEXTURE_STARTON			(1<<0)

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CEnvProjectedTexture : public CPointEntity
{
	DECLARE_CLASS(CEnvProjectedTexture, CPointEntity);
public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CEnvProjectedTexture();
	bool KeyValue(const char *szKeyName, const char *szValue);
	virtual bool GetKeyValue(const char *szKeyName, char *szValue, int iMaxLen);

	// Always transmit to clients
	virtual int UpdateTransmitState();
	virtual void Activate(void);

	void InputTurnOn(inputdata_t &inputdata);
	void InputTurnOff(inputdata_t &inputdata);
	void InputSetFOV(inputdata_t &inputdata);
	void InputSetTarget(inputdata_t &inputdata);
	void InputSetCameraSpace(inputdata_t &inputdata);
	void InputSetLightOnlyTarget(inputdata_t &inputdata);
	void InputSetLightWorld(inputdata_t &inputdata);
	void InputSetEnableShadows(inputdata_t &inputdata);
	void InputSetLightColor(inputdata_t &inputdata);
	void InputSetSpotlightTexture(inputdata_t &inputdata);
	void InputSetAmbient(inputdata_t &inputdata);

	void InitialThink(void);

	CNetworkHandle(CBaseEntity, m_hTargetEntity);

	CNetworkVar(bool, m_bState);
	CNetworkVar(float, m_flLightFOV);
	CNetworkVar(bool, m_bEnableShadows);
	CNetworkVar(bool, m_bLightOnlyTarget);
	CNetworkVar(bool, m_bLightWorld);
	CNetworkVar(bool, m_bCameraSpace);
	CNetworkVector( m_LinearFloatLightColor );
	CNetworkColor32(m_LightColor);
	CNetworkVar(float, m_flAmbient);
	CNetworkString(m_SpotlightTextureName, MAX_PATH);
	CNetworkVar(int, m_nSpotlightTextureFrame);
	CNetworkVar(float, m_flNearZ);
	CNetworkVar(float, m_flFarZ);
	CNetworkVar(float, m_flQuadraticAtten);
	CNetworkVar(float, m_flLinearAtten);
	CNetworkVar(float, m_flConstantAtten);
	CNetworkVar(int, m_nShadowQuality);

private:

};