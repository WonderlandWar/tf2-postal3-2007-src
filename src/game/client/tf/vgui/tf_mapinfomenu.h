//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_MAPINFOMENU_H
#define TF_MAPINFOMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include "vgui_controls/KeyRepeat.h"

//-----------------------------------------------------------------------------
// Purpose: displays the MapInfo menu
//-----------------------------------------------------------------------------

class CTFMapInfoMenu : public vgui::Frame, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE( CTFMapInfoMenu, vgui::Frame );

public:
	CTFMapInfoMenu( IViewPort *pViewPort );
	virtual ~CTFMapInfoMenu();

	virtual const char *GetName( void ){ return PANEL_MAPINFO; }
	virtual void SetData( KeyValues *data ){}
	virtual void Reset(){ Update(); }
	virtual void Update();
	virtual bool NeedsUpdate( void ){ return false; }
	virtual bool HasInputElements( void ){ return true; }
	virtual void ShowPanel( bool bShow );

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ){ return BaseClass::GetVPanel(); }
	virtual bool IsVisible(){ return BaseClass::IsVisible(); }
	virtual void SetParent( vgui::VPANEL parent ){ BaseClass::SetParent( parent ); }

protected:
	virtual void OnKeyCodePressed(vgui::KeyCode code);
	virtual void PerformLayout( void );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnCommand( const char *command );
	
private:
	// helper functions
	void LoadMapPage( const char *mapName );
	void SetMapTitle();
	bool CheckForIntroMovie();
	void CheckIntroButton();

protected:
	IViewPort			*m_pViewPort;
	CTFLabel			*m_pTitle;
	CTFRichText			*m_pMapInfo;
	vgui::HTML			*m_pMapInfoHTML;

#ifdef _X360
	CTFFooter			*m_pFooter;
#else
	CTFButton			*m_pContinue;
	CTFButton			*m_pBack;
	CTFButton			*m_pIntro;
#endif

	vgui::ImagePanel	*m_pMapImage;

	char				m_szMapName[MAX_PATH];
};


#endif // TF_MAPINFOMENU_H
