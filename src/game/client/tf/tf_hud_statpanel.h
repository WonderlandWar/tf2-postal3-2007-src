//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TFSTATPANEL_H
#define TFSTATPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <game/client/iviewport.h>
#include <vgui/IScheme.h>
#include "hud.h"
#include "hudelement.h"
#include "../game/shared/tf/tf_shareddefs.h"
#include "../game/shared/tf/tf_gamestats_shared.h"
#include "tf_hud_playerstatus.h"

using namespace vgui;

enum PlayerStatsVersions_t
{
	PLAYERSTATS_FILE_VERSION
};

struct ClassStats_t
{
	int					iPlayerClass;		// which class these stats refer to
	int					iNumberOfRounds;	// how many times player has played this class
	RoundStats_t		accumulated;
	RoundStats_t		max;

	ClassStats_t()
	{
		iPlayerClass	= TF_CLASS_UNDEFINED;
		iNumberOfRounds = 0;
	}

	void AccumulateRound( const RoundStats_t &other )
	{
		iNumberOfRounds++;
		accumulated.AccumulateRound( other );
	}
};

enum RecordBreakType_t
{
	RECORDBREAK_NONE,
	RECORDBREAK_CLOSE,
	RECORDBREAK_TIE,
	RECORDBREAK_BEST,

	RECORDBREAK_MAX
};

class C_TFPlayer;

class CTFStatPanel : public EditablePanel, public CHudElement
{
private:
	DECLARE_CLASS_SIMPLE( CTFStatPanel, EditablePanel );

public:
	CTFStatPanel( const char *pElementName );
	virtual ~CTFStatPanel();

	virtual void Reset();
	virtual void Init();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void FireGameEvent( IGameEvent * event );
	virtual void OnTick();
	void Show();
	void Hide();
	virtual bool ShouldDraw( void );

	void		ShowStatPanel( int iClassIndex, int iTeam, int iCurStatValue, TFStatType_t statType, RecordBreakType_t recordBreakType );
	void		TestStatPanel( TFStatType_t statType );

	void		WriteStats( void );
	bool		ReadStats( void );
	void		ResetStats( void );

private:
	void		GetStatValueAsString( int iValue, TFStatType_t statType, char *value, int valuelen );
	void		UpdateStats( int iClass, RoundStats_t &stats, bool bShowNow );
	void		ResetDisplayedStat();

	int							m_iCurStatValue;			// the value of the currently displayed stat
	int							m_iCurStatClassIndex;		// the player class for current stat
	int							m_iCurStatTeam;				// the team of current stat
	TFStatType_t				m_statType;					// which stat broke a record
	RecordBreakType_t			m_recordBreakType;			// was record broken, tied, or just close
	bool						m_bDisplayAfterSpawn;
	float						m_flTimeHide;				// time at which to hide the panel

	CUtlVector<ClassStats_t>	m_aClassStats;
	bool						m_bStatsChanged;
	CTFClassImage				*m_pClassImage;
};

CTFStatPanel *GetStatPanel();

#endif //TFSTATPANEL_H