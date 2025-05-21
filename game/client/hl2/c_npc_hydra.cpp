//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "bone_setup.h"
#include "c_ai_basenpc.h"
#include "engine/IVDebugOverlay.h"
#include "tier0/vprof.h"
#include "soundinfo.h"
#include "dlight.h"
#include "iefx.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

class C_NPC_Hydra : public C_AI_BaseNPC
{
	DECLARE_CLASS(C_NPC_Hydra, C_AI_BaseNPC);
public:

	DECLARE_CLIENTCLASS();
	DECLARE_INTERPOLATION();

	C_NPC_Hydra();
	virtual			~C_NPC_Hydra();

	virtual	void	OnLatchInterpolatedVariables(int flags);
	virtual bool	SetupBones(matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime);
	virtual	void	StandardBlendingRules(CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask);

	void			CalcBoneChain(Vector pos[], const Vector chain[]);
	void			CalcBoneAngles(const Vector pos[], Quaternion q[]);

	virtual bool	GetSoundSpatialization(SpatializationInfo_t& info);

	virtual void	ResetLatched();

	virtual void ClientThink();


#define	CHAIN_LINKS 32

	bool			m_bNewChain;
	int				m_fLatchFlags;
	Vector			m_vecChain[CHAIN_LINKS];

	Vector			m_vecHeadDir;

	CInterpolatedVar<Vector>	m_iv_vecHeadDir;

	float			m_flRelaxedLength;

	Vector			*m_vecPos;
	CInterpolatedVar<Vector>	*m_iv_vecPos;

	int				m_numHydraBones;
	float			*m_boneLength;

	float			m_maxPossibleLength;

private:

	C_NPC_Hydra(const C_NPC_Hydra &);

	dlight_t *m_dlight;

	int		m_iAttachment;
};

IMPLEMENT_CLIENTCLASS_DT(C_NPC_Hydra, DT_NPC_Hydra, CNPC_Hydra)

RecvPropVector(RECVINFO(m_vecChain[0])),
RecvPropVector(RECVINFO(m_vecChain[1])),
RecvPropVector(RECVINFO(m_vecChain[2])),
RecvPropVector(RECVINFO(m_vecChain[3])),
RecvPropVector(RECVINFO(m_vecChain[4])),
RecvPropVector(RECVINFO(m_vecChain[5])),
RecvPropVector(RECVINFO(m_vecChain[6])),
RecvPropVector(RECVINFO(m_vecChain[7])),
RecvPropVector(RECVINFO(m_vecChain[8])),
RecvPropVector(RECVINFO(m_vecChain[9])),
RecvPropVector(RECVINFO(m_vecChain[10])),
RecvPropVector(RECVINFO(m_vecChain[11])),
RecvPropVector(RECVINFO(m_vecChain[12])),
RecvPropVector(RECVINFO(m_vecChain[13])),
RecvPropVector(RECVINFO(m_vecChain[14])),
RecvPropVector(RECVINFO(m_vecChain[15])),
RecvPropVector(RECVINFO(m_vecChain[16])),
RecvPropVector(RECVINFO(m_vecChain[17])),
RecvPropVector(RECVINFO(m_vecChain[18])),
RecvPropVector(RECVINFO(m_vecChain[19])),
RecvPropVector(RECVINFO(m_vecChain[20])),
RecvPropVector(RECVINFO(m_vecChain[21])),
RecvPropVector(RECVINFO(m_vecChain[22])),
RecvPropVector(RECVINFO(m_vecChain[23])),
RecvPropVector(RECVINFO(m_vecChain[24])),
RecvPropVector(RECVINFO(m_vecChain[25])),
RecvPropVector(RECVINFO(m_vecChain[26])),
RecvPropVector(RECVINFO(m_vecChain[27])),
RecvPropVector(RECVINFO(m_vecChain[28])),
RecvPropVector(RECVINFO(m_vecChain[29])),
RecvPropVector(RECVINFO(m_vecChain[30])),
RecvPropVector(RECVINFO(m_vecChain[31])),
RecvPropVector(RECVINFO(m_vecHeadDir)),
RecvPropFloat(RECVINFO(m_flRelaxedLength)),

END_RECV_TABLE()

C_NPC_Hydra::C_NPC_Hydra() : m_iv_vecHeadDir("C_NPC_Hydra::m_iv_vecHeadDir")
{
	AddVar(&m_vecHeadDir, &m_iv_vecHeadDir, LATCH_ANIMATION_VAR);

	m_numHydraBones = 0;
	m_boneLength = NULL;
	m_maxPossibleLength = 1;
	m_vecPos = NULL;
	m_iv_vecPos = NULL;
}

C_NPC_Hydra::~C_NPC_Hydra()
{
	delete m_boneLength;
	delete m_vecPos;
	delete[] m_iv_vecPos;
	m_iv_vecPos = NULL;

	m_iAttachment = -1;
}

void C_NPC_Hydra::OnLatchInterpolatedVariables(int flags)
{
	m_bNewChain = true;
	m_fLatchFlags = flags;

	BaseClass::OnLatchInterpolatedVariables(flags);
}

void C_NPC_Hydra::ResetLatched()
{
	for (int i = 0; i < m_numHydraBones; i++)
	{
		m_iv_vecPos[i].Reset();
	}

	BaseClass::ResetLatched();
}

bool C_NPC_Hydra::SetupBones(matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime)
{
	return BaseClass::SetupBones(pBoneToWorldOut, nMaxBones, boneMask, currentTime);
}

void C_NPC_Hydra::StandardBlendingRules(CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask)
{
	BaseClass::StandardBlendingRules(hdr, pos, q, currentTime, boneMask);

	VPROF("C_NPC_Hydra::StandardBlendingRules");

	if (!hdr)
		return;

	int i;

	bool bNewlyInited = false;

	if (m_numHydraBones != hdr->numbones())
	{
		m_numHydraBones = hdr->numbones();

		float poseparam[MAXSTUDIOPOSEPARAM];
		for (i = 0; i < hdr->GetNumPoseParameters(); i++)
		{
			poseparam[i] = 0;
		}

		//CalcPose(hdr, NULL, pos, q, 0.0f, 0.0f, poseparam, BONE_USED_BY_ANYTHING);

		if (m_boneLength)
		{
			delete[] m_boneLength;
		}

		m_boneLength = new float[m_numHydraBones];

		if (m_vecPos)
		{
			delete[] m_vecPos;
		}

		m_vecPos = new Vector[m_numHydraBones];

		if (m_iv_vecPos)
		{
			delete m_iv_vecPos;
		}

		m_iv_vecPos = new CInterpolatedVar<Vector>[m_numHydraBones];

		for (i = 0; i < m_numHydraBones; i++)
		{
			m_iv_vecPos[i].Setup(&m_vecPos[i], LATCH_SIMULATION_VAR | EXCLUDE_AUTO_LATCH | EXCLUDE_AUTO_INTERPOLATE);
		}

		m_maxPossibleLength = 0;

		for (i = 0; i < m_numHydraBones - 1; i++)
		{
			m_boneLength[i] = (pos[i + 1] - pos[i]).Length();
			m_maxPossibleLength += m_boneLength[i];
		}

		m_boneLength[i] = 0.0f;

		bNewlyInited = true;
	}

	if (m_bNewChain)
	{
		CalcBoneChain(m_vecPos, m_vecChain);
		for (i = 0; i < m_numHydraBones; i++)
		{
			m_vecPos[i] = m_vecPos[i] - GetAbsOrigin();

			if (m_fLatchFlags & LATCH_SIMULATION_VAR)
			{
				m_iv_vecPos[i].NoteChanged(currentTime, true);
			}
		}

		m_bNewChain = false;
	}

	if (bNewlyInited)
	{

		for (i = 0; i < m_numHydraBones; i++)
		{
			m_iv_vecPos[i].Reset();
		}
	}

	for (i = 0; i < m_numHydraBones; i++)
	{
		m_iv_vecPos[i].Interpolate(currentTime);
		pos[i] = m_vecPos[i];
	}

	CalcBoneAngles(pos, q);

	Quaternion qTmp;
	AngleQuaternion(QAngle(0, -90, 0), qTmp);
	QuaternionMult(q[m_numHydraBones - 1], qTmp, q[m_numHydraBones - 1]);
}

void  C_NPC_Hydra::CalcBoneChain(Vector pos[], const Vector chain[])
{
	int i, j;

	i = CHAIN_LINKS - 1;
	while (i > 0)
	{
		if ((chain[i] - chain[i - 1]).LengthSqr() > 0.0)
		{
			break;
		}

		i--;
	}

	j = m_numHydraBones - 1;

	float totalLength = 0;

	for (int k = i; k > 0; k--)
	{
		totalLength += (chain[k] - chain[k - 1]).Length();
	}

	totalLength = clamp(totalLength, 1.0, m_maxPossibleLength);

	float scale = m_flRelaxedLength / totalLength;

	float dist = -16;

	while (j >= 0 && i > 0)
	{
		float dt = (chain[i] - chain[i - 1]).Length() * scale;
		float dx = dt;
		while (j >= 0 && dist + dt >= m_boneLength[j])
		{
			float s = (dx - (dt - (m_boneLength[j] - dist))) / dx;

			if (s < 0 || s > 1.)
				s = 0;

			Catmull_Rom_Spline(chain[(i < CHAIN_LINKS - 1) ? i + 1 : CHAIN_LINKS - 1], chain[i], chain[(i > 0) ? i - 1 : 0], chain[(i > 1) ? i - 2 : 0], s, pos[j]);

			dt = dt - (m_boneLength[j] - dist);
			j--;
			dist = 0;
		}
		dist += dt;
		i--;
	}

	while (j >= 0)
	{
		pos[j] = chain[0];
		j--;
	}
}

void C_NPC_Hydra::CalcBoneAngles(const Vector pos[], Quaternion q[])
{
	int i;
	matrix3x4_t bonematrix;

	for (i = m_numHydraBones - 1; i >= 0; i--)
	{
		Vector forward;
		Vector left2;

		if (i != m_numHydraBones - 1)
		{
			QuaternionMatrix(q[i + 1], bonematrix);
			MatrixGetColumn(bonematrix, 1, left2);

			forward = (pos[i + 1] - pos[i]);
			float length = VectorNormalize(forward);
			if (length == 0.0)
			{
				q[i] = q[i + 1];
				continue;
			}
		}
		else
		{
			forward = m_vecHeadDir;
			VectorNormalize(forward);

			VectorMatrix(forward, bonematrix);
			MatrixGetColumn(bonematrix, 1, left2);
		}

		Vector up = CrossProduct(forward, left2);
		VectorNormalize(up);

		Vector left = CrossProduct(up, forward);

		MatrixSetColumn(forward, 0, bonematrix);
		MatrixSetColumn(left, 1, bonematrix);
		MatrixSetColumn(up, 2, bonematrix);

		QAngle angles;
		MatrixAngles(bonematrix, angles);
		AngleQuaternion(angles, q[i]);
	}
}

bool C_NPC_Hydra::GetSoundSpatialization(SpatializationInfo_t& info)
{
	bool bret = BaseClass::GetSoundSpatialization(info);
	if (bret)
	{
	}

	return bret;
}
void C_NPC_Hydra::ClientThink()
{
	BaseClass::ClientThink();
}