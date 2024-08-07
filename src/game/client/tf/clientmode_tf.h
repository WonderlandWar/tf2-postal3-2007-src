//========= Copyright � 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef TF_CLIENTMODE_H
#define TF_CLIENTMODE_H
#ifdef _WIN32
#pragma once
#endif

#include "clientmode_shared.h"
#include "tf_viewport.h"
#include "GameUI/igameui.h"

class CHudMenuEngyBuild;
class CHudMenuEngyDestroy;
class CHudMenuSpyDisguise;
class CTFFreezePanel;

#if defined( _X360 )
class CTFClientScoreBoardDialog;
#endif

class ClientModeTFNormal : public ClientModeShared 
{
DECLARE_CLASS( ClientModeTFNormal, ClientModeShared );

private:

// IClientMode overrides.
public:

					ClientModeTFNormal();
	virtual			~ClientModeTFNormal();

	virtual void	Init();
	virtual void	InitViewport();
	virtual void	Shutdown();

//	virtual int		KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	virtual float	GetViewModelFOV( void );
	virtual bool	ShouldDrawViewModel();

	int				GetDeathMessageStartHeight( void );

	virtual void	FireGameEvent( IGameEvent *event );
	virtual void	PostRenderVGui();

	virtual void	CreateMove( float flInputSampleTime, CUserCmd *cmd );

	virtual int		HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );
	
private:
	
	//	void	UpdateSpectatorMode( void );

private:

	CHudMenuEngyBuild *m_pMenuEngyBuild;
	CHudMenuEngyDestroy *m_pMenuEngyDestroy;
	CHudMenuSpyDisguise *m_pMenuSpyDisguise;
	CTFFreezePanel		*m_pFreezePanel;
	IGameUI			*m_pGameUI;

#if defined( _X360 )
	CTFClientScoreBoardDialog	*m_pScoreboard;
#endif
};


extern IClientMode *GetClientModeNormal();
extern ClientModeTFNormal* GetClientModeTFNormal();

#endif // TF_CLIENTMODE_H
