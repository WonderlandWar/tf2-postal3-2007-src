//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_MENU_SPY_DISGUISE_H
#define TF_HUD_MENU_SPY_DISGUISE_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>

using namespace vgui;

#define ALL_BUILDINGS	-1

class CHudMenuSpyDisguise : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudMenuSpyDisguise, EditablePanel );

public:
	CHudMenuSpyDisguise( const char *pElementName );

	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );

	virtual void	FireGameEvent( IGameEvent *event );

	virtual void	SetVisible( bool state );

	int	HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

private:
	EditablePanel *m_pClassItems_Red[9];
	EditablePanel *m_pClassItems_Blue[9];

	int m_iShowingTeam;
};

#endif	// TF_HUD_MENU_SPY_DISGUISE_H