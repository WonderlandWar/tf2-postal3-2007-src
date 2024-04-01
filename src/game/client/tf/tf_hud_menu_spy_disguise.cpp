//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include "c_baseobject.h"

#include "tf_hud_menu_spy_disguise.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//======================================

DECLARE_HUDELEMENT( CHudMenuSpyDisguise );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudMenuSpyDisguise::CHudMenuSpyDisguise( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudMenuSpyDisguise" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	for ( int i=0; i<9; i++ )
	{
		char buf[32];
		Q_snprintf( buf, sizeof(buf), "class_item_red_%d", i+1 );
		m_pClassItems_Red[i] = new EditablePanel( this, buf );

		Q_snprintf( buf, sizeof(buf), "class_item_blue_%d", i+1 );
		m_pClassItems_Blue[i] = new EditablePanel( this, buf );
	}

	m_iShowingTeam = TF_TEAM_RED;

	ListenForGameEvent( "spy_pda_reset" );
}

ConVar tf_disguise_menu_controller_mode( "tf_disguise_menu_controller_mode", "0", FCVAR_ARCHIVE, "Use console controller disguise menus. 1 = ON, 0 = OFF." );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuSpyDisguise::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/disguise_menu/HudMenuSpyDisguise.res" );

	m_pClassItems_Red[0]->LoadControlSettings( "resource/UI/disguise_menu/scout_red.res" );
	m_pClassItems_Red[1]->LoadControlSettings( "resource/UI/disguise_menu/soldier_red.res" );
	m_pClassItems_Red[2]->LoadControlSettings( "resource/UI/disguise_menu/pyro_red.res" );
	m_pClassItems_Red[3]->LoadControlSettings( "resource/UI/disguise_menu/demoman_red.res" );
	m_pClassItems_Red[4]->LoadControlSettings( "resource/UI/disguise_menu/heavy_red.res" );
	m_pClassItems_Red[5]->LoadControlSettings( "resource/UI/disguise_menu/engineer_red.res" );
	m_pClassItems_Red[6]->LoadControlSettings( "resource/UI/disguise_menu/medic_red.res" );
	m_pClassItems_Red[7]->LoadControlSettings( "resource/UI/disguise_menu/sniper_red.res" );
	m_pClassItems_Red[8]->LoadControlSettings( "resource/UI/disguise_menu/spy_red.res" );

	m_pClassItems_Blue[0]->LoadControlSettings( "resource/UI/disguise_menu/scout_blue.res" );
	m_pClassItems_Blue[1]->LoadControlSettings( "resource/UI/disguise_menu/soldier_blue.res" );
	m_pClassItems_Blue[2]->LoadControlSettings( "resource/UI/disguise_menu/pyro_blue.res" );
	m_pClassItems_Blue[3]->LoadControlSettings( "resource/UI/disguise_menu/demoman_blue.res" );
	m_pClassItems_Blue[4]->LoadControlSettings( "resource/UI/disguise_menu/heavy_blue.res" );
	m_pClassItems_Blue[5]->LoadControlSettings( "resource/UI/disguise_menu/engineer_blue.res" );
	m_pClassItems_Blue[6]->LoadControlSettings( "resource/UI/disguise_menu/medic_blue.res" );
	m_pClassItems_Blue[7]->LoadControlSettings( "resource/UI/disguise_menu/sniper_blue.res" );
	m_pClassItems_Blue[8]->LoadControlSettings( "resource/UI/disguise_menu/spy_blue.res" );


	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudMenuSpyDisguise::ShouldDraw( void )
{
	CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return false;

	CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();

	if ( !pWpn )
		return false;

	// Don't show the menu for first person spectator
	if ( pPlayer != pWpn->GetOwner() )
		return false;

	if ( pPlayer->m_Shared.InCond( TF_COND_TAUNTING ) )
		return false;

	return ( pWpn->GetWeaponID() == TF_WEAPON_PDA_SPY );
}

//-----------------------------------------------------------------------------
// Purpose: Keyboard input hook. Return 0 if handled
//-----------------------------------------------------------------------------
int	CHudMenuSpyDisguise::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( !ShouldDraw() )
	{
		return 1;
	}

	if ( !down )
	{
		return 1;
	}

	// menu classes are not in the same order as the defines
	static int iRemapKeyToClass[9] = 
	{
		TF_CLASS_SCOUT,
		TF_CLASS_SOLDIER,
		TF_CLASS_PYRO,
		TF_CLASS_DEMOMAN,
		TF_CLASS_HEAVYWEAPONS,
		TF_CLASS_ENGINEER,
		TF_CLASS_MEDIC,
		TF_CLASS_SNIPER,
		TF_CLASS_SPY
	};

	switch( keynum )
	{
	case KEY_1:
	case KEY_2:
	case KEY_3:
	case KEY_4:
	case KEY_5:
	case KEY_6:
	case KEY_7:
	case KEY_8:
	case KEY_9:
		{
			int iClass = iRemapKeyToClass[ keynum - KEY_1 ];
			int iTeam = ( m_iShowingTeam == TF_TEAM_BLUE ) ? 1 : 0;

			CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( pPlayer )
			{
				char szCmd[64];
				Q_snprintf( szCmd, sizeof(szCmd), "disguise %d %d; lastinv", iClass, iTeam );
				engine->ExecuteClientCmd( szCmd );
			}
		}
		return 0;

	case KEY_MINUS:
		{
		// flip the teams
		m_iShowingTeam = ( m_iShowingTeam == TF_TEAM_BLUE ) ? TF_TEAM_RED : TF_TEAM_BLUE;

		// show / hide the class items
		bool bShowBlue = ( m_iShowingTeam == TF_TEAM_BLUE );

		for ( int i=0; i<9; i++ )
		{
			m_pClassItems_Red[i]->SetVisible( !bShowBlue );
			m_pClassItems_Blue[i]->SetVisible( bShowBlue );
		}
		return 0;
		}
	case KEY_0:
		// cancel, close the menu
		engine->ExecuteClientCmd( "lastinv" );
		return 0;

	default:
		return 1;	// key not handled
	}
	

	return 1;	// key not handled
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuSpyDisguise::FireGameEvent( IGameEvent *event )
{
	const char * type = event->GetName();

	if ( Q_strcmp(type, "spy_pda_reset") == 0 )
	{
		CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pPlayer )
		{
			bool bShowBlue = ( pPlayer->GetTeamNumber() == TF_TEAM_RED );

			for ( int i=0; i<9; i++ )
			{
				m_pClassItems_Red[i]->SetVisible( !bShowBlue );
				m_pClassItems_Blue[i]->SetVisible( bShowBlue );
			}

			m_iShowingTeam = ( bShowBlue ) ? TF_TEAM_BLUE : TF_TEAM_RED;
		}
	}
	else
	{
		CHudElement::FireGameEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuSpyDisguise::SetVisible( bool state )
{
	if ( state == true )
	{
		// set the %lastinv% dialog var to our binding
		const char *key = engine->Key_LookupBinding( "lastinv" );
		if ( !key )
		{
			key = "< not bound >";
		}

		SetDialogVariable( "lastinv", key );
	}

	BaseClass::SetVisible( state );
}