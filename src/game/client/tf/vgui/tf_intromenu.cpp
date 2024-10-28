//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include <KeyValues.h>
#include <vgui/IVGUI.h>
#include <vgui/ISurface.h>
#include <FileSystem.h>
#include <vgui_controls/AnimationController.h>
#include "iclientmode.h"
#include "clientmode_shared.h"
#include "shareddefs.h"
#include "tf_shareddefs.h"
#include "tf_controls.h"
#include "tf_gamerules.h"
#include "winerror.h"
#include "ixboxsystem.h"
#include "intromenu.h"
#include "tf_intromenu.h"

// used to determine the action the intro menu should take when OnTick handles a think for us
enum
{
	INTRO_NONE,
	INTRO_STARTVIDEO,
	INTRO_BACK,
	INTRO_CONTINUE,
};

using namespace vgui;

char* ReadAndAllocStringValue( KeyValues *pSub, const char *pName, const char *pFilename = NULL );

// sort function for the list of captions that we're going to show
int CaptionsSort( CVideoCaption* const *p1, CVideoCaption* const *p2 )
{
	// check the start time
	if ( (*p2)->m_flStartTime < (*p1)->m_flStartTime )
	{
		return 1;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFIntroMenu::CTFIntroMenu( IViewPort *pViewPort ) : CIntroMenu( pViewPort )
{
	LoadControlSettings( "Resource/UI/IntroMenu.res" );

	m_flThink = -1;
	m_pVideo = 0;
	m_pModel = 0;
	m_iAction = INTRO_NONE;

	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFIntroMenu::~CTFIntroMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFIntroMenu::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	
	m_pVideo = new CTFVideoPanel( this, "VideoPanel" );
	m_pModel = new CModelPanel( this, "MenuBG" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFIntroMenu::SetNextThink( float flActionThink, int iAction )
{
	m_flThink = flActionThink;
	m_iAction = iAction;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFIntroMenu::OnTick()
{
	// do we have anything special to do?
	if ( m_flThink > 0 && m_flThink < gpGlobals->curtime )
	{
		if ( m_iAction == INTRO_STARTVIDEO )
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

			if ( m_pVideo )
			{
				m_pVideo->Activate();
				m_pVideo->BeginPlayback( strFullpath );
				m_pVideo->MoveToFront();
			}
		}
		else if ( m_iAction == INTRO_BACK )
		{
			m_pViewPort->ShowPanel( this, false );
			m_pViewPort->ShowPanel( PANEL_MAPINFO, true );
		}
		else if ( m_iAction == INTRO_CONTINUE )
		{
			m_pViewPort->ShowPanel( this, false );
			m_pViewPort->ShowPanel( PANEL_TEAM, true );
		}

		// reset our think
		SetNextThink( -1, INTRO_NONE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFIntroMenu::ShowPanel( bool bShow )
{
	if ( BaseClass::IsVisible() == bShow )
		return;

	// reset our think
	SetNextThink( -1, INTRO_NONE );

	if ( bShow )
	{
		Activate();
		SetMouseInputEnabled( true );

		if ( m_pVideo )
		{
			m_pVideo->Shutdown(); // make sure we're not currently running
			SetNextThink( gpGlobals->curtime + m_pVideo->GetDelay(), INTRO_STARTVIDEO );
		}

		if ( m_pModel )
		{
			m_pModel->SetPanelDirty();
		}
	}
	else
	{
		if ( m_pVideo )
			m_pVideo->Shutdown();

		SetVisible( false );
		SetMouseInputEnabled( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFIntroMenu::OnIntroFinished( void )
{
	float flTime = gpGlobals->curtime;

	if ( m_pVideo )
		m_pVideo->Shutdown();

	if ( m_pModel && m_pModel->SetSequence( "Up" ) )
	{
		// wait for the model sequence to finish before going to the next menu
		flTime = gpGlobals->curtime + 0.35;
	}


	SetNextThink( flTime, INTRO_CONTINUE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFIntroMenu::OnCommand( const char *command )
{
	if ( !Q_strcmp( command, "back" ) )
	{
		float flTime = gpGlobals->curtime;

		if ( m_pVideo )
			m_pVideo->Shutdown();

		// try to play the screenup sequence
		if ( m_pModel && m_pModel->SetSequence( "Up" ) )
		{
			flTime = gpGlobals->curtime + 0.35f;
		}

		// wait for the model sequence to finish before going back to the mapinfo menu
		SetNextThink( flTime, INTRO_BACK );
	}
	else if ( !Q_strcmp( command, "skip" ) )
	{
		if ( m_pVideo )
			m_pVideo->Shutdown();

		// continue right now
		SetNextThink( gpGlobals->curtime, INTRO_CONTINUE );
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}