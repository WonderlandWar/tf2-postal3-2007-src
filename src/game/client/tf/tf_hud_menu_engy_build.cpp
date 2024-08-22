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
#include <vgui/IVGUI.h>
#include "c_baseobject.h"

#include "tf_hud_menu_engy_build.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//======================================

DECLARE_HUDELEMENT_DEPTH( CHudMenuEngyBuild, 40 );	// in front of engy building status

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudMenuEngyBuild::CHudMenuEngyBuild( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudMenuEngyBuild" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	for ( int i=0; i<4; i++ )
	{
		char buf[32];

		Q_snprintf( buf, sizeof(buf), "active_item_%d", i+1 );
		m_pAvailableObjects[i] = new EditablePanel( this, buf );

		Q_snprintf( buf, sizeof(buf), "already_built_item_%d", i+1 );
		m_pAlreadyBuiltObjects[i] = new EditablePanel( this, buf );

		Q_snprintf( buf, sizeof(buf), "cant_afford_item_%d", i+1 );
		m_pCantAffordObjects[i] = new EditablePanel( this, buf );
	}

	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuEngyBuild::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/build_menu/HudMenuEngyBuild.res" );

	// Load the already built images, not destroyable
	m_pAlreadyBuiltObjects[0]->LoadControlSettings( "resource/UI/build_menu/sentry_already_built.res" );
	m_pAlreadyBuiltObjects[1]->LoadControlSettings( "resource/UI/build_menu/dispenser_already_built.res" );
	m_pAlreadyBuiltObjects[2]->LoadControlSettings( "resource/UI/build_menu/tele_entrance_already_built.res" );
	m_pAlreadyBuiltObjects[3]->LoadControlSettings( "resource/UI/build_menu/tele_exit_already_built.res" );

	m_pAvailableObjects[0]->LoadControlSettings( "resource/UI/build_menu/sentry_active.res" );
	m_pAvailableObjects[1]->LoadControlSettings( "resource/UI/build_menu/dispenser_active.res" );
	m_pAvailableObjects[2]->LoadControlSettings( "resource/UI/build_menu/tele_entrance_active.res" );
	m_pAvailableObjects[3]->LoadControlSettings( "resource/UI/build_menu/tele_exit_active.res" );

	m_pCantAffordObjects[0]->LoadControlSettings( "resource/UI/build_menu/sentry_cant_afford.res" );
	m_pCantAffordObjects[1]->LoadControlSettings( "resource/UI/build_menu/dispenser_cant_afford.res" );
	m_pCantAffordObjects[2]->LoadControlSettings( "resource/UI/build_menu/tele_entrance_cant_afford.res" );
	m_pCantAffordObjects[3]->LoadControlSettings( "resource/UI/build_menu/tele_exit_cant_afford.res" );

	// Set the cost label
	for ( int i=0; i<4; i++ )
	{
		int iCost = GetObjectInfo( MapIndexToObjectID( i ) )->m_Cost;

		m_pAvailableObjects[i]->SetDialogVariable( "metal", iCost );
		m_pAlreadyBuiltObjects[i]->SetDialogVariable( "metal", iCost );
		m_pCantAffordObjects[i]->SetDialogVariable( "metal", iCost );
	}

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudMenuEngyBuild::ShouldDraw( void )
{
	CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return false;

	CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();

	if ( !pWpn )
		return false;

	// Don't show the menu for first person spectator
	if ( pPlayer != pWpn->GetOwner() )
		return false;

	return ( pWpn->GetWeaponID() == TF_WEAPON_PDA_ENGINEER_BUILD );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CHudMenuEngyBuild::MapIndexToObjectID( int index )
{
	static int iRemapIndexToObjectID[4] = 
	{
		OBJ_SENTRYGUN,
		OBJ_DISPENSER,
		OBJ_TELEPORTER_ENTRANCE,
		OBJ_TELEPORTER_EXIT
	};

	Assert( index >= 0 && index <= 3 );

	if ( index >= 0 && index <= 3 )
	{
		return iRemapIndexToObjectID[index];
	}
	else
	{
		Assert( !"Bad param to CHudMenuEngyBuild::MMapIndexToObjectID" );
		return OBJ_LAST;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Keyboard input hook. Return 0 if handled
//-----------------------------------------------------------------------------
int	CHudMenuEngyBuild::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( !ShouldDraw() )
	{
		return 1;
	}

	if ( !down )
	{
		return 1;
	}

	int iSlot = 0;

	switch( keynum )
	{
	case KEY_1:
	case KEY_XBUTTON_UP:
		iSlot = OBJ_SENTRYGUN;
		break;
	case KEY_2:
    case KEY_XBUTTON_RIGHT:
		iSlot = OBJ_DISPENSER;
		break;
	case KEY_3:
	case KEY_XBUTTON_DOWN:
		iSlot = OBJ_TELEPORTER_ENTRANCE;
		break;
	case KEY_4:
	case KEY_XBUTTON_LEFT:
		iSlot = OBJ_TELEPORTER_EXIT;
		break;

	case KEY_5:
	case KEY_6:
	case KEY_7:
	case KEY_8:
	case KEY_9:
		// Eat these keys
		return 0;

	case KEY_0:
	case KEY_XBUTTON_B:
		// cancel, close the menu
		engine->ExecuteClientCmd( "lastinv" );
		return 0;

	default:
		return 1;	// key not handled
	}
	
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pLocalPlayer )
		return 1;

	C_BaseObject *pObj = pLocalPlayer->GetObjectOfType( iSlot );
	int iCost = GetObjectInfo( iSlot )->m_Cost;

	if ( pObj == NULL && pLocalPlayer->GetAmmoCount( TF_AMMO_METAL ) >= iCost )
	{
		char szCmd[128];
		Q_snprintf( szCmd, sizeof(szCmd), "build %d", iSlot );
		engine->ClientCmd( szCmd );
		return 0;
	}
	else
	{
		pLocalPlayer->EmitSound( "Player.DenyWeaponSelection" );
		return 0;
	}

	return 1;	// key not handled
}

void CHudMenuEngyBuild::OnTick( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pLocalPlayer )
		return;

	int iAccount = pLocalPlayer->GetAmmoCount( TF_AMMO_METAL );

	for ( int i=0;i<4; i++ )
	{
		int iRemappedObjectID = MapIndexToObjectID( i );

		// update this slot
		C_BaseObject *pObj = NULL;

		if ( pLocalPlayer )
		{
			pObj = pLocalPlayer->GetObjectOfType( iRemappedObjectID );
		}			

		m_pAvailableObjects[i]->SetVisible( false );
		m_pAlreadyBuiltObjects[i]->SetVisible( false );
		m_pCantAffordObjects[i]->SetVisible( false );

		// If the building is already built
		if ( pObj != NULL && !pObj->IsPlacing() )
		{
			m_pAlreadyBuiltObjects[i]->SetVisible( true );
		}
		// See if we can afford it
		else if ( iAccount < GetObjectInfo( iRemappedObjectID )->m_Cost )
		{
			m_pCantAffordObjects[i]->SetVisible( true );
		}
		else
		{
			// we can buy it
			m_pAvailableObjects[i]->SetVisible( true );
		}
	}
}

void CHudMenuEngyBuild::SetVisible( bool state )
{
	if ( state == true )
	{
		// close the weapon selection menu
		engine->ClientCmd( "cancelselect" );

		// set the %lastinv% dialog var to our binding
		const char *key = engine->Key_LookupBinding( "lastinv" );
		if ( !key )
		{
			key = "< not bound >";
		}

		SetDialogVariable( "lastinv", key );

		// Set selection to the first available building that we can build

		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

		if ( !pLocalPlayer )
			return;

		int iDefaultSlot = 1;

		// Find the first slot that represents a building that we haven't built
		int iSlot;
		for ( iSlot = 1; iSlot <= 4; iSlot++ )
		{
			int iBuilding = MapIndexToObjectID( iSlot );
			C_BaseObject *pObj = pLocalPlayer->GetObjectOfType( iBuilding );

			if ( pObj == NULL )
			{
				iDefaultSlot = iSlot;
				break;
			}
		}
	}

	BaseClass::SetVisible( state );
}