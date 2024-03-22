//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_hud_mediccallers.h"
#include "iclientmode.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include "view.h"
#include "ivieweffects.h"
#include "viewrender.h"
#include "prediction.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MEDICCALLER_WIDE		(XRES(56))
#define MEDICCALLER_TALL		(YRES(30))
#define MEDICCALLER_ARROW_WIDE	(XRES(16))
#define MEDICCALLER_ARROW_TALL	(YRES(24))

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFMedicCallerPanel::CTFMedicCallerPanel( Panel *parent, const char *name ) : EditablePanel(parent,name)
{
	m_hPlayer = NULL;
	m_pArrowMaterial = NULL;
	m_bDrawLeftArrow = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFMedicCallerPanel::~CTFMedicCallerPanel( void )
{
	if ( m_pArrowMaterial )
	{
		m_pArrowMaterial->DecrementReferenceCount();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CTFMedicCallerPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/MedicCallerPanel.res" );

	if ( m_pArrowMaterial )
	{
		m_pArrowMaterial->DecrementReferenceCount();
	}
	m_pArrowMaterial = materials->FindMaterial( "HUD/medic_arrow", TEXTURE_GROUP_VGUI );
	m_pArrowMaterial->IncrementReferenceCount();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMedicCallerPanel::GetCallerPosition( const Vector &vecDelta, float flRadius, float *xpos, float *ypos, float *flRotation )
{
	// Player Data
	Vector playerPosition = MainViewOrigin();
	QAngle playerAngles = MainViewAngles();

	Vector forward, right, up(0,0,1);
	AngleVectors (playerAngles, &forward, NULL, NULL );
	forward.z = 0;
	VectorNormalize(forward);
	CrossProduct( up, forward, right );
	float front = DotProduct(vecDelta, forward);
	float side = DotProduct(vecDelta, right);
	*xpos = flRadius * -side;
	*ypos = flRadius * -front;

	// Get the rotation (yaw)
	*flRotation = atan2(*xpos,*ypos) + M_PI;
	*flRotation *= 180 / M_PI;

	float yawRadians = -(*flRotation) * M_PI / 180.0f;
	float ca = cos( yawRadians );
	float sa = sin( yawRadians );

	// Rotate it around the circle
	*xpos = (int)((ScreenWidth() / 2) + (flRadius * sa));
	*ypos = (int)((ScreenHeight() / 2) - (flRadius * ca));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMedicCallerPanel::OnTick( void )
{
	if ( !m_hPlayer || !m_hPlayer->IsAlive() || gpGlobals->curtime > m_flRemoveAt )
	{
		MarkForDeletion();
		return;
	}

	// If the local player has started healing this guy, remove it too.
	//Also don't draw it if we're dead.
	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalTFPlayer )
	{
		CBaseEntity *pHealTarget = pLocalTFPlayer->MedicGetHealTarget();
		if ( (pHealTarget && pHealTarget == m_hPlayer) || pLocalTFPlayer->IsAlive() == false )
		{
			MarkForDeletion();
			return;
		}

		// If we're pointing to an enemy spy and they are no longer disguised, remove ourselves
		if ( m_hPlayer->IsPlayerClass( TF_CLASS_SPY ) && 
			!( m_hPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && m_hPlayer->m_Shared.GetDisguiseTeam() == pLocalTFPlayer->GetTeamNumber() ) )
		{
			MarkForDeletion();
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMedicCallerPanel::Paint( void )
{
	BaseClass::Paint();

	float uA,uB,yA,yB;
	int x,y;
	GetPos( x,y );
	if ( m_bDrawLeftArrow )
	{
		uA = 1.0;
		uB = 0.0;
		yA = 0.0;
		yB = 1.0;
	}
	else
	{
		uA = 0.0;
		uB = 1.0;
		yA = 0.0;
		yB = 1.0;
		x += GetWide() - MEDICCALLER_ARROW_WIDE;
	}

	int iyindent = (GetTall() - MEDICCALLER_ARROW_TALL) * 0.5;
	y += iyindent;

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( m_pArrowMaterial );
	IMesh* pMesh = pRenderContext->GetDynamicMesh( true );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Position3f( x, y, 0.0f );
	meshBuilder.TexCoord2f( 0, uA, yA );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( x + MEDICCALLER_ARROW_WIDE, y, 0.0f );
	meshBuilder.TexCoord2f( 0, uB, yA );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( x + MEDICCALLER_ARROW_WIDE, y + MEDICCALLER_ARROW_TALL, 0.0f );
	meshBuilder.TexCoord2f( 0, uB, yB );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( x, y + MEDICCALLER_ARROW_TALL, 0.0f );
	meshBuilder.TexCoord2f( 0, uA, yB );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMedicCallerPanel::SetPlayer( C_TFPlayer *pPlayer, float flDuration, Vector &vecOffset )
{
	m_hPlayer = pPlayer;
	m_flRemoveAt = gpGlobals->curtime + flDuration;
	m_vecOffset = vecOffset;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMedicCallerPanel::AddMedicCaller( C_TFPlayer *pPlayer, float flDuration, Vector &vecOffset )
{
	CTFMedicCallerPanel *pCaller = new CTFMedicCallerPanel( g_pClientMode->GetViewport(), "MedicCallerPanel" );
	vgui::SETUP_PANEL(pCaller);
	pCaller->SetBounds( 0,0, MEDICCALLER_WIDE, MEDICCALLER_TALL );
	pCaller->SetPlayer( pPlayer, flDuration, vecOffset );
	pCaller->SetVisible( true );
	vgui::ivgui()->AddTickSignal( pCaller->GetVPanel() );
}
