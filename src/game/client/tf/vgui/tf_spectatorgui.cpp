//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "c_team.h"
#include "tf_spectatorgui.h"
#include "tf_shareddefs.h"
#include "tf_gamerules.h"
#include "tf_hud_objectivestatus.h"
#include "tf_hud_statpanel.h"
#include "iclientmode.h"
#include "c_playerresource.h"
#include "tf_hud_building_status.h"
#include "tf_hud_winpanel.h"
#include "tf_tips.h"
#include "tf_mapinfomenu.h"

#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>

using namespace vgui;

extern ConVar _cl_classmenuopen;

const char *GetMapDisplayName( const char *mapName );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFSpectatorGUI::CTFSpectatorGUI(IViewPort *pViewPort) : CSpectatorGUI(pViewPort)
{
	m_flNextTipChangeTime = 0;
	m_iTipClass = TF_CLASS_UNDEFINED;

	m_nEngBuilds_xpos = m_nEngBuilds_ypos = 0;
	m_nSpyBuilds_xpos = m_nSpyBuilds_ypos = 0;

	m_pTargetHealth = new CTFSpectatorGUIHealth( this, "SpectatorGUIHealth" );

	m_pReinforcementsLabel = new Label( this, "ReinforcementsLabel", "" );
	m_pTargetNameLabel = new Label ( this, "TargetNameLabel", "" );
	m_pSpectatingLabel = new Label ( this, "SpectatingLabel", "" );
	m_pClassOrTeamKeyLabel = new Label ( this, "ClassOrTeamKeyLabel", "" );
	m_pClassOrTeamLabel = new Label( this, "ClassOrTeamLabel", "" );
	m_pSwitchCamModeKeyLabel = new Label( this, "SwitchCamModeKeyLabel", "" );
	m_pCycleTargetFwdKeyLabel = new Label( this, "CycleTargetFwdKeyLabel", "" );
	m_pCycleTargetRevKeyLabel = new Label( this, "CycleTargetRevKeyLabel", "" );
	m_pMapLabel = new Label( this, "MapLabel", "" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSpectatorGUI::Reset( void )
{
	BaseClass::Reset();
}

//-----------------------------------------------------------------------------
// Purpose: makes the GUI fill the screen
//-----------------------------------------------------------------------------
void CTFSpectatorGUI::PerformLayout( void )
{
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFSpectatorGUI::NeedsUpdate( void )
{
	if ( !C_BasePlayer::GetLocalPlayer() )
		return false;

	if( IsVisible() )
		return true;

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		
	if ( m_nLastSpecMode != pPlayer->GetObserverMode() )
		return true;

	if ( m_nLastSpecTarget != pPlayer->GetObserverTarget() )
		return true;

	return BaseClass::NeedsUpdate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSpectatorGUI::Update()
{
	BaseClass::Update();
	
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer )
	{
		m_nLastSpecMode = pPlayer->GetObserverMode();
		m_nLastSpecTarget = pPlayer->GetObserverTarget();
	}

	UpdateReinforcements();
	UpdateTarget();
	UpdateKeyLabels();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSpectatorGUI::UpdateReinforcements( void )
{
	if( !m_pReinforcementsLabel )
		return;

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer || pPlayer->IsHLTV() ||
		( pPlayer->GetTeamNumber() != TF_TEAM_RED && pPlayer->GetTeamNumber() != TF_TEAM_BLUE ) ||
		( pPlayer->m_Shared.GetState() != TF_STATE_OBSERVER ) && ( pPlayer->m_Shared.GetState() != TF_STATE_DYING ) ||
		( pPlayer->GetObserverMode() == OBS_MODE_FREEZECAM ) )
	{
		m_pReinforcementsLabel->SetVisible( false );
		return;
	}

	wchar_t wLabel[128];
	
	if ( TFGameRules()->InStalemate() )
	{
		g_pVGuiLocalize->ConstructString( wLabel, sizeof( wLabel ), g_pVGuiLocalize->Find( "#game_respawntime_stalemate" ), 0 );
	}
	else
	{
		float flNextRespawn = TFGameRules()->GetNextRespawnWave( pPlayer->GetTeamNumber(), pPlayer );
		if ( !flNextRespawn )
		{
			m_pReinforcementsLabel->SetVisible( false );
			return;
		}

		int iRespawnWait = (flNextRespawn - gpGlobals->curtime);
		if ( iRespawnWait <= 0 )
		{
			g_pVGuiLocalize->ConstructString( wLabel, sizeof( wLabel ), g_pVGuiLocalize->Find("#game_respawntime_now" ), 0 );
		}
		else if ( iRespawnWait <= 1.0 )
		{
			g_pVGuiLocalize->ConstructString( wLabel, sizeof( wLabel ), g_pVGuiLocalize->Find("#game_respawntime_in_sec" ), 0 );
		}
		else
		{
			char szSecs[6];
			Q_snprintf( szSecs, sizeof(szSecs), "%d", iRespawnWait );
			wchar_t wSecs[4];
			g_pVGuiLocalize->ConvertANSIToUnicode(szSecs, wSecs, sizeof(wSecs));
			g_pVGuiLocalize->ConstructString( wLabel, sizeof( wLabel ), g_pVGuiLocalize->Find("#game_respawntime_in_secs" ), 1, wSecs );
		}
	}

	m_pReinforcementsLabel->SetVisible( true );
	m_pReinforcementsLabel->SetText( wLabel, true );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSpectatorGUI::UpdateKeyLabels( void )
{
	if ( m_pClassOrTeamKeyLabel && m_pClassOrTeamLabel )
	{
		const char *pszBinding = NULL;

		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pPlayer )
		{
			const char *szTemp = NULL;

			if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR )
			{
				pszBinding = engine->Key_LookupBinding( "changeteam" );
				szTemp = "#TF_Spectator_ChangeTeam";
			}
			else
			{
				pszBinding = engine->Key_LookupBinding( "changeclass" );
				szTemp = "#TF_Spectator_ChangeClass";
			}

			m_pClassOrTeamLabel->SetText( szTemp );
		}

		char szBinding[16];		
		Q_snprintf( szBinding, sizeof( szBinding ), "%s", pszBinding );

		wchar_t wBinding[16] = L"";
		g_pVGuiLocalize->ConvertANSIToUnicode( szBinding, wBinding, sizeof( wBinding ) );
		wchar_t *wzTemp = g_pVGuiLocalize->Find( "#TF_Spectator_ClassOrTeamKey" );

		g_pVGuiLocalize->ConstructString( wzTemp, 512, wBinding, 1 );

		m_pClassOrTeamKeyLabel->SetText( szBinding );
	}

	if ( m_pSwitchCamModeKeyLabel )
	{
		const char *pszKey = engine->Key_LookupBinding( "+jump" );
		wchar_t wKey[16] = L"";
		g_pVGuiLocalize->ConvertANSIToUnicode( pszKey, wKey, sizeof( wKey ) );

		wchar_t wLabel[256] = L"";
		const wchar_t *wzTemp = g_pVGuiLocalize->Find( "TF_Spectator_SwitchCamModeKey" );		
		g_pVGuiLocalize->ConstructString( wLabel, sizeof( wLabel ), wKey, sizeof ( wKey ), 1, wzTemp );

		m_pSwitchCamModeKeyLabel->SetText( wLabel );
	}

	if ( m_pCycleTargetFwdKeyLabel )
	{
		const char *pszKey = engine->Key_LookupBinding( "+attack" );
		wchar_t wKey[16] = L"";
		g_pVGuiLocalize->ConvertANSIToUnicode( pszKey, wKey, sizeof( wKey ) );

		wchar_t wLabel[256] = L"";
		const wchar_t *wzTemp = g_pVGuiLocalize->Find( "TF_Spectator_CycleTargetFwdKey" );
		g_pVGuiLocalize->ConstructString( wLabel, sizeof( wLabel ), wKey, sizeof ( wKey ), 1, wzTemp );

		m_pCycleTargetFwdKeyLabel->SetText( wLabel );
	}

	if ( m_pCycleTargetRevKeyLabel )
	{
		const char *pszKey = engine->Key_LookupBinding( "+attack2" );
		wchar_t wKey[16] = L"";
		g_pVGuiLocalize->ConvertANSIToUnicode( pszKey, wKey, sizeof( wKey ) );

		wchar_t wLabel[256] = L"";
		const wchar_t *wzTemp = g_pVGuiLocalize->Find( "TF_Spectator_CycleTargetRevKey" );		
		g_pVGuiLocalize->ConstructString( wLabel, sizeof( wLabel ), wKey, sizeof ( wKey ), 1, wzTemp );

		m_pCycleTargetRevKeyLabel->SetText( wLabel );
	}

	if ( m_pMapLabel )
	{
		wchar_t wMapName[16];
		wchar_t wLabel[256];
		char szMapName[16];

		char tempname[128];
		Q_FileBase( engine->GetLevelName(), tempname, sizeof( tempname ) );
		Q_strlower( tempname );

		if ( IsX360() )
		{
			char *pExt = Q_stristr( tempname, ".360" );
			if ( pExt )
			{
				*pExt = '\0';
			}
		}

		Q_strncpy( szMapName, GetMapDisplayName( tempname ), sizeof( szMapName ) );

		g_pVGuiLocalize->ConvertANSIToUnicode( szMapName, wMapName, sizeof(wMapName));
		g_pVGuiLocalize->ConstructString( wLabel, sizeof( wLabel ), g_pVGuiLocalize->Find( "#Spec_Map" ), 1, wMapName );

		m_pMapLabel->SetText( wLabel ); 
	}
}

//-----------------------------------------------------------------------------
// Purpose: shows/hides the buy menu
//-----------------------------------------------------------------------------
void CTFSpectatorGUI::ShowPanel(bool bShow)
{
	if ( bShow != IsVisible() )
	{
		CTFHudObjectiveStatus *pStatus = GET_HUDELEMENT( CTFHudObjectiveStatus );
		CHudBuildingStatusContainer_Engineer *pEngBuilds = GET_NAMED_HUDELEMENT( CHudBuildingStatusContainer_Engineer, BuildingStatus_Engineer );
		CHudBuildingStatusContainer_Spy *pSpyBuilds = GET_NAMED_HUDELEMENT( CHudBuildingStatusContainer_Spy, BuildingStatus_Spy );

		if ( bShow )
		{
			int xPos = 0, yPos = 0;

			if ( pStatus )
			{
				pStatus->SetParent( this );
				pStatus->SetProportional( true );
			}

			if ( pEngBuilds )
			{
				pEngBuilds->GetPos( xPos, yPos );
				m_nEngBuilds_xpos = xPos;
				m_nEngBuilds_ypos = yPos;
				pEngBuilds->SetPos( xPos, GetTopBarHeight() );
			}

			if ( pSpyBuilds )
			{
				pSpyBuilds->GetPos( xPos, yPos );
				m_nSpyBuilds_xpos = xPos;
				m_nSpyBuilds_ypos = yPos;
				pSpyBuilds->SetPos( xPos, GetTopBarHeight() );
			}	

			CTFWinPanel *pWinPanel = GET_HUDELEMENT( CTFWinPanel );

			// TFP3: May not be accurate...
			bool bShow = pWinPanel && !pWinPanel->IsVisible();			
			ShowSpectatingTarget( bShow );

			m_flNextTipChangeTime = 0;	// force a new tip immediately

			InvalidateLayout();
		}
		else
		{
			if ( pStatus )
			{
				pStatus->SetParent( g_pClientMode->GetViewport() );
			}

			if ( pEngBuilds )
			{
				pEngBuilds->SetPos( m_nEngBuilds_xpos, m_nEngBuilds_ypos );
			}

			if ( pSpyBuilds )
			{
				pSpyBuilds->SetPos( m_nSpyBuilds_xpos, m_nSpyBuilds_ypos );
			}
		}

		UpdateKeyLabels();
	}

	BaseClass::ShowPanel( bShow );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSpectatorGUI::ShowSpectatingTarget( bool bShow )
{
	if ( m_pTargetNameLabel )
		m_pTargetNameLabel->SetVisible( bShow );

	if ( m_pSpectatingLabel )
		m_pSpectatingLabel->SetVisible( bShow );
  
	if ( m_pTargetHealth )
		m_pTargetHealth->SetVisible( bShow );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSpectatorGUI::UpdateTarget( void )
{
	CTFWinPanel *pWinPanel = GET_HUDELEMENT( CTFWinPanel );
	if ( pWinPanel && pWinPanel->IsVisible() )
	{
		ShowSpectatingTarget( false );
		return;
	}

	int iSpecTarget = GetSpectatorTarget();

	CBaseEntity *pSpecTarget = ClientEntityList().GetBaseEntity( iSpecTarget );

	if ( iSpecTarget <= 0 || iSpecTarget > gpGlobals->maxClients || pSpecTarget == C_BasePlayer::GetLocalPlayer() )
	{
		if ( m_pTargetNameLabel && m_pTargetNameLabel->IsVisible() )
			ShowSpectatingTarget( false );

		return;
	}
	
	if ( m_pTargetNameLabel->IsVisible() && ShouldShowSpectatingTarget() )
	{	
		ShowSpectatingTarget( true );
	}
	
	int iHealth = pSpecTarget->GetHealth();

	if ( pSpecTarget->IsPlayer() )
	{
		C_BasePlayer *pPlayer = ToBasePlayer( pSpecTarget );
		if ( pPlayer->IsObserver() )
			iHealth = 0;
	}

	int iMaxBuffedHealth = 0;
	
	C_TFPlayer *pPlayer = ToTFPlayer( pSpecTarget );
	if ( pPlayer )
		iMaxBuffedHealth = pPlayer->m_Shared.GetMaxBuffedHealth();

	m_pTargetHealth->SetHealth( iHealth, pSpecTarget->GetMaxHealth(), iMaxBuffedHealth );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFSpectatorGUI::ShouldShowSpectatingTarget( void )
{
	CTFWinPanel *pWinPanel = GET_HUDELEMENT( CTFWinPanel );

	return pWinPanel && !pWinPanel->IsVisible();
}