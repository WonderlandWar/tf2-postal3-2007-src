//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_hud_freezepanel.h"
#include "vgui_controls/AnimationController.h"
#include "iclientmode.h"
#include "c_tf_player.h"
#include "c_tf_playerresource.h"
#include <vgui_controls/Label.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "c_baseobject.h"
#include "fmtstr.h"
#include "tf_gamerules.h"
#include "tf_hud_statpanel.h"
#include "view.h"
#include "ivieweffects.h"
#include "viewrender.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT_DEPTH( CTFFreezePanel, 1 );

#define CALLOUT_WIDE		(XRES(100))
#define CALLOUT_TALL		(XRES(50))

extern float g_flFreezeFlash;

#define FREEZECAM_SCREENSHOT_STRING "is looking good!"

bool IsTakingAFreezecamScreenshot( void )
{
	// Don't draw in freezecam, or when the game's not running
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	bool bInFreezeCam = ( pPlayer && pPlayer->GetObserverMode() == OBS_MODE_FREEZECAM );

	if ( bInFreezeCam == true && engine->IsTakingScreenshot() )
		return true;

	return false;
}

DECLARE_BUILD_FACTORY( CTFFreezePanelHealth );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFFreezePanel::CTFFreezePanel( const char *pElementName )
	: EditablePanel( NULL, "FreezePanel" ), CHudElement( pElementName )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	SetVisible( false );
	SetScheme( "ClientScheme" );

	m_iKillerIndex = 0;
	m_iShowNemesisPanel = SHOW_NO_NEMESIS;
	m_iYBase = -1;
	m_flShowCalloutsAt = 0;
	
	m_pKillerHealth = new CTFFreezePanelHealth( this, "FreezePanelHealth" );
	m_pNemesisSubPanel = new EditablePanel( this, "NemesisSubPanel" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFreezePanel::Reset()
{
	Hide();

	if ( m_pKillerHealth )
	{
		m_pKillerHealth->Reset();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFreezePanel::Init()
{
	// listen for events
	ListenForGameEvent( "show_freezepanel" );
	ListenForGameEvent( "hide_freezepanel" );
	ListenForGameEvent( "freezecam_started" );
	ListenForGameEvent( "player_death" );
	ListenForGameEvent( "teamplay_win_panel" );
	
	Hide();

	CHudElement::Init();
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CTFFreezePanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
		
	// Move killer panels when the win panel is up
	int xp,yp;
	GetPos( xp, yp );
	m_iYBase = yp;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFreezePanel::FireGameEvent( IGameEvent * event )
{
	const char *pEventName = event->GetName();

	if ( Q_strcmp( "player_death", pEventName ) == 0 )
	{
		// see if the local player died
		int iPlayerIndexVictim = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer && iPlayerIndexVictim == pLocalPlayer->entindex() )
		{
			// the local player is dead, see if this is a new nemesis or a revenge
			if ( event->GetInt( "dominated" ) > 0 )
			{
				m_iShowNemesisPanel = SHOW_NEW_NEMESIS;
			}
			else if ( event->GetInt( "revenge" ) > 0 )
			{
				m_iShowNemesisPanel = SHOW_REVENGE;
			}
			else
			{
				m_iShowNemesisPanel = SHOW_NO_NEMESIS;
			}
		}		
	}
	else if ( Q_strcmp( "hide_freezepanel", pEventName ) == 0 )
	{
		Hide();
	}
	else if ( Q_strcmp( "freezecam_started", pEventName ) == 0 )
	{
		ShowCalloutsIn( 1.0 );
		ShowSnapshotPanelIn( 1.25 );
	}
	else if ( Q_strcmp( "teamplay_win_panel", pEventName ) == 0 )
	{
		Hide();
	}
	else if ( Q_strcmp( "show_freezepanel", pEventName ) == 0 )
	{
		C_TF_PlayerResource *tf_PR = dynamic_cast<C_TF_PlayerResource *>(g_PR);
		if ( !tf_PR )
		{
			m_pNemesisSubPanel->SetDialogVariable( "nemesisname", NULL );
			return;
		}

		Show();

		ShowSnapshotPanel( false );

		// Get the entity who killed us
		m_iKillerIndex = event->GetInt( "killer" );
		C_BaseEntity *pKiller =  ClientEntityList().GetBaseEntity( m_iKillerIndex );
		
		LoadControlSettings( "resource/UI/FreezePanel_Basic.res" );

		int xp,yp;
		GetPos( xp, yp );
		if ( TFGameRules()->RoundHasBeenWon() )
		{
			SetPos( xp, m_iYBase - YRES(50) );
		}
		else
		{
			SetPos( xp, m_iYBase );
		}

		if ( pKiller )
		{
			CTFPlayer *pPlayer = ToTFPlayer ( pKiller );
			int iMaxBuffedHealth = 0;

			if ( pPlayer )
			{
				iMaxBuffedHealth = pPlayer->m_Shared.GetMaxBuffedHealth();
			}

			int iKillerHealth = pKiller->GetHealth();
			if ( !pKiller->IsAlive() )
			{
				iKillerHealth = 0;
			}
			m_pKillerHealth->SetHealth( iKillerHealth, pKiller->GetMaxHealth(), iMaxBuffedHealth );

			vgui::Label *pFreezeLabel = dynamic_cast<Label *>( FindChildByName("FreezeLabel") );

			if ( pKiller->IsPlayer() )
			{
				C_TFPlayer *pVictim = C_TFPlayer::GetLocalTFPlayer();
				CTFPlayer *pTFKiller = ToTFPlayer( pKiller );

				//If this was just a regular kill but this guy is our nemesis then just show it.
				if ( pVictim && pTFKiller && pTFKiller->m_Shared.IsPlayerDominated( pVictim->entindex() ) )
				{
					if ( !pKiller->IsAlive() )
					{
						pFreezeLabel->SetText( "#FreezePanel_Nemesis_Dead" );
					}
					else
					{
						pFreezeLabel->SetText( "#FreezePanel_Nemesis" );
					}
				}
				else
				{
					if ( !pKiller->IsAlive() )
					{
						pFreezeLabel->SetText( "#FreezePanel_Killer_Dead" );
					}
					else
					{
						pFreezeLabel->SetText( "#FreezePanel_Killer" );
					}
				}

				SetDialogVariable( "killername", g_PR->GetPlayerName( m_iKillerIndex ) );
				
				CAvatarImagePanel *pAvatar = dynamic_cast<CAvatarImagePanel*>( FindChildByName( "AvatarImage" ) );
				if ( pAvatar )
				{
					pAvatar->SetPlayer( (C_BasePlayer*)pKiller );
				}
			}
			else if ( pKiller->IsBaseObject() )
			{
				C_BaseObject *pObj = assert_cast<C_BaseObject *>( pKiller );
				C_TFPlayer *pOwner = pObj->GetOwner();

				Assert( pOwner && "Why does this object not have an owner?" );

				if ( pOwner )
				{
					m_iKillerIndex = pOwner->entindex();

					SetDialogVariable( "killername", g_PR->GetPlayerName( m_iKillerIndex ) );

					CAvatarImagePanel *pAvatar = dynamic_cast<CAvatarImagePanel*>( FindChildByName( "AvatarImage" ) );

					if ( pAvatar )
					{
						pAvatar->SetPlayer( pOwner );
					}

					pKiller = pOwner;
				}

				if ( pFreezeLabel )
				{
					if ( pKiller && !pKiller->IsAlive() )
					{
						pFreezeLabel->SetText( "#FreezePanel_KillerObject_Dead" );
					}
					else
					{
						pFreezeLabel->SetText( "#FreezePanel_KillerObject" );
					}
				}
				const char *pszStatusName = pObj->GetStatusName();
				wchar_t *wszLocalized = g_pVGuiLocalize->Find( pszStatusName );

				if ( !wszLocalized )
				{
					SetDialogVariable( "objectkiller", pszStatusName );
				}
				else
				{
					SetDialogVariable( "objectkiller", wszLocalized );
				}
			}
			else if ( pFreezeLabel )
			{
				if ( !pKiller->IsAlive() )
				{
					pFreezeLabel->SetText( "#FreezePanel_Killer_Dead" );
				}
				else
				{
					pFreezeLabel->SetText( "#FreezePanel_Killer" );
				}
			}
		}
		
		// see if we should show nemesis panel
		const wchar_t *pchNemesisText = NULL;
		switch ( m_iShowNemesisPanel )
		{
		case SHOW_NO_NEMESIS:
			{
				C_TFPlayer *pVictim = C_TFPlayer::GetLocalTFPlayer();
				CTFPlayer *pTFKiller = ToTFPlayer( pKiller );
			
				//If this was just a regular kill but this guy is our nemesis then just show it.
				if ( pTFKiller && pTFKiller->m_Shared.IsPlayerDominated( pVictim->entindex() ) )
				{					
					pchNemesisText = g_pVGuiLocalize->Find( "#TF_FreezeNemesis" );
				}	
			}
			break;
		case SHOW_NEW_NEMESIS:
			{
				C_TFPlayer *pVictim = C_TFPlayer::GetLocalTFPlayer();
				CTFPlayer *pTFKiller = ToTFPlayer( pKiller );
				// check to see if killer is still the nemesis of victim; victim may have managed to kill him after victim's
				// death by grenade or some such, extracting revenge and clearing nemesis condition
				if ( pTFKiller && pTFKiller->m_Shared.IsPlayerDominated( pVictim->entindex() ) )
				{					
					pchNemesisText = g_pVGuiLocalize->Find( "#TF_NewNemesis" );
				}			
			}
			break;
		case SHOW_REVENGE:
			pchNemesisText = g_pVGuiLocalize->Find( "#TF_GotRevenge" );
			break;
		default:
			Assert( false );	// invalid value
			break;
		}
		m_pNemesisSubPanel->SetDialogVariable( "nemesisname", pchNemesisText );

		ShowNemesisPanel( NULL != pchNemesisText );
		m_iShowNemesisPanel = SHOW_NO_NEMESIS;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFreezePanel::ShowCalloutsIn( float flTime )
{
	m_flShowCalloutsAt = gpGlobals->curtime + flTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFreezePanelCallout *CTFFreezePanel::TestAndAddCallout( Vector &origin, Vector &vMins, Vector &vMaxs, CUtlVector<Vector> *vecCalloutsTL, 
			CUtlVector<Vector> *vecCalloutsBR, Vector &vecFreezeTL, Vector &vecFreezeBR, Vector &vecStatTL, Vector &vecStatBR, int *iX, int *iY )
{
	// This is the offset from the topleft of the callout to the arrow tip
	const int iXOffset = XRES(25);
	const int iYOffset = YRES(50);

	//if ( engine->IsBoxInViewCluster( vMins + origin, vMaxs + origin) && !engine->CullBox( vMins + origin, vMaxs + origin ) )
	{
		if ( GetVectorInScreenSpace( origin, *iX, *iY ) )
		{
			*iX -= iXOffset;
			*iY -= iYOffset;
			int iRight = *iX + CALLOUT_WIDE;
			int iBottom = *iY + CALLOUT_TALL;
			if ( *iX > 0 && *iY > 0 && (iRight < ScreenWidth()) && (iBottom < (ScreenHeight()-YRES(40))) )
			{
				// Make sure it wouldn't be over the top of the freezepanel or statpanel
				Vector vecCalloutTL( *iX, *iY, 0 );
				Vector vecCalloutBR( iRight, iBottom, 1 );
				if ( !QuickBoxIntersectTest( vecCalloutTL, vecCalloutBR, vecFreezeTL, vecFreezeBR ) &&
					 !QuickBoxIntersectTest( vecCalloutTL, vecCalloutBR, vecStatTL, vecStatBR ) )
				{
					// Make sure it doesn't intersect any other callouts
					bool bClear = true;
					for ( int iCall = 0; iCall < vecCalloutsTL->Count(); iCall++ )
					{
						if ( QuickBoxIntersectTest( vecCalloutTL, vecCalloutBR, vecCalloutsTL->Element(iCall), vecCalloutsBR->Element(iCall) ) )
						{
							bClear = false;
							break;
						}
					}

					if ( bClear )
					{
						// Verify that we have LOS to the gib
						trace_t	tr;
						UTIL_TraceLine( origin, MainViewOrigin(), MASK_OPAQUE, NULL, COLLISION_GROUP_NONE, &tr );
						bClear = ( tr.fraction >= 1.0f );
					}

					if ( bClear )
					{
						CTFFreezePanelCallout *pCallout = new CTFFreezePanelCallout( g_pClientMode->GetViewport(), "FreezePanelCallout" );
						m_pCalloutPanels.AddToTail( vgui::SETUP_PANEL(pCallout) );
						vecCalloutsTL->AddToTail( vecCalloutTL );
						vecCalloutsBR->AddToTail( vecCalloutBR );
						pCallout->SetVisible( true );
						pCallout->SetBounds( *iX, *iY, CALLOUT_WIDE, CALLOUT_TALL );
						return pCallout;
					}
				}
			}
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFreezePanel::UpdateCallout( void )
{
	CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	// Abort early if we have no gibs or ragdoll
	CUtlVector<EHANDLE>	*pGibList = pPlayer->GetSpawnedGibs();
	IRagdoll *pRagdoll = pPlayer->GetRepresentativeRagdoll();
	if ( (!pGibList || pGibList->Count() == 0) && !pRagdoll )
		return;

	// Precalc the vectors of the freezepanel & statpanel
	int iX, iY;
	GetPos( iX, iY );
	Vector vecFreezeTL( iX, iY, 0 );
	Vector vecFreezeBR( iX + GetWide(), iY + GetTall(), 1 );

	CUtlVector<Vector> vecCalloutsTL;
	CUtlVector<Vector> vecCalloutsBR;

	Vector vecStatTL(0,0,0);
	Vector vecStatBR(0,0,1);
	CTFStatPanel *pStatPanel = GET_HUDELEMENT( CTFStatPanel );
	if ( pStatPanel && pStatPanel->IsVisible() )
	{
		pStatPanel->GetPos( iX, iY );
		vecStatTL.x = iX;
		vecStatTL.y = iY;
		vecStatBR.x = vecStatTL.x + pStatPanel->GetWide();
		vecStatBR.y = vecStatTL.y + pStatPanel->GetTall();
	}

	Vector vMins, vMaxs;

	// Check gibs
	if ( pGibList && pGibList->Count() )
	{
		int iCount = 0;
		for ( int i = 0; i < pGibList->Count(); i++ )
		{
			CBaseEntity *pGib = pGibList->Element(i);
			if ( pGib )
			{
				Vector origin = pGib->GetRenderOrigin();
				IPhysicsObject *pPhysicsObject = pGib->VPhysicsGetObject();
				if( pPhysicsObject )
				{
					Vector vecMassCenter = pPhysicsObject->GetMassCenterLocalSpace();
					pGib->CollisionProp()->CollisionToWorldSpace( vecMassCenter, &origin );
				}
				pGib->GetRenderBounds( vMins, vMaxs );

				// Try and add the callout
				CTFFreezePanelCallout *pCallout = TestAndAddCallout( origin, vMins, vMaxs, &vecCalloutsTL, &vecCalloutsBR, 
					vecFreezeTL, vecFreezeBR, vecStatTL, vecStatBR, &iX, &iY );
				if ( pCallout )
				{
					pCallout->UpdateForGib( i, iCount );
					iCount++;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFreezePanel::Show()
{
	m_flShowCalloutsAt = 0;
	SetVisible( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFreezePanel::Hide()
{
	SetVisible( false );

	// Delete all our callout panels
	for ( int i = m_pCalloutPanels.Count()-1; i >= 0; i-- )
	{
		m_pCalloutPanels[i]->MarkForDeletion();
	}
	m_pCalloutPanels.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFreezePanel::ShouldDraw( void )
{
	return ( IsVisible() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFreezePanel::OnThink( void )
{
	BaseClass::OnThink();

	if ( m_flShowCalloutsAt && m_flShowCalloutsAt < gpGlobals->curtime )
	{
		if ( ShouldDraw() )
		{
			UpdateCallout();
		}
		m_flShowCalloutsAt = 0;
	}

	if ( m_flShowSnapshotReminderAt && m_flShowSnapshotReminderAt < gpGlobals->curtime )
	{
		if ( ShouldDraw() )
		{
			ShowSnapshotPanel( true );
		}
		m_flShowSnapshotReminderAt = 0;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFreezePanel::ShowSnapshotPanelIn( float flTime )
{
#if defined (_X360 )
	return;
#endif

	m_flShowSnapshotReminderAt = gpGlobals->curtime + flTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFreezePanel::ShowSnapshotPanel( bool bShow )
{
	vgui::ImagePanel *pScreenshotPanel = dynamic_cast<ImagePanel *>( FindChildByName( "ScreenshotPanelBG" ) );
	vgui::ImagePanel *pIcon = dynamic_cast<ImagePanel *>( FindChildByName( "ScreenshotIcon" ) );
	vgui::Label *pLabel = dynamic_cast<Label *>( FindChildByName( "ScreenshotLabel" ) );
	if ( !pScreenshotPanel || !pIcon || !pLabel )
		return;

	const char *key = engine->Key_LookupBinding( "screenshot" );

	if ( key == NULL || FStrEq( key, "(null)" ) )
	{
		bShow = false;
		key = " ";
	}
		
	pScreenshotPanel->SetVisible( bShow );
	pIcon->SetVisible( bShow );
	pLabel->SetVisible( bShow );

	char szKey[16];
	Q_snprintf( szKey, sizeof(szKey), "%s", key );
	wchar_t wKey[16];
	wchar_t wLabel[256];

	g_pVGuiLocalize->ConvertANSIToUnicode(szKey, wKey, sizeof(wKey));
	g_pVGuiLocalize->ConstructString( wLabel, sizeof( wLabel ), g_pVGuiLocalize->Find("#TF_freezecam_snapshot" ), 1, wKey );

	pLabel->SetText( wLabel );

	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudSnapShotReminderIn" );
	
}

int	CTFFreezePanel::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( ShouldDraw() && pszCurrentBinding )
	{
		if ( FStrEq( pszCurrentBinding, "screenshot" ) || FStrEq( pszCurrentBinding, "jpeg" ) )
		{
			// Get the local player.
			C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
			if ( pPlayer )
			{
				//Do effects
				g_flFreezeFlash = gpGlobals->curtime + 0.75f;
				pPlayer->EmitSound( "Camera.SnapShot" );

				//Extend Freezecam by a couple more seconds.
				engine->ClientCmd( "extendfreeze" );
				view->FreezeFrame( 3.0f );

				//Hide the reminder panel
				m_flShowSnapshotReminderAt = 0;
				ShowSnapshotPanel( false );

				//Set the screenshot name
				if ( m_iKillerIndex <= MAX_PLAYERS )
				{
					const char *pszKillerName = g_PR->GetPlayerName( m_iKillerIndex );

					if ( pszKillerName )
					{
						ConVarRef cl_screenshotname( "cl_screenshotname" );

						if ( cl_screenshotname.IsValid() )
						{
							char szScreenShotName[512];

							Q_snprintf( szScreenShotName, sizeof( szScreenShotName ), "%s %s", pszKillerName, FREEZECAM_SCREENSHOT_STRING );

							cl_screenshotname.SetValue( szScreenShotName );
						}
					}
				}
			}
		}
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Shows or hides the nemesis part of the panel
//-----------------------------------------------------------------------------
void CTFFreezePanel::ShowNemesisPanel( bool bShow )
{
	m_pNemesisSubPanel->SetVisible( bShow );

#ifndef _X360
	if ( bShow )
	{
		vgui::Label *pLabel = dynamic_cast< vgui::Label *>( m_pNemesisSubPanel->FindChildByName( "NemesisLabel" ) );
		vgui::ImagePanel *pBG = dynamic_cast< vgui::ImagePanel *>( m_pNemesisSubPanel->FindChildByName( "NemesisPanelBG" ) );
		vgui::ImagePanel *pIcon = dynamic_cast< vgui::ImagePanel *>( m_pNemesisSubPanel->FindChildByName( "NemesisIcon" ) );

		// check that our Nemesis panel and resize it to the length of the string (the right side is pinned and doesn't move)
		if ( pLabel && pBG && pIcon )
		{
			int wide, tall;
			pLabel->GetContentSize( wide, tall );

			int nDiff = wide - pLabel->GetWide();

			if ( nDiff != 0 )
			{
				int x, y, w, t;

				// move the icon
				pIcon->GetBounds( x, y, w, t );
				pIcon->SetBounds( x - nDiff, y, w, t );

				// move/resize the label
				pLabel->GetBounds( x, y, w, t );
				pLabel->SetBounds( x - nDiff, y, w + nDiff, t );

				// move/resize the background
				pBG->GetBounds( x, y, w, t );
				pBG->SetBounds( x - nDiff, y, w + nDiff, t );
			}
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFreezePanelCallout::CTFFreezePanelCallout( Panel *parent, const char *name ) : EditablePanel(parent,name)
{
	m_pGibLabel = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CTFFreezePanelCallout::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/FreezePanelCallout.res" );

	m_pGibLabel = dynamic_cast<Label *>( FindChildByName("CalloutLabel") );
}

const char *pszCalloutGibNames[] =
{
	"#Callout_Head",
	"#Callout_Foot",
	"#Callout_Hand",
	"#Callout_Torso",
	NULL,	// Random
};
const char *pszCalloutRandomGibNames[] =
{
	"#Callout_Organ2",
	"#Callout_Organ3",
	"#Callout_Organ4",
	"#Callout_Organ5",
	"#Callout_Organ6",
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFreezePanelCallout::UpdateForGib( int iGib, int iCount )
{
	if ( !m_pGibLabel )
		return;

	if ( iGib < ARRAYSIZE(pszCalloutGibNames) )
	{
		if ( pszCalloutGibNames[iGib] )
		{
			m_pGibLabel->SetText( pszCalloutGibNames[iGib] );
		}
		else
		{
			m_pGibLabel->SetText( pszCalloutRandomGibNames[ RandomInt(0,ARRAYSIZE(pszCalloutRandomGibNames)-1) ] );
		}
	}
	else
	{
		if ( iCount > 1 )
		{
			m_pGibLabel->SetText( "#FreezePanel_Callout3" );
		}
		else if ( iCount == 1 )
		{
			m_pGibLabel->SetText( "#FreezePanel_Callout2" );
		}
		else
		{
			m_pGibLabel->SetText( "#FreezePanel_Callout" );
		}
	}
	
#ifndef _X360
	int wide, tall;
	m_pGibLabel->GetContentSize( wide, tall );

	// is the text wider than the label?
	if ( wide > m_pGibLabel->GetWide() )
	{
		int nDiff = wide - m_pGibLabel->GetWide();
		int x, y, w, t;

		// make the label wider
		m_pGibLabel->GetBounds( x, y, w, t );
		m_pGibLabel->SetBounds( x, y, w + nDiff, t );

		CTFImagePanel *pBackground = dynamic_cast<CTFImagePanel *>( FindChildByName( "CalloutBG" ) );
		if ( pBackground )
		{
			// also adjust the background image
			pBackground->GetBounds( x, y, w, t );
			pBackground->SetBounds( x, y, w + nDiff, t );
		}

		// make ourselves bigger to accommodate the wider children
		GetBounds( x, y, w, t );
		SetBounds( x, y, w + nDiff, t );

		// check that we haven't run off the right side of the screen
		if ( x + GetWide() > ScreenWidth() )
		{
			// push ourselves to the left to fit on the screen
			nDiff = ( x + GetWide() ) - ScreenWidth();
			SetPos( x - nDiff, y );

			// push the arrow to the right to offset moving ourselves to the left
			vgui::ImagePanel *pArrow = dynamic_cast<ImagePanel *>( FindChildByName( "ArrowIcon" ) );
			if ( pArrow )
			{
				pArrow->GetBounds( x, y, w, t );
				pArrow->SetBounds( x + nDiff, y, w, t );
			}
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFreezePanelCallout::UpdateForRagdoll( void )
{
	if ( !m_pGibLabel )
		return;

	m_pGibLabel->SetText( "#Callout_Ragdoll" );	
}