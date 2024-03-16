//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: TF's custom C_PlayerResource
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_TF_PLAYERRESOURCE_H
#define C_TF_PLAYERRESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "c_playerresource.h"

class C_TF_PlayerResource : public C_PlayerResource
{
	DECLARE_CLASS( C_TF_PlayerResource, C_PlayerResource );
public:
	DECLARE_CLIENTCLASS();

	C_TF_PlayerResource();
	virtual ~C_TF_PlayerResource();

	int	GetTotalScore( int iIndex ) { return GetArrayValue( iIndex, m_iTotalScore, 0 ); }
	int GetCaptures( int iIndex ) { return GetArrayValue( iIndex, m_iCaptures, 0 ); }
	int GetDefenses( int iIndex ) { return GetArrayValue( iIndex, m_iDefenses, 0 ); }
	int GetDominations( int iIndex ) { return GetArrayValue( iIndex, m_iDominations, 0 ); }
	int GetRevenge( int iIndex ) { return GetArrayValue( iIndex, m_iRevenge, 0 ); }
	int GetBuildingsDestroyed( int iIndex ) { return GetArrayValue( iIndex, m_iBuildingsDestroyed, 0 ); }
	int GetHeadshots( int iIndex ) { return GetArrayValue( iIndex, m_iHeadshots, 0 ); }
	int GetBackstabs( int iIndex ) { return GetArrayValue( iIndex, m_iBackstabs, 0 ); }
	int GetHealPoints( int iIndex ) { return GetArrayValue( iIndex, m_iHealPoints, 0 ); }
	int GetInvulns( int iIndex ) { return GetArrayValue( iIndex, m_iInvulns, 0 ); }
	int GetTeleports( int iIndex ) { return GetArrayValue( iIndex, m_iTeleports, 0 ); }
	int GetResupplyPoints( int iIndex ) { return GetArrayValue( iIndex, m_iResupplyPoints, 0 ); }
	int GetKillAssists( int iIndex ) { return GetArrayValue( iIndex, m_iKillAssists, 0 ); }
	int GetMaxHealth( int iIndex )   { return GetArrayValue( iIndex, m_iMaxHealth, TF_HEALTH_UNDEFINED ); }
	int GetPlayerClass( int iIndex ) { return GetArrayValue( iIndex, m_iPlayerClass, TF_CLASS_UNDEFINED ); }

	int GetCountForPlayerClass( int iClass );
	
protected:
	int GetArrayValue( int iIndex, int *pArray, int defaultVal );

	int		m_iTotalScore[MAX_PLAYERS+1];
	int		m_iCaptures[MAX_PLAYERS+1];
	int		m_iDefenses[MAX_PLAYERS+1];
	int		m_iDominations[MAX_PLAYERS+1];
    int		m_iRevenge[MAX_PLAYERS+1];
    int		m_iBuildingsDestroyed[MAX_PLAYERS+1];
    int		m_iHeadshots[MAX_PLAYERS+1];
    int		m_iBackstabs[MAX_PLAYERS+1];
    int		m_iHealPoints[MAX_PLAYERS+1];
    int		m_iInvulns[MAX_PLAYERS+1];
    int		m_iTeleports[MAX_PLAYERS+1];
    int		m_iResupplyPoints[MAX_PLAYERS+1];
    int		m_iKillAssists[MAX_PLAYERS+1];
	int		m_iMaxHealth[MAX_PLAYERS+1];
	int		m_iPlayerClass[MAX_PLAYERS+1];
};


#endif // C_TF_PLAYERRESOURCE_H
