//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: TF's custom CPlayerResource
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tf_player.h"
#include "player_resource.h"
#include "tf_player_resource.h"
#include "tf_gamestats.h"
#include "tf_gamerules.h"
#include <coordsize.h>

// Datatable
IMPLEMENT_SERVERCLASS_ST( CTFPlayerResource, DT_TFPlayerResource )
	SendPropArray3( SENDINFO_ARRAY3( m_iTotalScore ), SendPropInt( SENDINFO_ARRAY( m_iTotalScore ), 10, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iCaptures ), SendPropInt( SENDINFO_ARRAY( m_iCaptures ), 10, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iDefenses ), SendPropInt( SENDINFO_ARRAY( m_iDefenses ), 10, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iDominations ), SendPropInt( SENDINFO_ARRAY( m_iDominations ), 10, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iRevenge ), SendPropInt( SENDINFO_ARRAY( m_iRevenge ), 10, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iBuildingsDestroyed ), SendPropInt( SENDINFO_ARRAY( m_iBuildingsDestroyed ), 10, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iHeadshots ), SendPropInt( SENDINFO_ARRAY( m_iHeadshots ), 10, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iBackstabs ), SendPropInt( SENDINFO_ARRAY( m_iBackstabs ), 10, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iHealPoints ), SendPropInt( SENDINFO_ARRAY( m_iHealPoints ), 20, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iInvulns ), SendPropInt( SENDINFO_ARRAY( m_iInvulns ), 10, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iTeleports ), SendPropInt( SENDINFO_ARRAY( m_iTeleports ), 10, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iResupplyPoints ), SendPropInt( SENDINFO_ARRAY( m_iResupplyPoints ), 10, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iKillAssists ), SendPropInt( SENDINFO_ARRAY( m_iKillAssists ), 10, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iMaxHealth ), SendPropInt( SENDINFO_ARRAY( m_iMaxHealth ), 10, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iPlayerClass ), SendPropInt( SENDINFO_ARRAY( m_iPlayerClass ), 5, SPROP_UNSIGNED ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( tf_player_manager, CTFPlayerResource );

CTFPlayerResource::CTFPlayerResource( void )
{
	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerResource::UpdatePlayerData( void )
{
	int i;

	BaseClass::UpdatePlayerData();

	for ( i = 1 ; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = (CTFPlayer*)UTIL_PlayerByIndex( i );
		
		if ( pPlayer && pPlayer->IsConnected() )
		{			
			PlayerStats_t *pPlayerStats = CTF_GameStats.FindPlayerStats( pPlayer );
			
			RoundStats_t *pAliveStats = &pPlayerStats->statsCurrentLife;
			if ( pPlayerStats ) 
			{
			
			m_iTeleports.Set( i, pAliveStats->m_iStat[TFSTAT_TELEPORTS] );
			m_iHealPoints.Set( i, pAliveStats->m_iStat[TFSTAT_HEALING] ); // TFP3: This is probably incorrect, fix me later...
			m_iInvulns.Set( i, pAliveStats->m_iStat[TFSTAT_INVULNS] );
			m_iKillAssists.Set( i, pAliveStats->m_iStat[TFSTAT_KILLASSISTS] );
			m_iCaptures.Set( i, pAliveStats->m_iStat[TFSTAT_CAPTURES] );
			m_iDefenses.Set( i, pAliveStats->m_iStat[TFSTAT_DEFENSES] );
			m_iDominations.Set( i, pAliveStats->m_iStat[TFSTAT_DOMINATIONS] );
			m_iRevenge.Set( i, pAliveStats->m_iStat[TFSTAT_REVENGE] );
			m_iBuildingsDestroyed.Set( i, pAliveStats->m_iStat[TFSTAT_BUILDINGSDESTROYED] );
			m_iHeadshots.Set( i, pAliveStats->m_iStat[TFSTAT_HEADSHOTS] );
			m_iBackstabs.Set( i, pAliveStats->m_iStat[TFSTAT_BACKSTABS] );
			
			m_iMaxHealth.Set( i, pPlayer->GetPlayerClass()->GetMaxHealth() );
			m_iPlayerClass.Set( i, pPlayer->GetPlayerClass()->GetClassIndex() );
				int iTotalScore = CTFGameRules::CalcPlayerScore( &pPlayerStats->statsAccumulated );
				m_iTotalScore.Set( i, iTotalScore );
			}					
		}
	}
}

void CTFPlayerResource::Spawn( void )
{
	int i;

	for ( i = 0; i < MAX_PLAYERS + 1; i++ )
	{
		m_iTotalScore.Set( i, 0 );
		m_iHealPoints.Set( i, 0 );
		m_iTeleports.Set( i, 0 );
		m_iResupplyPoints.Set( i, 0 );
		m_iKillAssists.Set( i, 0 );
		m_iCaptures.Set( i, 0 );
		m_iMaxHealth.Set( i, TF_HEALTH_UNDEFINED );
		m_iPlayerClass.Set( i, TF_CLASS_UNDEFINED );
	}

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Gets a value from an array member
//-----------------------------------------------------------------------------
int CTFPlayerResource::GetTotalScore( int iIndex )
{
	CTFPlayer *pPlayer = (CTFPlayer*)UTIL_PlayerByIndex( iIndex );

	if ( pPlayer && pPlayer->IsConnected() )
	{	
		return m_iTotalScore[iIndex];
	}

	return 0;
}