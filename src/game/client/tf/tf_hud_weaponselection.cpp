//========= Copyright © 1996-2006, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_selection.h"
#include "iclientmode.h"
#include "history_resource.h"

#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/EditablePanel.h>

#include "vgui/ILocalize.h"

#include <string.h>
#include "baseobject_shared.h"
#include "tf_imagepanel.h"
#include "c_tf_player.h"
#include "c_tf_weapon_builder.h"

#define SELECTION_TIMEOUT_THRESHOLD		2.5f	// Seconds
#define SELECTION_FADEOUT_TIME			3.0f

#define FASTSWITCH_DISPLAY_TIMEOUT		0.5f
#define FASTSWITCH_FADEOUT_TIME			0.5f

ConVar tf_weapon_select_demo_start_delay( "tf_weapon_select_demo_start_delay", "1.0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Delay after spawning to start the weapon bucket demo." );
ConVar tf_weapon_select_demo_time( "tf_weapon_select_demo_time", "0.5", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Time to pulse each weapon bucket upon spawning as a new class. 0 to turn off." );

//-----------------------------------------------------------------------------
// Purpose: tf weapon selection hud element
//-----------------------------------------------------------------------------
class CHudWeaponSelection : public CBaseHudWeaponSelection, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudWeaponSelection, vgui::Panel );

public:
	CHudWeaponSelection(const char *pElementName );

	virtual bool ShouldDraw();
	virtual void OnWeaponPickup( C_BaseCombatWeapon *pWeapon );

	virtual void CycleToNextWeapon( void );
	virtual void CycleToPrevWeapon( void );

	virtual C_BaseCombatWeapon *GetWeaponInSlot( int iSlot, int iSlotPos );
	virtual void SelectWeaponSlot( int iSlot );

	virtual C_BaseCombatWeapon	*GetSelectedWeapon( void );

	virtual void OpenSelection( void );
	virtual void HideSelection( void );

	virtual void LevelInit();

	virtual void FireGameEvent( IGameEvent *event );

protected:
	virtual void OnThink();
	virtual void PostChildPaint();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	virtual bool IsWeaponSelectable()
	{ 
		if (IsInSelectionMode())
			return true;

		return false;
	}

private:
	C_BaseCombatWeapon *FindNextWeaponInWeaponSelection(int iCurrentSlot, int iCurrentPosition);
	C_BaseCombatWeapon *FindPrevWeaponInWeaponSelection(int iCurrentSlot, int iCurrentPosition);

	int GetNumVisibleSlots();

	virtual	void SetSelectedWeapon( C_BaseCombatWeapon *pWeapon ) 
	{ 
		m_hSelectedWeapon = pWeapon;
	}

	void DrawBox(int x, int y, int wide, int tall, Color color, float normalizedAlpha, int number);
	void DrawString( wchar_t *text, int xpos, int ypos, Color col );

	CPanelAnimationVar( vgui::HFont, m_hNumberFont, "NumberFont", "HudSelectionText" );
	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "HudSelectionText" );

	CPanelAnimationVarAliasType( float, m_flSmallBoxWide, "SmallBoxWide", "32", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flSmallBoxTall, "SmallBoxTall", "21", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flLargeBoxWide, "LargeBoxWide", "108", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flLargeBoxTall, "LargeBoxTall", "72", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flBoxGap, "BoxGap", "12", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flRightMargin, "RightMargin", "0", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flSelectionNumberXPos, "SelectionNumberXPos", "4", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flSelectionNumberYPos, "SelectionNumberYPos", "4", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flIconXPos, "IconXPos", "16", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flIconYPos, "IconYPos", "8", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flTextYPos, "TextYPos", "54", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flErrorYPos, "ErrorYPos", "60", "proportional_float" );

	CPanelAnimationVar( float, m_flAlphaOverride, "Alpha", "255" );
	CPanelAnimationVar( float, m_flSelectionAlphaOverride, "SelectionAlpha", "255" );

	CPanelAnimationVar( Color, m_TextColor, "TextColor", "SelectionTextFg" );
	CPanelAnimationVar( Color, m_NumberColor, "NumberColor", "SelectionNumberFg" );
	CPanelAnimationVar( Color, m_EmptyBoxColor, "EmptyBoxColor", "SelectionEmptyBoxBg" );
	CPanelAnimationVar( Color, m_BoxColor, "BoxColor", "SelectionBoxBg" );
	CPanelAnimationVar( Color, m_SelectedBoxColor, "SelectedBoxClor", "SelectionSelectedBoxBg" );

	CPanelAnimationVar( float, m_flWeaponPickupGrowTime, "SelectionGrowTime", "0.1" );

	CPanelAnimationVar( float, m_flTextScan, "TextScan", "1.0" );

	CPanelAnimationVar( int, m_iMaxSlots, "MaxSlots", "6" );
	CPanelAnimationVar( bool, m_bPlaySelectionSounds, "PlaySelectSounds", "1" );

	CTFImagePanel *m_pActiveWeaponBG;

	float m_flDemoStartTime;
	float m_flDemoModeChangeTime;
	int m_iDemoModeSlot;
};

DECLARE_HUDELEMENT( CHudWeaponSelection );

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudWeaponSelection::CHudWeaponSelection( const char *pElementName ) : CBaseHudWeaponSelection( pElementName ), EditablePanel( NULL, "HudWeaponSelection" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pActiveWeaponBG = new CTFImagePanel( this, "ActiveWeapon" );
	m_pActiveWeaponBG->SetVisible( false );

	SetPostChildPaintEnabled( true );

	m_flDemoStartTime = -1;
	m_flDemoModeChangeTime = 0;
	m_iDemoModeSlot = -1;

	ListenForGameEvent( "localplayer_changeclass" );
}

//-----------------------------------------------------------------------------
// Purpose: sets up display for showing weapon pickup
//-----------------------------------------------------------------------------
void CHudWeaponSelection::OnWeaponPickup( C_BaseCombatWeapon *pWeapon )
{
	// add to pickup history
	CHudHistoryResource *pHudHR = GET_HUDELEMENT( CHudHistoryResource );
	if ( pHudHR )
	{
		pHudHR->AddToHistory( pWeapon );
	}
}

//-----------------------------------------------------------------------------
// Purpose: updates animation status
//-----------------------------------------------------------------------------
void CHudWeaponSelection::OnThink()
{
	// close
	if ( gpGlobals->curtime - m_flSelectionTime > SELECTION_FADEOUT_TIME )
	{
		HideSelection();
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the panel should draw
//-----------------------------------------------------------------------------
bool CHudWeaponSelection::ShouldDraw()
{	
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
	{
		if ( IsInSelectionMode() )
		{
			HideSelection();
		}
		return false;
	}
	
	// Make sure the player's allowed to switch weapons
	if ( pPlayer->IsAllowedToSwitchWeapons() == false )
		return false;

	if ( pPlayer->IsAlive() == false )
		return false;

	// we only show demo mode in hud_fastswitch 0
	if ( m_iDemoModeSlot >= 0 || m_flDemoStartTime > 0 )
	{
		return true;
	}

	if ( !CHudElement::ShouldDraw() )
		return false;

	return m_bSelectionVisible;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudWeaponSelection::LevelInit()
{
	CHudElement::LevelInit();
	
	m_iMaxSlots = clamp( m_iMaxSlots, 0, MAX_WEAPON_SLOTS );
}

//-------------------------------------------------------------------------
// Purpose: Calculates how many weapons slots need to be displayed
//-------------------------------------------------------------------------
int CHudWeaponSelection::GetNumVisibleSlots()
{
	int nCount = 0;

	// iterate over all the weapon slots
	for ( int i = 0; i < m_iMaxSlots; i++ )
	{
		if ( GetFirstPos( i ) )
		{
			nCount++;
		}
	}

	return nCount;
}

//-------------------------------------------------------------------------
// Purpose: draws the selection area
//-------------------------------------------------------------------------
void CHudWeaponSelection::PostChildPaint()
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	// See if we should start the bucket demo
	if ( m_flDemoStartTime > 0 && m_flDemoStartTime < gpGlobals->curtime )
	{
		float flDemoTime = tf_weapon_select_demo_time.GetFloat();

		if ( flDemoTime > 0 )
		{
			m_iDemoModeSlot = 0;
			m_flDemoModeChangeTime = gpGlobals->curtime + flDemoTime;
		}

		m_flDemoStartTime = -1;
		//m_iSelectedSlot = m_iDemoModeSlot;
	}

	// scroll through the slots for demo mode
	if ( m_iDemoModeSlot >= 0 && m_flDemoModeChangeTime < gpGlobals->curtime )
	{
		// Keep iterating until we find a slot that has a weapon in it
		while ( !GetFirstPos( ++m_iDemoModeSlot ) && m_iDemoModeSlot < m_iMaxSlots )
		{
			// blank
		}			
		m_flDemoModeChangeTime = gpGlobals->curtime + tf_weapon_select_demo_time.GetFloat();
	}

	if ( m_iDemoModeSlot >= m_iMaxSlots )
	{
		m_iDemoModeSlot = -1;
	}
	
	int iSelectedSlot = -1;

	// find and display our current selection
	C_BaseCombatWeapon *pSelectedWeapon = GetSelectedWeapon();
	if ( pSelectedWeapon )
	{
		iSelectedSlot = pSelectedWeapon->GetSlot();
	}

	if ( m_iDemoModeSlot > -1 )
	{
		iSelectedSlot = m_iDemoModeSlot;
	}

	m_pActiveWeaponBG->SetVisible( pSelectedWeapon != NULL );

	if ( iSelectedSlot <= -1 )
		return;

	int nNumSlots = GetNumVisibleSlots();
	if ( nNumSlots <= 0 )
		return;

	// calculate where to start drawing
	int nTotalHeight = ( nNumSlots - 1 ) * ( m_flSmallBoxTall + m_flBoxGap ) + m_flLargeBoxTall;
	int xStartPos = GetWide() - m_flBoxGap - m_flRightMargin;
	int xpos = xStartPos;
	int ypos = ( GetTall() - nTotalHeight ) / 2;

	int iActiveSlot = (pSelectedWeapon ? pSelectedWeapon->GetSlot() : -1);

	// draw the bucket set
	// iterate over all the weapon slots
	for ( int i = 0; i < m_iMaxSlots; i++ )
	{
		Color col( 255, 255, 255, 255 );

		if ( i == iActiveSlot )
		{
			xpos = xStartPos - m_flLargeBoxWide;

			bool bFirstItem = true;
			for ( int slotpos = 0; slotpos < MAX_WEAPON_POSITIONS; slotpos++ )
			{
				C_BaseCombatWeapon *pWeapon = GetWeaponInSlot(i, slotpos);
				if ( !pWeapon )
					continue;

				if ( !pWeapon->VisibleInWeaponSelection() )
					continue;

				if ( pWeapon == pSelectedWeapon || ( m_iDemoModeSlot == i ) )
				{
					// draw selected weapon
					if ( m_pActiveWeaponBG )
					{
						m_pActiveWeaponBG->SetPos( xpos - XRES(10), ypos - YRES(10));

						int shortcut = bFirstItem ? i + 1 : -1;

						if ( IsPC() && shortcut >= 0 )
						{
							Color numberColor = m_NumberColor;
							numberColor[3] *= m_flSelectionAlphaOverride / 255.0f;
							surface()->DrawSetTextColor(numberColor);
							surface()->DrawSetTextFont(m_hNumberFont);
							wchar_t wch = '0' + shortcut;
							surface()->DrawSetTextPos( xStartPos - XRES(5) - m_flSelectionNumberXPos, ypos + YRES(5) + m_flSelectionNumberYPos );
							surface()->DrawUnicodeChar(wch);
						}
					}
				}
				else
				{
					// draw selected weapon
					DrawBox( xpos + XRES(5), ypos + YRES(5), m_flLargeBoxWide - XRES(10), m_flLargeBoxTall - YRES(10), col, m_flSelectionAlphaOverride, bFirstItem ? i + 1 : -1 );
				}

				// draw icon
				const CHudTexture *pTexture = pWeapon->GetSpriteInactive(); // red team
				if ( pPlayer )
				{
					if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
					{
						pTexture = pWeapon->GetSpriteActive();
					}
				}

				if ( pTexture )
				{
					pTexture->DrawSelf( xpos, ypos, m_flLargeBoxWide, m_flLargeBoxTall, col );
				}

				if ( !pWeapon->CanBeSelected() )
				{
					int msgX = xpos + ( XRES(1) * 5.0 );
					int msgY = ypos + (int)m_flErrorYPos;
							
					wchar_t *pText = L"OUT OF AMMO";
					Color ammoColor = Color(-1,0,0,-1);
					DrawString( pText, msgX, msgY, ammoColor );
				}

				xpos -= ( m_flLargeBoxWide + m_flBoxGap );
				bFirstItem = false;
			}

			ypos += ( m_flLargeBoxTall + m_flBoxGap );
		}
		else
		{
			xpos = xStartPos - m_flSmallBoxWide;

			// check to see if there is a weapons in this bucket
			if ( GetFirstPos( i ) )
			{
				// draw has weapon in slot
				DrawBox( xpos + XRES(5), ypos + YRES(5), m_flSmallBoxWide - XRES(10), m_flSmallBoxTall - YRES(10), m_BoxColor, m_flAlphaOverride, i + 1 );

				C_BaseCombatWeapon *pWeapon = GetFirstPos( i );
				if ( !pWeapon )
					continue;

				const CHudTexture *pTexture = pWeapon->GetSpriteInactive(); // red team
				if ( pPlayer )
				{
					if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
					{
						pTexture = pWeapon->GetSpriteActive();
					}
				}

				if ( pTexture )
				{
					pTexture->DrawSelf( xpos, ypos, m_flSmallBoxWide, m_flSmallBoxTall, col  );
				}

				ypos += ( m_flSmallBoxTall + m_flBoxGap );	
			}
		}
	}
}

void CHudWeaponSelection::DrawString( wchar_t *text, int xpos, int ypos, Color col )
{
	surface()->DrawSetTextColor( col );
	surface()->DrawSetTextFont( m_hTextFont );

	// count the position
	int slen = 0, charCount = 0, maxslen = 0;
	{
		for (wchar_t *pch = text; *pch != 0; pch++)
		{
			if (*pch == '\n') 
			{
				// newline character, drop to the next line
				if (slen > maxslen)
				{
					maxslen = slen;
				}
				slen = 0;
			}
			else if (*pch == '\r')
			{
				// do nothing
			}
			else
			{
				slen += surface()->GetCharacterWidth( m_hTextFont, *pch );
				charCount++;
			}
		}
	}

	surface()->DrawSetTextPos( xpos, ypos );
	// adjust the charCount by the scan amount
	charCount *= m_flTextScan;
	for (wchar_t *pch = text; charCount > 0; pch++)
	{
		if (*pch == '\n')
		{
			// newline character, move to the next line
			surface()->DrawSetTextPos( xpos + ((m_flLargeBoxWide - slen) / 2), ypos + (surface()->GetFontTall(m_hTextFont) * 1.1f));
		}
		else if (*pch == '\r')
		{
			// do nothing
		}
		else
		{
			surface()->DrawUnicodeChar(*pch);
			charCount--;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: draws a selection box
//-----------------------------------------------------------------------------
void CHudWeaponSelection::DrawBox(int x, int y, int wide, int tall, Color color, float normalizedAlpha, int number)
{
	BaseClass::DrawBox( x, y, wide, tall, color, normalizedAlpha / 255.0f );

	// draw the number
	if ( IsPC() && number >= 0)
	{
		Color numberColor = m_NumberColor;
		numberColor[3] *= normalizedAlpha / 255.0f;
		surface()->DrawSetTextColor(numberColor);
		surface()->DrawSetTextFont(m_hNumberFont);
		wchar_t wch = '0' + number;
		surface()->DrawSetTextPos(x + wide - m_flSelectionNumberXPos, y + m_flSelectionNumberYPos);
		surface()->DrawUnicodeChar(wch);
	}
}

//-----------------------------------------------------------------------------
// Purpose: hud scheme settings
//-----------------------------------------------------------------------------
void CHudWeaponSelection::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	SetPaintBackgroundEnabled(false);

	// set our size
	int screenWide, screenTall;
	int x, y;
	GetPos(x, y);
	GetHudSize(screenWide, screenTall);
	SetBounds(0, 0, screenWide, screenTall);

	// load control settings...
	LoadControlSettings( "resource/UI/HudWeaponSelection.res" );
}

//-----------------------------------------------------------------------------
// Purpose: Opens weapon selection control
//-----------------------------------------------------------------------------
void CHudWeaponSelection::OpenSelection( void )
{
	Assert(!IsInSelectionMode());

	CBaseHudWeaponSelection::OpenSelection();
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("OpenWeaponSelectionMenu");
}

//-----------------------------------------------------------------------------
// Purpose: Closes weapon selection control immediately
//-----------------------------------------------------------------------------
void CHudWeaponSelection::HideSelection( void )
{
	CBaseHudWeaponSelection::HideSelection();
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("CloseWeaponSelectionMenu");
}

//-----------------------------------------------------------------------------
// Purpose: Returns the next available weapon item in the weapon selection
//-----------------------------------------------------------------------------
C_BaseCombatWeapon *CHudWeaponSelection::FindNextWeaponInWeaponSelection(int iCurrentSlot, int iCurrentPosition)
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return NULL;

	C_BaseCombatWeapon *pNextWeapon = NULL;

	// search all the weapons looking for the closest next
	int iLowestNextSlot = MAX_WEAPON_SLOTS;
	int iLowestNextPosition = MAX_WEAPON_POSITIONS;
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		C_BaseCombatWeapon *pWeapon = pPlayer->GetWeapon(i);
		if ( !pWeapon )
			continue;

		if ( pWeapon->VisibleInWeaponSelection() )
		{
			int weaponSlot = pWeapon->GetSlot(), weaponPosition = pWeapon->GetPosition();

			// see if this weapon is further ahead in the selection list
			if ( weaponSlot > iCurrentSlot || (weaponSlot == iCurrentSlot && weaponPosition > iCurrentPosition) )
			{
				// see if this weapon is closer than the current lowest
				if ( weaponSlot < iLowestNextSlot || (weaponSlot == iLowestNextSlot && weaponPosition < iLowestNextPosition) )
				{
					iLowestNextSlot = weaponSlot;
					iLowestNextPosition = weaponPosition;
					pNextWeapon = pWeapon;
				}
			}
		}
	}

	return pNextWeapon;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the prior available weapon item in the weapon selection
//-----------------------------------------------------------------------------
C_BaseCombatWeapon *CHudWeaponSelection::FindPrevWeaponInWeaponSelection(int iCurrentSlot, int iCurrentPosition)
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return NULL;

	C_BaseCombatWeapon *pPrevWeapon = NULL;

	// search all the weapons looking for the closest next
	int iLowestPrevSlot = -1;
	int iLowestPrevPosition = -1;
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		C_BaseCombatWeapon *pWeapon = pPlayer->GetWeapon(i);
		if ( !pWeapon )
			continue;

		if ( pWeapon->VisibleInWeaponSelection() )
		{
			int weaponSlot = pWeapon->GetSlot(), weaponPosition = pWeapon->GetPosition();

			// see if this weapon is further ahead in the selection list
			if ( weaponSlot < iCurrentSlot || (weaponSlot == iCurrentSlot && weaponPosition < iCurrentPosition) )
			{
				// see if this weapon is closer than the current lowest
				if ( weaponSlot > iLowestPrevSlot || (weaponSlot == iLowestPrevSlot && weaponPosition > iLowestPrevPosition) )
				{
					iLowestPrevSlot = weaponSlot;
					iLowestPrevPosition = weaponPosition;
					pPrevWeapon = pWeapon;
				}
			}
		}
	}

	return pPrevWeapon;
}

//-----------------------------------------------------------------------------
// Purpose: Moves the selection to the next item in the menu
//-----------------------------------------------------------------------------
void CHudWeaponSelection::CycleToNextWeapon( void )
{
	// Get the local player.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	C_BaseCombatWeapon *pNextWeapon = NULL;
	if ( IsInSelectionMode() )
	{
		// find the next selection spot
		C_BaseCombatWeapon *pWeapon = GetSelectedWeapon();
		if ( !pWeapon )
			return;

		pNextWeapon = FindNextWeaponInWeaponSelection( pWeapon->GetSlot(), pWeapon->GetPosition() );
	}
	else
	{
		// open selection at the current place
		pNextWeapon = pPlayer->GetActiveWeapon();
		if ( pNextWeapon )
		{
			pNextWeapon = FindNextWeaponInWeaponSelection( pNextWeapon->GetSlot(), pNextWeapon->GetPosition() );
		}
	}

	if ( !pNextWeapon )
	{
		// wrap around back to start
		pNextWeapon = FindNextWeaponInWeaponSelection(-1, -1);
	}

	if ( pNextWeapon )
	{
		SetSelectedWeapon( pNextWeapon );

		if( hud_fastswitch.GetInt() > 0 )
		{
			SelectWeapon();
		}
		else if ( !IsInSelectionMode() )
		{
			OpenSelection();
		}

		// cancel demo mode
		m_iDemoModeSlot = -1;
		m_flDemoStartTime = -1;

		// Play the "cycle to next weapon" sound
		if( m_bPlaySelectionSounds )
			pPlayer->EmitSound( "Player.WeaponSelectionMoveSlot" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Moves the selection to the previous item in the menu
//-----------------------------------------------------------------------------
void CHudWeaponSelection::CycleToPrevWeapon( void )
{
	// Get the local player.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	if ( pPlayer->IsAlive() == false )
		return;

	C_BaseCombatWeapon *pNextWeapon = NULL;
	if ( IsInSelectionMode() )
	{
		// find the next selection spot
		C_BaseCombatWeapon *pWeapon = GetSelectedWeapon();
		if ( !pWeapon )
			return;

		pNextWeapon = FindPrevWeaponInWeaponSelection( pWeapon->GetSlot(), pWeapon->GetPosition() );
	}
	else
	{
		// open selection at the current place
		pNextWeapon = pPlayer->GetActiveWeapon();
		if ( pNextWeapon )
		{
			pNextWeapon = FindPrevWeaponInWeaponSelection( pNextWeapon->GetSlot(), pNextWeapon->GetPosition() );
		}
	}

	if ( !pNextWeapon )
	{
		// wrap around back to end of weapon list
		pNextWeapon = FindPrevWeaponInWeaponSelection(MAX_WEAPON_SLOTS, MAX_WEAPON_POSITIONS);
	}

	if ( pNextWeapon )
	{
		SetSelectedWeapon( pNextWeapon );

		if( hud_fastswitch.GetInt() > 0 )
		{
			SelectWeapon();
		}
		else if ( !IsInSelectionMode() )
		{
			OpenSelection();
		}

		// cancel demo mode
		m_iDemoModeSlot = -1;
		m_flDemoStartTime = -1;

		// Play the "cycle to next weapon" sound
		if( m_bPlaySelectionSounds )
			pPlayer->EmitSound( "Player.WeaponSelectionMoveSlot" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns the weapon in the specified slot
//-----------------------------------------------------------------------------
C_BaseCombatWeapon *CHudWeaponSelection::GetWeaponInSlot( int iSlot, int iSlotPos )
{
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return NULL;

	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		C_BaseCombatWeapon *pWeapon = player->GetWeapon(i);
		
		if ( pWeapon == NULL )
			continue;

		if ( pWeapon->GetSlot() == iSlot && pWeapon->GetPosition() == iSlotPos )
			return pWeapon;
	}

	return NULL;
}

C_BaseCombatWeapon *CHudWeaponSelection::GetSelectedWeapon( void )
{ 
	if ( m_iDemoModeSlot >= 0 )
	{
		C_BaseCombatWeapon *pWeapon = GetFirstPos( m_iDemoModeSlot );
		return pWeapon;
	}
	else
	{
		return m_hSelectedWeapon;
	}
}

void CHudWeaponSelection::FireGameEvent( IGameEvent *event )
{
	const char * type = event->GetName();

	if ( Q_strcmp(type, "localplayer_changeclass") == 0 )
	{
		m_flDemoStartTime = gpGlobals->curtime + tf_weapon_select_demo_start_delay.GetFloat();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Moves selection to the specified slot
//-----------------------------------------------------------------------------
void CHudWeaponSelection::SelectWeaponSlot( int iSlot )
{
	// iSlot is one higher than it should be, since it's the number key, not the 0-based index into the weapons
	--iSlot;

	// Get the local player.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	// Don't try and read past our possible number of slots
	if ( iSlot > MAX_WEAPON_SLOTS )
		return;
	
	// Make sure the player's allowed to switch weapons
	if ( pPlayer->IsAllowedToSwitchWeapons() == false )
		return;
	
	int slotPos = 0;
	C_BaseCombatWeapon *pActiveWeapon = GetSelectedWeapon();

	// start later in the list
	if ( IsInSelectionMode() && pActiveWeapon && pActiveWeapon->GetSlot() == iSlot )
	{
		slotPos = pActiveWeapon->GetPosition() + 1;
	}
	else if ( !pActiveWeapon )
	{

	}

	// find the weapon in this slot
	pActiveWeapon = GetNextActivePos( iSlot, slotPos );
	if ( !pActiveWeapon )
	{
		pActiveWeapon = GetNextActivePos( iSlot, 0 );
	}
			
	if ( pActiveWeapon != NULL )
	{
		// Mark the change
		SetSelectedWeapon( pActiveWeapon );

		if( hud_fastswitch.GetInt() > 0 )
		{
			SelectWeapon();
		}
		else if ( !IsInSelectionMode() )
		{
			// open the weapon selection
			OpenSelection();
		}

		m_iDemoModeSlot = -1;
		m_flDemoStartTime = -1;
	}

	if( m_bPlaySelectionSounds )
		pPlayer->EmitSound( "Player.WeaponSelectionMoveSlot" );
}
