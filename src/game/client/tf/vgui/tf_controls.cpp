//========= Copyright © 1996-2007, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"

#include <vgui_controls/ScrollBarSlider.h>
#include "vgui/ILocalize.h"
#include "vgui/ISurface.h"
#include "tf_controls.h"


using namespace vgui;

DECLARE_BUILD_FACTORY_DEFAULT_TEXT( CTFButton, CTFButton );
DECLARE_BUILD_FACTORY_DEFAULT_TEXT( CTFLabel, CTFLabel );
DECLARE_BUILD_FACTORY( CTFRichText );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFButton::CTFButton( Panel *parent, const char *name, const char *text ) : Button( parent, name, text )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFButton::CTFButton( Panel *parent, const char *name, const wchar_t *wszText ) : Button( parent, name, wszText )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFButton::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetFont( m_hFont );
	SetFgColor( m_clrText );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFLabel::CTFLabel( Panel *parent, const char *name, const char *text ) : Label( parent, name, text )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFLabel::CTFLabel( Panel *parent, const char *name, const wchar_t *wszText ) : Label( parent, name, wszText )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLabel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetFgColor( m_clrText );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFRichText::CTFRichText( Panel *parent, const char *name ) : RichText( parent, name )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRichText::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetFont( m_hFont );
	SetFgColor( m_clrText );

	SetBorder( pScheme->GetBorder( "NoBorder" ) );
	SetBgColor( pScheme->GetColor( "Blank", Color( 0,0,0,0 ) ) );
	SetPanelInteractive( false );
	SetUnusedScrollbarInvisible( true );
}
