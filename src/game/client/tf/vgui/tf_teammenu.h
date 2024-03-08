//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_TEAMMENU_H
#define TF_TEAMMENU_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_controls.h"
#include <teammenu.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFTeamButton : public CTFButton
{
private:
	DECLARE_CLASS_SIMPLE( CTFTeamButton, CTFButton );

public:
	CTFTeamButton( vgui::Panel *parent, const char *panelName );

	void ApplySchemeSettings( vgui::IScheme *pScheme );
	
private:

	CPanelAnimationVar( Color, m_clrDefaultBG, "DefaultBGColor", "Button.BgColor" );
	CPanelAnimationVar( Color, m_clrArmedBG, "ArmedBGColor", "Button.ArmedBgColor" );
	CPanelAnimationVar( Color, m_clrDepressedBG, "DepressedBGColor", "Button.DepressedBgColor" );

};

//-----------------------------------------------------------------------------
// Purpose: Displays the team menu
//-----------------------------------------------------------------------------
class CTFTeamMenu : public CTeamMenu
{
private:
	DECLARE_CLASS_SIMPLE( CTFTeamMenu, CTeamMenu );
		
public:
	CTFTeamMenu( IViewPort *pViewPort );
	~CTFTeamMenu();

	void Update();
	void ShowPanel( bool bShow );

	CON_COMMAND_MEMBER_F( CTFTeamMenu, "join_team", Join_Team, "Send a jointeam command", 0 );

protected:
	virtual void OnKeyCodePressed( vgui::KeyCode code );

	// command callbacks
	virtual void OnCommand( const char *command );

	virtual void LoadMapPage( const char *mapName );

	virtual void OnTick( void );

private:

	CTFTeamButton	*m_pBlueTeamButton;
	CTFTeamButton	*m_pRedTeamButton;
	CTFTeamButton	*m_pAutoTeamButton;
	CTFTeamButton	*m_pSpecTeamButton;

#ifdef _X360
	CTFFooter		*m_pFooter;
#else
	CTFButton		*m_pCancelButton;
#endif

	bool m_bRedDisabled;
	bool m_bBlueDisabled;


private:
	enum { NUM_TEAMS = 3 };

	ButtonCode_t m_iTeamMenuKey;
};

#endif // TF_TEAMMENU_H
