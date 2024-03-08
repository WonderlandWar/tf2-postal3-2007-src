//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "cbase.h"
#include "hltvdirector.h"
#include "team_control_point.h"


CBaseEntity* GetCapturePointByIndex( int iCaptureIndex )
{
	CTeamControlPoint *pTeamControlPoint = (CTeamControlPoint *)gEntList.FindEntityByClassname( NULL, "team_control_point" );

	while ( pTeamControlPoint )
	{
		if ( pTeamControlPoint->GetPointIndex() == iCaptureIndex )
		{
			return pTeamControlPoint;
		}

		pTeamControlPoint = (CTeamControlPoint *)gEntList.FindEntityByClassname( pTeamControlPoint, "team_control_point" );
	}

	return NULL;
}

class CTFHLTVDirector : public CHLTVDirector
{
public:
	DECLARE_CLASS( CTFHLTVDirector, CHLTVDirector );

	const char** GetModEvents();
	void SetHLTVServer( IHLTVServer *hltv );
	void CreateShotFromEvent( CGameEvent *event);

	virtual char	*GetFixedCameraEntityName( void ) { return "info_observer_point"; }
};

void CTFHLTVDirector::SetHLTVServer( IHLTVServer *hltv )
{
	BaseClass::SetHLTVServer( hltv );

	if ( m_pHLTVServer )
	{
		// mod specific events the director uses to find interesting shots
		ListenForGameEvent( "teamplay_point_captured" );
		ListenForGameEvent( "teamplay_capture_blocked" );
		ListenForGameEvent( "teamplay_point_startcapture" );
		ListenForGameEvent( "teamplay_flag_event" );
		ListenForGameEvent( "ctf_flag_captured" );
	}
}

void CTFHLTVDirector::CreateShotFromEvent( CGameEvent *event ) 
{
	// show event at least for 2 more seconds after it occured
	const char *name = event->m_Event->GetName();

	if ( !Q_strcmp( "tf_point_captured", name ) || !Q_strcmp( "tf_capture_blocked", name ) )
	{
		int player = event->m_Event->GetInt( "blocker" );
		if ( !player )
			player = event->m_Event->GetInt( "cappers" );
		if( player )
		{
			IGameEvent *shot = gameeventmanager->CreateEvent( "hltv_chase" );
			shot->SetInt( "target1", player );
			shot->SetInt( "target2", 0 );
			shot->SetFloat( "distance", 96.0 );
			shot->SetInt( "theta", 0 ); // left/right
			shot->SetInt( "phi", 20 ); // hi/low
			
			int nNewNextShotTick = event->m_Tick + (int)(2.0 / gpGlobals->interval_per_tick + 0.5);
			if ( m_nNextShotTick < nNewNextShotTick )
				nNewNextShotTick = m_nNextShotTick;

			m_nNextShotTick = nNewNextShotTick;
			
			m_iPVSEntity = player;

			// send spectators the HLTV director command as a game event
			m_pHLTVServer->BroadcastEvent( shot );
			gameeventmanager->FreeEvent( shot );
			DevMsg( "DrcCmd: %s\n", name );
		}
	}
	else
	{
		// let baseclass create a shot
		BaseClass::CreateShotFromEvent( event );
	}
}

const char** CTFHLTVDirector::GetModEvents()
{
	// game events relayed to spectator clients
	static const char *s_modevents[] =
	{
		"game_newmap",
		"hltv_status",
		"hltv_chat",
		"player_connect",
		"player_disconnect",
		"player_changeclass",
		"player_team",
		"player_info",
		"player_death",
		"player_chat",
		"player_spawn",
		"round_start",
		"round_end",
		"server_cvar",
		"server_spawn",
				
		// additional TF events:
		"controlpoint_starttouch",
		"controlpoint_endtouch",
		"ctf_flag_captured",
		"teamplay_broadcast_audio",
		"teamplay_capture_blocked",
		"teamplay_flag_event",
		"teamplay_game_over",
		"teamplay_point_captured",
		"teamplay_round_stalemate",
		"teamplay_round_start",
		"teamplay_round_win",
		"teamplay_timer_time_added",
		"teamplay_update_timer",
		"teamplay_win_panel",
		"tf_game_over",
		"object_destroyed",
			
		NULL
	};

	return s_modevents;
}

static CTFHLTVDirector s_HLTVDirector;	// singleton

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CHLTVDirector, IHLTVDirector, INTERFACEVERSION_HLTVDIRECTOR, s_HLTVDirector );

CHLTVDirector* HLTVDirector()
{
	return &s_HLTVDirector;
}

IGameSystem* HLTVDirectorSystem()
{
	return &s_HLTVDirector;
}