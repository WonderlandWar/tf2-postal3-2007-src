//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/Frame.h>
#include <vgui/IScheme.h>
#include <game/client/iviewport.h>
#include <vgui/IVGUI.h>
#include <KeyValues.h>
#include <FileSystem.h>

#include "vguicenterprint.h"
#include "tf_controls.h"
#include "tf_modelpanel.h"
#include "tf_teammenu.h"
#include <convar.h>
#include "IGameUIFuncs.h" // for key bindings
#include "hud.h" // for gEngfuncs
#include "c_tf_player.h"
#include "tf_gamerules.h"
#include "c_team.h"
#include "tf_hud_notification_panel.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFTeamButton::CTFTeamButton( vgui::Panel *parent, const char *panelName ) : CTFButton( parent, panelName, "" )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTeamButton::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetDefaultColor( GetFgColor(), m_clrDefaultBG );
	SetArmedColor( GetButtonFgColor(), m_clrArmedBG );
	SetDepressedColor( GetButtonFgColor(), m_clrDepressedBG );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFTeamMenu::CTFTeamMenu( IViewPort *pViewPort ) : CTeamMenu( pViewPort )
{
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	SetCloseButtonVisible( false );
	SetVisible( false );
	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );

	m_iTeamMenuKey = BUTTON_CODE_INVALID;

	m_pBlueTeamButton = new CTFTeamButton( this, "teambutton0" );
	m_pRedTeamButton = new CTFTeamButton( this, "teambutton1" );
	m_pAutoTeamButton = new CTFTeamButton( this, "teambutton2" );
	m_pSpecTeamButton = new CTFTeamButton( this, "teambutton3" );

	m_pCancelButton = new CTFButton( this, "CancelButton", "#TF_Cancel" );

	vgui::ivgui()->AddTickSignal( GetVPanel() );

	m_bRedDisabled = false;
	m_bBlueDisabled = false;

	LoadControlSettings( "Resource/UI/Teammenu.res" );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFTeamMenu::~CTFTeamMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTeamMenu::ShowPanel( bool bShow )
{
	if ( BaseClass::IsVisible() == bShow )
		return;

	if ( bShow )
	{
		if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN && 
			 C_TFPlayer::GetLocalTFPlayer() && 
			 C_TFPlayer::GetLocalTFPlayer()->GetTeamNumber() != TFGameRules()->GetWinningTeam()
			 && C_TFPlayer::GetLocalTFPlayer()->GetTeamNumber() != TEAM_SPECTATOR 
	  		 && C_TFPlayer::GetLocalTFPlayer()->GetTeamNumber() != TEAM_UNASSIGNED )
		{
			SetVisible( false );
			SetMouseInputEnabled( false );
			internalCenterPrint->Print( "#TF_CantChangeTeamNow" );
			return;
		}

		engine->CheckPoint( "TeamMenu" );

		Activate();
		SetMouseInputEnabled( true );

		// get key bindings if shown
		m_iTeamMenuKey = gameuifuncs->GetButtonCodeForBind( "changeteam" );
		m_iScoreBoardKey = gameuifuncs->GetButtonCodeForBind( "showscores" );
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );

		if ( IsConsole() )
		{
			// Close the door behind us
			CTFTeamButton *pButton = dynamic_cast< CTFTeamButton *> ( GetFocusNavGroup().GetCurrentFocus() );
			if ( pButton )
			{
				pButton->OnCursorExited();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: called to update the menu with new information
//-----------------------------------------------------------------------------
void CTFTeamMenu::Update( void )
{
	BaseClass::Update();

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( pLocalPlayer && ( pLocalPlayer->GetTeamNumber() != TEAM_UNASSIGNED ) )
	{
		if ( m_pCancelButton )
		{
			m_pCancelButton->SetVisible( true );
		}
	}
	else
	{
		if ( m_pCancelButton && m_pCancelButton->IsVisible() )
		{
			m_pCancelButton->SetVisible( false );
		}
	}
	
	CTFTeamButton *pButton = (CTFTeamButton*)FindChildByName( "teambutton2" );
	if ( pButton )
		pButton->RequestFocus( 0 );

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTeamMenu::Join_Team( const CCommand &args )
{
	if ( args.ArgC() > 1 )
	{
		char cmd[256];
		Q_snprintf( cmd, sizeof( cmd ), "jointeam %s", args.Arg( 1 ) );
		OnCommand( cmd );
	}
}

//-----------------------------------------------------------------------------
// Purpose: chooses and loads the text page to display that describes mapName map
//-----------------------------------------------------------------------------
void CTFTeamMenu::LoadMapPage( const char *mapName )
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTeamMenu::OnKeyCodePressed( KeyCode code )
{
	if ( ( m_iTeamMenuKey != BUTTON_CODE_INVALID && m_iTeamMenuKey == code ) )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

		if ( pLocalPlayer && ( pLocalPlayer->GetTeamNumber() != TEAM_UNASSIGNED ) )
		{
			ShowPanel( false );
		}
	}
	else if( code == KEY_SPACE )
	{
		engine->ClientCmd( "jointeam auto" );

		ShowPanel( false );
		OnClose();
	}
	else if ( m_iScoreBoardKey != BUTTON_CODE_INVALID && m_iScoreBoardKey == code )
	{
		gViewPortInterface->ShowPanel( PANEL_SCOREBOARD, true );
		gViewPortInterface->PostMessageToPanel( PANEL_SCOREBOARD, new KeyValues( "PollHideCode", "code", code ) );
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when the user picks a team
//-----------------------------------------------------------------------------
void CTFTeamMenu::OnCommand( const char *command )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( Q_stricmp( command, "vguicancel" ) )
	{
		// we're selecting a team, so make sure it's not the team we're already on before sending to the server
		if ( pLocalPlayer && ( Q_strstr( command, "jointeam " ) ) )
		{
			const char *pTeam = command + Q_strlen( "jointeam " );
			int iTeam = TEAM_INVALID;

			if ( Q_stricmp( pTeam, "spectate" ) == 0 )
			{
				iTeam = TEAM_SPECTATOR;
			}
			else if ( Q_stricmp( pTeam, "red" ) == 0 )
			{
				iTeam = TF_TEAM_RED;
			}
			else if ( Q_stricmp( pTeam, "blue" ) == 0 )
			{
				iTeam = TF_TEAM_BLUE;
			}

			if ( iTeam == TF_TEAM_RED && m_bRedDisabled )
			{
				return;
			}

			if ( iTeam == TF_TEAM_BLUE && m_bBlueDisabled )
			{
				return;
			}

			// are we selecting the team we're already on?
			if ( pLocalPlayer->GetTeamNumber() != iTeam )
			{
				engine->ClientCmd( command );
			}
		}
	}

	BaseClass::OnCommand( command );
	ShowPanel( false );
	OnClose();
}

//-----------------------------------------------------------------------------
// Frame-based update
//-----------------------------------------------------------------------------
void CTFTeamMenu::OnTick()
{
	// update the number of players on each team

	// enable or disable buttons based on team limit

	C_Team *pRed = GetGlobalTeam( TF_TEAM_RED );
	C_Team *pBlue = GetGlobalTeam( TF_TEAM_BLUE );

	if ( !pRed || !pBlue )
		return;

	// set our team counts
	SetDialogVariable( "bluecount", pBlue->Get_Number_Players() );
	SetDialogVariable( "redcount", pRed->Get_Number_Players() );

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pLocalPlayer )
		return;

	CTFGameRules *pRules = TFGameRules();

	if ( !pRules )
		return;

	// check if teams are unbalanced
	m_bRedDisabled = m_bBlueDisabled = false;

	int iHeavyTeam, iLightTeam;

	bool bUnbalanced = pRules->AreTeamsUnbalanced( iHeavyTeam, iLightTeam );
	
	int iCurrentTeam = pLocalPlayer->GetTeamNumber();

	if ( ( bUnbalanced && iHeavyTeam == TF_TEAM_RED ) || ( pRules->WouldChangeUnbalanceTeams( TF_TEAM_RED, iCurrentTeam ) ) )
	{
		m_bRedDisabled = true;
	}

	if ( ( bUnbalanced && iHeavyTeam == TF_TEAM_BLUE ) || ( pRules->WouldChangeUnbalanceTeams( TF_TEAM_BLUE, iCurrentTeam ) ) )
	{
		m_bBlueDisabled = true;
	}
}