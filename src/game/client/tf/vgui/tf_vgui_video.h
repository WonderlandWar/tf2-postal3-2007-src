//====== Copyright © 1996-2007, Valve Corporation, All rights reserved. =======
//
// Purpose: VGUI panel which can play back video, in-engine
//
//=============================================================================

#ifndef TF_VGUI_VIDEO_H
#define TF_VGUI_VIDEO_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_video.h"

class CTFVideoPanel : public VideoPanel
{
	DECLARE_CLASS_SIMPLE( CTFVideoPanel, VideoPanel );
public:

	CTFVideoPanel( vgui::Panel *parent, const char *panelName );
	~CTFVideoPanel();

	virtual void OnClose();
	virtual void OnKeyCodePressed( vgui::KeyCode code ){}
	virtual void ApplySettings( KeyValues *inResourceData );
	
	virtual void GetPanelPos( int &xpos, int &ypos );
	virtual void Shutdown();

	float GetDelay(){ return m_flDelay; }

protected:
	virtual void ReleaseVideo();
	virtual void OnVideoOver();

private:
	float m_flDelay;
};

#endif // TF_VGUI_VIDEO_H
