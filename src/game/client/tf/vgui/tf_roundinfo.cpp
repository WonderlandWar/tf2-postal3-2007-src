//========= Copyright © 1996-2007, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/Frame.h>
#include <game/client/iviewport.h>
#include <KeyValues.h>
#include <filesystem.h>
#include "materialsystem/imaterialvar.h"
#include "IGameUIFuncs.h" // for key bindings

#include "tf_controls.h"
#include "tf_imagepanel.h"
#include "c_team_objectiveresource.h"
#include "c_tf_objective_resource.h"
#include "c_tf_player.h"

#include "tf_shareddefs.h"
#include "tf_roundinfo.h"


#include "vgui/ISurface.h"
#include <vgui/ILocalize.h>
#include <vgui/IVGui.h>
#include "engine/IEngineSound.h"

using namespace vgui;

const char *GetMapDisplayName( const char *mapName );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFRoundInfo::CTFRoundInfo( IViewPort *pViewPort ) : Frame( NULL, PANEL_ROUNDINFO )
{
	m_pViewPort = pViewPort;

	// load the new scheme early!!
	SetScheme( "ClientScheme" );

	SetTitleBarVisible( false );
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	SetCloseButtonVisible( false );
	SetSizeable( false );
	SetMoveable( false );
	SetProportional( true );
	SetVisible( false );
	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );

	m_pTitle = new CTFLabel( this, "RoundTitle", " " );
	m_pMapImage = new ImagePanel( this, "MapImage" );
	m_pStateImage = new ImagePanel( this, "StateImage" );
	m_pRoundImage = new ImagePanel( this, "RoundImage" );

#ifdef _X360
	m_pFooter = new CTFFooter( this, "Footer" );
#else
	m_pContinue = new CTFButton( this, "RoundContinue", "#TF_Continue" );
#endif

	m_szTitle[0] = 0;
	m_szMapImage[0] = 0;
	m_szRedImage[0] = 0;
	m_szBlueImage[0] = 0;
	
	ListenForGameEvent( "game_newmap" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRoundInfo::PerformLayout()
{
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRoundInfo::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	LoadControlSettings( "Resource/UI/RoundInfo.res" );

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRoundInfo::ShowPanel( bool bShow )
{
	if ( IsVisible() == bShow )
		return;

	if ( bShow )
	{
		// look for the textures we want to use and don't show the roundinfo panel if any are missing
		char temp[255];
		Q_snprintf( temp, sizeof( temp ), "VGUI/%s", m_szMapImage );
		IMaterial *pMapMaterial = materials->FindMaterial( temp, TEXTURE_GROUP_VGUI, false );
		
		IMaterial *pTeamMaterial = NULL;
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pPlayer )
		{
			if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
				Q_snprintf( temp, sizeof( temp ), "VGUI/%s", m_szBlueImage );
			else
				Q_snprintf( temp, sizeof( temp ), "VGUI/%s", m_szRedImage );
			
			pTeamMaterial = materials->FindMaterial( temp, TEXTURE_GROUP_VGUI, false );
		}
		
		Q_snprintf( temp, sizeof( temp ), "VGUI/%s", m_szStateImage );
		IMaterial *pStateMaterial = materials->FindMaterial( temp, TEXTURE_GROUP_VGUI, false );
	
		// are we missing any of the images we want to show?
		if ( pMapMaterial && !IsErrorMaterial( pMapMaterial ) && 
			pTeamMaterial && !IsErrorMaterial( pTeamMaterial ) &&
			pStateMaterial && !IsErrorMaterial( pStateMaterial ) )
		{
			Activate();
			SetMouseInputEnabled( true );

			m_iRoundInfoKey = gameuifuncs->GetButtonCodeForBind( "showroundinfo" );
		}
		else
		{
			SetVisible( false );
			SetMouseInputEnabled( false );
		}
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRoundInfo::OnCommand( const char *command )
{
	if ( !Q_strcmp( command, "continue" ) )
	{
		m_pViewPort->ShowPanel( this, false );
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRoundInfo::UpdateImage( ImagePanel *pImagePanel, const char *pszImageName )
{
	if ( pImagePanel && ( Q_strlen( pszImageName ) > 0 ) )
	{
		char szTemp[255];
		Q_snprintf( szTemp, sizeof( szTemp ), "VGUI/%s", pszImageName );

		IMaterial *pTemp = materials->FindMaterial( szTemp, TEXTURE_GROUP_VGUI, false );
		if ( pTemp && !IsErrorMaterial( pTemp ) )
		{
			pImagePanel->SetImage( pszImageName );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRoundInfo::Update()
{
	char szMapName[MAX_MAP_NAME];
	Q_FileBase( engine->GetLevelName(), szMapName, sizeof(szMapName) );
	Q_strlower( szMapName );

	SetDialogVariable( "mapname", GetMapDisplayName( szMapName ) );

	if ( m_pMapImage )
	{
		char temp[255];
		Q_snprintf( temp, sizeof(temp), "../overviews/%s", szMapName );
		Q_strncpy(  m_szMapImage, temp, sizeof( m_szMapImage ) );

		UpdateImage( m_pMapImage, m_szMapImage );
	}

	if ( m_pRoundImage )
	{
		const char *pszTeamImage;
		
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pPlayer && pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
		{
			pszTeamImage = m_szBlueImage;
		}
		else
		{
			pszTeamImage = m_szRedImage;
		}

		UpdateImage( m_pRoundImage, pszTeamImage );
	}

	if ( m_pStateImage )
	{
		UpdateImage( m_pStateImage, m_szStateImage );
	}

#ifndef _X360
	if ( m_pContinue )
	{
		m_pContinue->RequestFocus();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRoundInfo::OnKeyCodeReleased( KeyCode code )
{
	if ( m_iRoundInfoKey == BUTTON_CODE_INVALID || m_iRoundInfoKey != code )
		BaseClass::OnKeyCodeReleased( code );
	else
		m_pViewPort->ShowPanel( this, false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRoundInfo::OnKeyCodePressed( KeyCode code )
{
	if( code == KEY_SPACE ||
		code == KEY_ENTER ||
		code == KEY_XBUTTON_A ||
		code == KEY_XBUTTON_B )
	{
		OnCommand( "continue" );
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRoundInfo::SetData( KeyValues *data )
{
	SetData( data->GetString("title"), data->GetString("red"), data->GetString("blue"), data->GetString("state") );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRoundInfo::SetData( const char *pszTitle, const char *pszRedImage, const char *pszBlueImage, const char *pszStateImage )
{
	V_strncpy( m_szTitle, pszTitle, 255 );
	if ( strlen( pszRedImage ) <= 0 )
	{
		m_szRedImage[0] = 0;
	}
	else
	{	
		V_snprintf( m_szRedImage, MAX_ROUND_IMAGE_NAME, "../overviews/%s", pszRedImage );
	}

	if ( strlen( pszBlueImage ) <= 0 )
	{	
		m_szBlueImage[0] = 0;
	}
	else
	{	
		V_snprintf( m_szBlueImage, MAX_ROUND_IMAGE_NAME, "../overviews/%s", pszBlueImage );
	}

	if ( strlen( pszStateImage ) <= 0 )
	{
		m_szStateImage[0] = 0;
		Update();
	}
	else
	{
	    V_snprintf( m_szStateImage, MAX_ROUND_IMAGE_NAME, "../overviews/%s", pszStateImage );
		Update();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRoundInfo::FireGameEvent( IGameEvent *event )
{
	if ( Q_strcmp( event->GetName(), "game_newmap" ) == 0 )
	{
		Update();
	}
}

void ShowRoundInfo()
{
	if ( gViewPortInterface )
		gViewPortInterface->ShowPanel( PANEL_ROUNDINFO, true );
}

ConCommand showroundinfo( "+showroundinfo", ShowRoundInfo );