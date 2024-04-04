//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/RichText.h>
#include <game/client/iviewport.h>
#include <vgui/ILocalize.h>
#include <KeyValues.h>
#include <filesystem.h>
#include "IGameUIFuncs.h" // for key bindings

#ifdef _WIN32
#include "winerror.h"
#endif
#include "ixboxsystem.h"
#include "tf_gamerules.h"
#include "tf_controls.h"
#include "tf_shareddefs.h"
#include "tf_mapinfomenu.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFMapInfoMenu::CTFMapInfoMenu( IViewPort *pViewPort ) : Frame( NULL, PANEL_MAPINFO )
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

	m_pTitle = new CTFLabel( this, "MapInfoTitle", " " );

	m_pContinue = new CTFButton( this, "MapInfoContinue", "#TF_Continue" );
	m_pBack = new CTFButton( this, "MapInfoBack", "#TF_Back" );
	m_pIntro = new CTFButton( this, "MapInfoWatchIntro", "#TF_WatchIntro" );

	// info window about this map
	m_pMapInfo = new CTFRichText( this, "MapInfoText" );
	m_pMapImage = new ImagePanel( this, "MapImage" );

	m_szMapName[0] = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFMapInfoMenu::~CTFMapInfoMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	LoadControlSettings("Resource/UI/MapInfoMenu.res");
	SetMapTitle();
	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::ShowPanel( bool bShow )
{
	if ( IsVisible() == bShow )
		return;

	if ( bShow )
	{
		Activate();
		SetMouseInputEnabled( true );
		SetDialogVariable( "gamemode", g_pVGuiLocalize->Find( g_pGameRules->GetGameTypeName() ) );
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
bool CTFMapInfoMenu::CheckForIntroMovie()
{
	char mapname[MAX_MAP_NAME];

	Q_FileBase( engine->GetLevelName(), mapname, sizeof( mapname ) );
	Q_strlower( mapname );

#ifdef _X360
	// need to remove the .360 extension on the end of the map name
	char *pExt = Q_stristr( mapname, ".360" );
	if ( pExt )
	{
		*pExt = '\0';
	}
#endif

	static char strFullpath[MAX_PATH];
	Q_strncpy( strFullpath, "media/", MAX_PATH );	// Assume we must play out of the media directory
	Q_strncat( strFullpath, mapname, MAX_PATH );
	Q_strncat( strFullpath, ".bik", MAX_PATH );		// Assume we're a .bik extension type

	if ( g_pFullFileSystem->FileExists( strFullpath ) )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::CheckIntroButton()
{
	if ( !IsVisible() )
		return;

	if ( CheckForIntroMovie() )
	{
		if ( m_pIntro && !m_pIntro->IsVisible() )
		{				
			m_pIntro->SetVisible( true );
		}
	}
	else 
	{
		if ( m_pIntro && m_pIntro->IsVisible() )
		{
			m_pIntro->SetVisible( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::OnCommand( const char *command )
{
	if ( !Q_strcmp( command, "back" ) )
	{
		 // only want to go back to the Welcome menu if we're not already on a team
		if ( !IsX360() && ( GetLocalPlayerTeam() == TEAM_UNASSIGNED ) )
		{
			m_pViewPort->ShowPanel( this, false );
			m_pViewPort->ShowPanel( PANEL_INFO, true );
		}
	}
	else if ( !Q_strcmp( command, "continue" ) )
	{
		m_pViewPort->ShowPanel( this, false );
		m_pViewPort->ShowPanel( PANEL_TEAM, true );

	}
	else if ( !Q_strcmp( command, "intro" ) )
	{
		m_pViewPort->ShowPanel( this, false );

		if ( CheckForIntroMovie() )
		{
			m_pViewPort->ShowPanel( PANEL_INTRO, true );
		}
		else
		{
			m_pViewPort->ShowPanel( PANEL_TEAM, true );
		}
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::Update()
{ 
	InvalidateLayout( false, true );
}

//-----------------------------------------------------------------------------
// Purpose: chooses and loads the text page to display that describes mapName map
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::LoadMapPage( const char *mapName )
{
	// load the map image (if it exists for the current map)
	char szMapImage[ MAX_PATH ];
	Q_snprintf( szMapImage, sizeof( szMapImage ), "VGUI/maps/menu_photos_%s", mapName );
	Q_strlower( szMapImage );

	IMaterial *pMapMaterial = materials->FindMaterial( szMapImage, TEXTURE_GROUP_VGUI, false );
	if ( pMapMaterial && !IsErrorMaterial( pMapMaterial ) )
	{
		if ( m_pMapImage )
		{
			if ( !m_pMapImage->IsVisible() )
			{
				m_pMapImage->SetVisible( true );
			}

			// take off the vgui/ at the beginning when we set the image
			Q_snprintf( szMapImage, sizeof( szMapImage ), "maps/menu_photos_%s", mapName );
			Q_strlower( szMapImage );
			
			m_pMapImage->SetImage( szMapImage );
		}
	}
	else
	{
		if ( m_pMapImage && m_pMapImage->IsVisible() )
		{
			m_pMapImage->SetVisible( false );
			
			// take off the vgui/ at the beginning when we set the image
			Q_snprintf( szMapImage, sizeof( szMapImage ), "maps/menu_photos_%s", mapName );
			Q_strlower( szMapImage );
			
			m_pMapImage->SetImage( szMapImage );
		}
	}

	// load the map description files
	char mapRES[ MAX_PATH ];

	char uilanguage[ 64 ];
	engine->GetUILanguage( uilanguage, sizeof( uilanguage ) );
	V_snprintf( mapRES, sizeof(mapRES), "resource/maphtml/%s_%s.html", mapName, uilanguage );
	
	if ( g_pFullFileSystem->FileExists( mapRES )
		|| (V_snprintf( mapRES, sizeof(mapRES), "resource/maphtml/%s_english.html", mapName ),
			g_pFullFileSystem->FileExists( mapRES ) ) )
	{
		char pPathData[MAX_PATH];
		char localURL[MAX_PATH+7];
		V_strncpy( localURL, "file://", sizeof( localURL ) );
		g_pFullFileSystem->GetLocalPath( mapRES, pPathData, sizeof(mapRES) );
		V_strncat( localURL, pPathData, sizeof( localURL ), -1 );
		g_pFullFileSystem->GetLocalCopy( pPathData );
		m_pMapInfo->SetVisible( false );
		return;
	}

	m_pMapInfo->SetVisible( true );

	Q_snprintf( mapRES, sizeof( mapRES ), "maps/%s.txt", mapName );

	// if no map specific description exists, load default text
	if( !g_pFullFileSystem->FileExists( mapRES ) )
	{
		const char *pszDefault = "maps/default.txt";

		if ( TFGameRules() )
		{
			if ( TFGameRules()->GetGameType() == TF_GAMETYPE_CTF )
			{
				pszDefault = "maps/default_ctf.txt";
			}
			else if ( TFGameRules()->GetGameType() == TF_GAMETYPE_CP )
			{
				pszDefault = "maps/default_cp.txt";
			}
		}

		if ( g_pFullFileSystem->FileExists( pszDefault ) )
		{
			Q_snprintf ( mapRES, sizeof( mapRES ), pszDefault );
		}
		else
		{
			m_pMapInfo->SetText( "" );

			// we haven't loaded a valid map image for the current map
			if ( m_pMapImage && !m_pMapImage->IsVisible() )
			{
				if ( m_pMapInfo )
				{
					m_pMapInfo->SetWide( m_pMapInfo->GetWide() + ( m_pMapImage->GetWide() * 0.75 ) ); // add in the extra space the images would have taken 
				}
			}

			return; 
		}
	}

	FileHandle_t f = g_pFullFileSystem->Open( mapRES, "r" );

	// read into a memory block
	int fileSize = g_pFullFileSystem->Size(f);
	int dataSize = fileSize + sizeof( wchar_t );
	if ( dataSize % 2 )
		++dataSize;
	wchar_t *memBlock = (wchar_t *)malloc(dataSize);
	memset( memBlock, 0x0, dataSize);
	int bytesRead = g_pFullFileSystem->Read(memBlock, fileSize, f);
	if ( bytesRead < fileSize )
	{
		// NULL-terminate based on the length read in, since Read() can transform \r\n to \n and
		// return fewer bytes than we were expecting.
		char *data = reinterpret_cast<char *>( memBlock );
		data[ bytesRead ] = 0;
		data[ bytesRead+1 ] = 0;
	}

	// null-terminate the stream (redundant, since we memset & then trimmed the transformed buffer already)
	memBlock[dataSize / sizeof(wchar_t) - 1] = 0x0000;

	// check the first character, make sure this a little-endian unicode file

#if defined( _X360 )
	if ( memBlock[0] != 0xFFFE )
#else
	if ( memBlock[0] != 0xFEFF )
#endif
	{
		// its a ascii char file
		m_pMapInfo->SetText( reinterpret_cast<char *>( memBlock ) );
	}
	else
	{
		// ensure little-endian unicode reads correctly on all platforms
		CByteswap byteSwap;
		byteSwap.SetTargetBigEndian( false );
		byteSwap.SwapBufferToTargetEndian( memBlock, memBlock, dataSize/sizeof(wchar_t) );

		m_pMapInfo->SetText( memBlock+1 );
	}
	// go back to the top of the text buffer
	m_pMapInfo->GotoTextStart();

	g_pFullFileSystem->Close( f );
	free(memBlock);

	// we haven't loaded a valid map image for the current map
	if ( m_pMapImage && !m_pMapImage->IsVisible() )
	{
		if ( m_pMapInfo )
		{
			m_pMapInfo->SetWide( m_pMapInfo->GetWide() + ( m_pMapImage->GetWide() * 0.75 ) ); // add in the extra space the images would have taken 
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::SetMapTitle()
{
	const char *pszSrc;
	// we haven't found a "friendly" map name, so let's just clean up what we have
	if ( !Q_strncmp( m_szMapName, "CP_", 3 ) ||
		 !Q_strncmp( m_szMapName, "TC_", 3 ) )
	{
		pszSrc = m_szMapName + 3;
	}
	else if ( !Q_strncmp( m_szMapName, "CTF_", 4 ) )
	{
		pszSrc = m_szMapName + 4;
	}
	else
	{
		pszSrc = m_szMapName;
	}

	SetDialogVariable( "mapname", pszSrc );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::OnKeyCodePressed( KeyCode code )
{
	if ( code == KEY_XBUTTON_A )
	{
		OnCommand( "continue" );
	}
	else if ( code == KEY_XBUTTON_Y )
	{
		OnCommand( "intro" );
	}
	else if( code == KEY_XBUTTON_UP || code == KEY_XSTICK1_UP )
	{
		// Scroll class info text up
		if ( m_pMapInfo )
		{
			PostMessage( m_pMapInfo, new KeyValues("MoveScrollBarDirect", "delta", 1) );
		}
	}
	else if( code == KEY_XBUTTON_DOWN || code == KEY_XSTICK1_DOWN )
	{
		// Scroll class info text up
		if ( m_pMapInfo )
		{
			PostMessage( m_pMapInfo, new KeyValues("MoveScrollBarDirect", "delta", -1) );
		}
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

void CTFMapInfoMenu::PerformLayout( void )
{
	char mapname[32];

	BaseClass::PerformLayout();
	V_FileBase( engine->GetLevelName(), mapname, sizeof( mapname ) );
	V_strncpy( m_szMapName, mapname, sizeof( m_szMapName ) );
	strupr( m_szMapName );

	CTFMapInfoMenu::LoadMapPage( mapname );
	SetMapTitle();

	if ( m_pContinue )
		m_pContinue->RequestFocus( 0 );
}