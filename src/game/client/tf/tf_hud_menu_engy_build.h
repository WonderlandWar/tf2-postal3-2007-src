//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_MENU_ENGY_BUILD_H
#define TF_HUD_MENU_ENGY_BUILD_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include "iconpanel.h"
#include "tf_controls.h"

using namespace vgui;

#define ALL_BUILDINGS	-1

class CHudMenuEngyBuild : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudMenuEngyBuild, EditablePanel );

public:
	CHudMenuEngyBuild( const char *pElementName );

	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );

	virtual void	SetVisible( bool state );

	virtual void	OnTick( void );

	int	HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	int MapIndexToObjectID( int index );

private:
	EditablePanel *m_pAvailableObjects[4];
	EditablePanel *m_pAlreadyBuiltObjects[4];
	EditablePanel *m_pCantAffordObjects[4];
};

#endif	// TF_HUD_MENU_ENGY_BUILD_H