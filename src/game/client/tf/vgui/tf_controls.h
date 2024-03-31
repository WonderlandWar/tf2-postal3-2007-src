//========= Copyright © 1996-2007, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_CONTROLS_H
#define TF_CONTROLS_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/IScheme.h>
#include <vgui/KeyCode.h>
#include <KeyValues.h>
#include <vgui/IVGui.h>
#include <vgui_controls/ScrollBar.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/RichText.h>
#include "tf_imagepanel.h"
#include <vgui_controls/ImagePanel.h>


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFButton : public vgui::Button
{
public:
	DECLARE_CLASS_SIMPLE( CTFButton, vgui::Button );

	CTFButton( vgui::Panel *parent, const char *name, const char *text );
	CTFButton( vgui::Panel *parent, const char *name, const wchar_t *wszText );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

private:

	CPanelAnimationVar( vgui::HFont, m_hFont, "font", "Default" );
	CPanelAnimationVar( Color, m_clrText, "fgcolor", "Button.TextColor" );
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFLabel : public vgui::Label
{
public:
	DECLARE_CLASS_SIMPLE( CTFLabel, vgui::Label );

	CTFLabel( vgui::Panel *parent, const char *panelName, const char *text );
	CTFLabel( vgui::Panel *parent, const char *panelName, const wchar_t *wszText );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

private:

	CPanelAnimationVar( Color, m_clrText, "fgcolor", "Label.TextColor" );
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFRichText : public vgui::RichText
{
public:
	DECLARE_CLASS_SIMPLE( CTFRichText, vgui::RichText );

	CTFRichText( vgui::Panel *parent, const char *panelName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

private:
	
	CPanelAnimationVar( vgui::HFont, m_hFont, "font", "Default" );
	CPanelAnimationVar( Color, m_clrText, "fgcolor", "RichText.TextColor" );
};

#endif // TF_CONTROLS_H
