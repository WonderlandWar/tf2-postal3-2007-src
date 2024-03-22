//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_classmenu.h"

#include <KeyValues.h>
#include <FileSystem.h>
#include <vgui_controls/Button.h>
#include <vgui/IVGUI.h>

#include "hud.h" // for gEngfuncs
#include "c_tf_player.h"
#include "c_tf_team.h"
#include "c_tf_playerresource.h"

#include "tf_controls.h"
#include "vguicenterprint.h"
#include "imagemouseoverbutton.h"
#include "iconpanel.h"

#include "IGameUIFuncs.h" // for key bindings

#include "tf_hud_notification_panel.h"

extern IGameUIFuncs *gameuifuncs; // for key binding details

using namespace vgui;

ConVar _cl_classmenuopen( "_cl_classmenuopen", "0", 0, "internal cvar used to tell server when class menu is open" );

// menu buttons are not in the same order as the defines
static int iRemapIndexToClass[TF_CLASS_MENU_BUTTONS] = 
{
	0,
	TF_CLASS_SCOUT,
	TF_CLASS_SOLDIER,
	TF_CLASS_PYRO,
	TF_CLASS_DEMOMAN,
	TF_CLASS_HEAVYWEAPONS,
	TF_CLASS_ENGINEER,
	TF_CLASS_MEDIC,
	TF_CLASS_SNIPER,
	TF_CLASS_SPY,
	0,
	0,
	TF_CLASS_RANDOM
};

int GetIndexForClass( int iClass )
{
	int iIndex = 0;

	for ( int i = 0 ; i < TF_CLASS_MENU_BUTTONS ; i++ )
	{
		if ( iRemapIndexToClass[i] == iClass )
		{
			iIndex = i;
			break;
		}
	}

	return iIndex;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFClassMenu::CTFClassMenu( IViewPort *pViewPort ) : CClassMenu( pViewPort )
{
	m_mouseoverButtons.RemoveAll();

	m_iClassMenuKey = BUTTON_CODE_INVALID;
	m_iCurrentClassIndex = TF_CLASS_HEAVYWEAPONS;

#ifdef _X360
	m_pFooter = new CTFFooter( this, "Footer" );
#endif

	m_pClassInfoPanel = new CTFClassInfoPanel( this, "ClassInfoPanel" );
	LoadControlSettings( "Resource/UI/ClassInfoPanel.res" );

	Q_memset( m_pClassButtons, 0, sizeof( m_pClassButtons ) );

	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassMenu::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/ClassMenu.res" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CImageMouseOverButton<CTFClassInfoPanel> *CTFClassMenu::GetCurrentClassButton()
{
	int iClass = TF_CLASS_HEAVYWEAPONS;
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( pLocalPlayer && pLocalPlayer->m_Shared.GetDesiredPlayerClassIndex() != TF_CLASS_UNDEFINED )
	{
		iClass = pLocalPlayer->m_Shared.GetDesiredPlayerClassIndex();
	}

	m_iCurrentClassIndex = GetIndexForClass( iClass );
	return m_pClassButtons[iClass];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassMenu::ShowPanel( bool bShow )
{
	if ( bShow )
	{
		// can't change class if you're on the losing team during the "bonus time" after a team has won the round
		if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN && 
			 C_TFPlayer::GetLocalTFPlayer() && 
			 C_TFPlayer::GetLocalTFPlayer()->GetTeamNumber() != TFGameRules()->GetWinningTeam()
			 && C_TFPlayer::GetLocalTFPlayer()->GetTeamNumber() != TEAM_SPECTATOR 
			 && C_TFPlayer::GetLocalTFPlayer()->GetTeamNumber() != TEAM_UNASSIGNED
			 && GetSpectatorMode() == OBS_MODE_NONE )
		{
			SetVisible( false );
			SetMouseInputEnabled( false );

			internalCenterPrint->Print( "#TF_CantChangeClassNow" );
			return;
		}

		engine->CheckPoint( "ClassMenu" );

		Activate();
		SetMouseInputEnabled( true );

		m_iClassMenuKey = gameuifuncs->GetButtonCodeForBind( "changeclass" );
		m_iScoreBoardKey = gameuifuncs->GetButtonCodeForBind( "showscores" );

		CImageMouseOverButton<CTFClassInfoPanel> *pInitialButton = GetCurrentClassButton();

		for( int i = 0; i < GetChildCount(); i++ ) 
		{
			CImageMouseOverButton<CTFClassInfoPanel> *button = dynamic_cast<CImageMouseOverButton<CTFClassInfoPanel> *>( GetChild( i ) );

			if ( button )
			{
				if ( button == pInitialButton )
				{
					button->ShowPage();
					button->SetArmed( true );
					button->SetAsDefaultButton( 1 );

					g_lastButton = button;
				}
				else
				{
					button->HidePage();
					button->SetArmed( false );
				}
			}
		}
	}
	else
	{
		// turn off all of our ClassInfo panels so the VCDs will stop playing
		for( int i = 0; i < GetChildCount(); i++ ) 
		{
			CImageMouseOverButton<CTFClassInfoPanel> *button = dynamic_cast<CImageMouseOverButton<CTFClassInfoPanel> *>( GetChild( i ) );

			if ( button )
			{
				button->HidePage();
			}
		}

		// everything is off so just reset these for next time
		g_lastButton = NULL;
		g_lastPanel = NULL;

		SetVisible( false );
		SetMouseInputEnabled( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassMenu::OnKeyCodePressed( KeyCode code )
{
	if ( ( m_iClassMenuKey != BUTTON_CODE_INVALID && m_iClassMenuKey == code ) ||
		code == KEY_XBUTTON_BACK || 
		code == KEY_XBUTTON_B )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

		if ( pLocalPlayer && ( pLocalPlayer->GetPlayerClass()->GetClassIndex() != TF_CLASS_UNDEFINED ) )
		{
			ShowPanel( false );
		}
	}
	else if( code == KEY_SPACE || code == KEY_XBUTTON_A )
	{
		ipanel()->SendMessage( GetFocusNavGroup().GetDefaultButton(), new KeyValues( "PressButton" ), GetVPanel() );
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassMenu::Update()
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	// Force them to pick a class if they haven't picked one yet.
	if ( ( pLocalPlayer && pLocalPlayer->m_Shared.GetDesiredPlayerClassIndex() != TF_CLASS_UNDEFINED ) )
	{
#ifdef _X360
		if ( m_pFooter )
		{
			m_pFooter->ShowButtonLabel( "cancel", true );
		}
#else
		SetVisibleButton( "CancelButton", true );
#endif
	}
	else
	{
#ifdef _X360
		if ( m_pFooter )
		{
			m_pFooter->ShowButtonLabel( "cancel", false );
		}
#else
		SetVisibleButton( "CancelButton", false );
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Panel *CTFClassMenu::CreateControlByName( const char *controlName )
{
	if ( !Q_stricmp( "CIconPanel", controlName ) )
	{
		return new CIconPanel( this, "icon_panel" );
	}
	else
	{
		return BaseClass::CreateControlByName( controlName );
	}
}

//-----------------------------------------------------------------------------
// Catch the mouseover event and set the active class
//-----------------------------------------------------------------------------
void CTFClassMenu::OnShowPage( const char *pagename )
{
}

//-----------------------------------------------------------------------------
// Draw nothing
//-----------------------------------------------------------------------------
void CTFClassMenu::PaintBackground( void )
{
}

//-----------------------------------------------------------------------------
// Do things that should be done often, eg number of players in the 
// selected class
//-----------------------------------------------------------------------------
void CTFClassMenu::OnTick( void )
{
	//When a player changes teams, their class and team values don't get here 
	//necessarily before the command to update the class menu. This leads to the cancel button 
	//being visible and people cancelling before they have a class. check for class == TF_CLASS_UNDEFINED and if so
	//hide the cancel button

	if ( !IsVisible() )
		return;

#ifndef _X360
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	// Force them to pick a class if they haven't picked one yet.
	if ( pLocalPlayer && pLocalPlayer->m_Shared.GetDesiredPlayerClassIndex() == TF_CLASS_UNDEFINED )
	{
		SetVisibleButton( "CancelButton", false );
	}

#endif

	BaseClass::OnTick();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassMenu::OnClose()
{
	ShowPanel( false );

	BaseClass::OnClose();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassMenu::SetVisible( bool state )
{
	BaseClass::SetVisible( state );

	if ( state )
	{
		engine->ServerCmd( "menuopen" );			// to the server
		engine->ClientCmd( "_cl_classmenuopen 1" );	// for other panels
	}
	else
	{
		engine->ServerCmd( "menuclosed" );	
		engine->ClientCmd( "_cl_classmenuopen 0" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Console command to select a class
//-----------------------------------------------------------------------------
void CTFClassMenu::Join_Class( const CCommand &args )
{
	if ( args.ArgC() > 1 )
	{
		char cmd[256];
		Q_snprintf( cmd, sizeof( cmd ), "joinclass %s", args.Arg( 1 ) );
		OnCommand( cmd );
		ShowPanel( false );
	}
}