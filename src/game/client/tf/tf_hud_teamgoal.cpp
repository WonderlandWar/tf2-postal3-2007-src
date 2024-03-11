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
#include <vgui_controls/Label.h>
#include <vgui_controls/EditablePanel.h>
#include "tf_imagepanel.h"
#include "tf_gamerules.h"
#include "c_tf_team.h"
#include "tf_hud_statpanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudTeamGoal : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudTeamGoal, EditablePanel );

public:
	CHudTeamGoal( const char *pElementName );

	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );

private:
	Label			*m_pGoalLabel;
};

DECLARE_HUDELEMENT( CHudTeamGoal );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTeamGoal::CHudTeamGoal( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudTeamGoal" )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTeamGoal::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudTeamGoal.res" );

	BaseClass::ApplySchemeSettings( pScheme );

	m_pGoalLabel = dynamic_cast<Label *>( FindChildByName("GoalLabel") );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudTeamGoal::ShouldDraw( void )
{
	// TFP3:
	// TODO: Is this correct?
	CTFStatPanel *pStatPanel = GET_HUDELEMENT( CTFStatPanel);
	if ( pStatPanel && pStatPanel->IsVisible() )
		return false;

	if ( !m_pGoalLabel )
		return false;

	if ( TFGameRules() )
	{
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pPlayer && pPlayer->IsAlive() && pPlayer->GetTeamNumber() >= FIRST_GAME_TEAM )
		{
			const char *pszGoal = TFGameRules()->GetTeamGoalString( pPlayer->GetTeamNumber() );
			if ( pszGoal && pszGoal[0] && CHudElement::ShouldDraw() )
			{
				if ( !IsVisible() )
				{
					wchar_t *pszLocalizedGoal = g_pVGuiLocalize->Find( pszGoal );
					if ( pszLocalizedGoal )
					{
						m_pGoalLabel->SetText( pszLocalizedGoal );
					}
					else
					{
						m_pGoalLabel->SetText( pszGoal );
					}
				}

				return true;
			}
		}
	}

	return false;
}

const char *pszTeamRoleIcons[NUM_TEAM_ROLES] =
{
	"../hud/hud_icon_capture",		// TEAM_ROLE_NONE = 0,
	"../hud/hud_icon_defend",		// TEAM_ROLE_DEFENDERS,
	"../hud/hud_icon_attack",		// TEAM_ROLE_ATTACKERS,
};

const char *pszTeamRoleSwitch[NUM_TEAM_ROLES] =
{
	" ",							// TEAM_ROLE_NONE = 0,
	"#TF_teamswitch_defenders",		// TEAM_ROLE_DEFENDERS,
	"#TF_teamswitch_attackers",		// TEAM_ROLE_ATTACKERS,
};
