//========= Copyright © 1996-2007, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_ROUNDINFO_H
#define TF_ROUNDINFO_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: displays the RoundInfo menu
//-----------------------------------------------------------------------------
class CTFRoundInfo : public vgui::Frame, public IViewPortPanel, public CGameEventListener
{
private:
	DECLARE_CLASS_SIMPLE( CTFRoundInfo, vgui::Frame );

public:
	CTFRoundInfo( IViewPort *pViewPort );

	virtual const char *GetName( void ){ return PANEL_ROUNDINFO; }
	virtual void SetData( KeyValues *data );
	virtual void Reset(){ Update(); }
	virtual void Update();
	virtual bool NeedsUpdate( void ){ return false; }
	virtual bool HasInputElements( void ){ return true; }
	virtual void ShowPanel( bool bShow );

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ){ return BaseClass::GetVPanel(); }
	virtual bool IsVisible(){ return BaseClass::IsVisible(); }
	virtual void SetParent( vgui::VPANEL parent ){ BaseClass::SetParent( parent ); }

	virtual void FireGameEvent( IGameEvent *event );

protected:
	virtual void OnKeyCodePressed( vgui::KeyCode code );
	virtual void OnKeyCodeReleased( vgui::KeyCode code );
	virtual void PerformLayout();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnCommand( const char *command );

    void SetData( const char *pszTitle, const char *pszRedImage, const char *pszBlueImage, const char *pszStateImage );
	void UpdateImage( vgui::ImagePanel *pImagePanel, const char *pszImageName );

protected:
	IViewPort			*m_pViewPort;

	CTFLabel			*m_pTitle;
	vgui::ImagePanel	*m_pMapImage;
	vgui::ImagePanel	*m_pRoundImage;
	vgui::ImagePanel	*m_pStateImage;
	
#ifdef _X360
	CTFFooter			*m_pFooter;
#else
	CTFButton			*m_pContinue;
#endif
	
    ButtonCode_t		m_iRoundInfoKey;
    
	char				m_szTitle[255];

	char				m_szMapImage[MAX_ROUND_IMAGE_NAME];
    char				m_szRedImage[MAX_ROUND_IMAGE_NAME];
    char				m_szBlueImage[MAX_ROUND_IMAGE_NAME];
    char				m_szStateImage[MAX_ROUND_IMAGE_NAME];
};


#endif // TF_ROUNDINFO_H
