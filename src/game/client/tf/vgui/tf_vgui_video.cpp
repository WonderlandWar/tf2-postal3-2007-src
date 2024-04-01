//====== Copyright © 1996-2007, Valve Corporation, All rights reserved. =======
//
// Purpose: VGUI panel which can play back video, in-engine
//
//=============================================================================

#include "cbase.h"
#include <vgui/IVGui.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include "vgui_video.h"
#include "tf_vgui_video.h"
#include "engine/ienginesound.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


DECLARE_BUILD_FACTORY( CTFVideoPanel );

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
CTFVideoPanel::CTFVideoPanel( vgui::Panel *parent, const char *panelName ) : VideoPanel( 0, 0, 50, 50 )
{
	SetParent( parent );
	SetProportional( true );
	SetKeyBoardInputEnabled( false );

	m_flDelay = 0.0f;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
CTFVideoPanel::~CTFVideoPanel()
{
	ReleaseVideo();
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CTFVideoPanel::ReleaseVideo()
{
	// Destroy any previously allocated video
	if ( m_BIKHandle != BIKHANDLE_INVALID )
	{
		bik->DestroyMaterial( m_BIKHandle );
		m_BIKHandle = BIKHANDLE_INVALID;
	}
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CTFVideoPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	SetExitCommand( inResourceData->GetString( "command", "" ) );
	m_flDelay = inResourceData->GetFloat( "delay", 0.0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFVideoPanel::GetPanelPos( int &xpos, int &ypos )
{
	vgui::ipanel()->GetAbsPos( GetVPanel(), xpos, ypos );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFVideoPanel::OnVideoOver()
{
	BaseClass::OnVideoOver();
	PostMessage( GetParent(), new KeyValues( "IntroFinished" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFVideoPanel::OnClose()
{
	// Fire an exit command if we're asked to do so
	if ( m_szExitCommand[0] )
	{
		engine->ClientCmd( m_szExitCommand );
	}

	// intentionally skipping VideoPanel::OnClose()
	EditablePanel::OnClose();

	SetVisible( false );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFVideoPanel::Shutdown()
{
	OnClose();
	ReleaseVideo();
}