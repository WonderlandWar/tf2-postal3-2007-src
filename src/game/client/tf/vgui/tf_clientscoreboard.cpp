//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include <tier1/fmtstr.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/Frame.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVgui.h>
#include <vgui_controls/SectionedListPanel.h>
#include <vgui_controls/ImageList.h>
#include <game/client/iviewport.h>
#include <KeyValues.h>
#include <FileSystem.h>
#include "IGameUIFuncs.h" // for key bindings

#include "tf_controls.h"
#include "tf_shareddefs.h"
#include "tf_clientscoreboard.h"
#include "tf_gamestats_shared.h"
#include "tf_hud_statpanel.h"
#include "c_playerresource.h"
#include "c_tf_playerresource.h"
#include "c_tf_team.h"
#include "c_tf_player.h"
#include "vgui_avatarimage.h"

#if defined ( _X360 )
#include "engine/imatchmaking.h"
#endif

using namespace vgui;

#define SCOREBOARD_MAX_LIST_ENTRIES 12

extern bool AvatarIndexLessFunc( const int &lhs, const int &rhs );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFClientScoreBoardDialog::CTFClientScoreBoardDialog( IViewPort *pViewPort ) : CClientScoreBoardDialog( pViewPort )
{
	SetProportional(true);
	SetKeyBoardInputEnabled(false);
	SetMouseInputEnabled(false);
	SetScheme( "ClientScheme" );

	m_pPlayerListBlue = new SectionedListPanel( this, "BluePlayerList" );
	m_pPlayerListRed = new SectionedListPanel( this, "RedPlayerList" );
	m_pLabelPlayerName = new CTFLabel( this, "PlayerNameLabel", "" );
	m_pImagePanelHorizLine = new ImagePanel( this, "HorizontalLine" );
	m_pClassImage = new CTFClassImage( this, "ClassImage" );
	m_iImageDead = 0;
	m_iImageDominated = 0;
	m_iImageNemesis = 0;
	
	ListenForGameEvent( "server_spawn" );

	SetDialogVariable( "server", "" );

	// Doesn't show in IDA
	//SetVisible( false );

	m_pImageList = NULL;

	m_mapAvatarsToImageList.SetLessFunc( AvatarIndexLessFunc );
	m_mapAvatarsToImageList.RemoveAll();
	memset( &m_iImageAvatars, 0, sizeof(int) * (MAX_PLAYERS+1) );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFClientScoreBoardDialog::~CTFClientScoreBoardDialog()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::PerformLayout()
{
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings("Resource/UI/scoreboard.res");
	
	if ( m_pImageList )
		delete m_pImageList;
	m_pImageList = new ImageList( false );

	m_mapAvatarsToImageList.RemoveAll();
	memset( &m_iImageAvatars, 0, sizeof(int) * (MAX_PLAYERS+1) );

	m_iImageDead = m_pImageList->AddImage( scheme()->GetImage( "../hud/leaderboard_dead", true ) );
	m_iImageDominated = m_pImageList->AddImage( scheme()->GetImage( "../hud/leaderboard_dominated", true ) );
	m_iImageNemesis = m_pImageList->AddImage( scheme()->GetImage( "../hud/leaderboard_nemesis", true ) );
		
	// resize the images to our resolution
	for (int i = 1; i < m_pImageList->GetImageCount(); i++ )
	{
		int wide = 13, tall = 13;
		m_pImageList->GetImage(i)->SetSize(scheme()->GetProportionalScaledValueEx( GetScheme(), wide ), scheme()->GetProportionalScaledValueEx( GetScheme(),tall ) );
	}

	SetPlayerListImages( m_pPlayerListBlue );
	SetPlayerListImages( m_pPlayerListRed );

	SetBgColor( Color( 0, 0, 0, 0) );
	SetBorder( NULL );
	SetVisible( false );

	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::ShowPanel( bool bShow )
{
	if ( IsVisible() == bShow )
	{
		return;
	}

	if ( bShow )
	{		
		SetVisible( true );
		MoveToFront();
	}
	else
	{
		SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Resets the scoreboard panel
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::Reset()
{
	InitPlayerList( m_pPlayerListBlue );
	InitPlayerList( m_pPlayerListRed );
}

//-----------------------------------------------------------------------------
// Purpose: Inits the player list in a list panel
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::InitPlayerList( SectionedListPanel *pPlayerList )
{
	pPlayerList->SetVerticalScrollbar( false );
	pPlayerList->RemoveAll();
	pPlayerList->RemoveAllSections();
	pPlayerList->AddSection( 0, "Players", TFPlayerSortFunc );
	pPlayerList->SetSectionAlwaysVisible( 0, true );
	pPlayerList->SetSectionFgColor( 0, Color( 255, 255, 255, 255 ) );
	pPlayerList->SetBgColor( Color( 0, 0, 0, 0 ) );
	pPlayerList->SetBorder( NULL );

	// Avatars are always displayed at 32x32 regardless of resolution
	int nAvatarWidth = scheme()->GetProportionalScaledValueEx( GetScheme(), 34 );
	pPlayerList->AddColumnToSection( 0, "avatar", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_CENTER, nAvatarWidth );
	
	int nNameWidth = scheme()->GetProportionalScaledValueEx( GetScheme(), 136 );
	pPlayerList->AddColumnToSection( 0, "name", "#TF_Scoreboard_Name", 0, nNameWidth - nAvatarWidth );
	
	int nStatusWidth = scheme()->GetProportionalScaledValueEx( GetScheme(), 12 );
	pPlayerList->AddColumnToSection( 0, "status", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_CENTER, nStatusWidth );
	
	int nNemesisWidth = scheme()->GetProportionalScaledValueEx( GetScheme(), 20 );
	pPlayerList->AddColumnToSection( 0, "nemesis", "", SectionedListPanel::COLUMN_IMAGE, nNemesisWidth );

	int nClassWidth = scheme()->GetProportionalScaledValueEx( GetScheme(), 35 );
	pPlayerList->AddColumnToSection( 0, "class", "", 0, nClassWidth );

	int nScoreWidth = scheme()->GetProportionalScaledValueEx( GetScheme(), 35 );
	pPlayerList->AddColumnToSection( 0, "score", "#TF_Scoreboard_Score", SectionedListPanel::COLUMN_RIGHT, nScoreWidth );

	int nPingWidth = scheme()->GetProportionalScaledValueEx( GetScheme(), 23 );
	pPlayerList->AddColumnToSection( 0, "ping", "#TF_Scoreboard_Ping", SectionedListPanel::COLUMN_RIGHT, nPingWidth );
}

//-----------------------------------------------------------------------------
// Purpose: Builds the image list to use in the player list
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::SetPlayerListImages( vgui::SectionedListPanel *pPlayerList )
{
	pPlayerList->SetImageList( m_pImageList, false );
	pPlayerList->SetVisible( true );
}

//-----------------------------------------------------------------------------
// Purpose: Updates the dialog
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::Update()
{
	UpdateTeamInfo();
	UpdatePlayerList();
	UpdateSpectatorList();
	UpdatePlayerDetails();
	MoveToCenterOfScreen();

	// update every second
	m_fNextUpdateTime = gpGlobals->curtime + 1.0f; 
}

//-----------------------------------------------------------------------------
// Purpose: Updates information about teams
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::UpdateTeamInfo()
{
	// update the team sections in the scoreboard
	for ( int teamIndex = TF_TEAM_RED; teamIndex <= TF_TEAM_BLUE; teamIndex++ )
	{
		wchar_t *teamName = NULL;
		C_Team *team = GetGlobalTeam( teamIndex );
		if ( team )
		{
			// choose dialog variables to set depending on team
				const char *pDialogVarTeamScore = NULL;
			const char *pDialogVarTeamPlayerCount = NULL;
			switch ( teamIndex ) {
				case TF_TEAM_RED:
					pDialogVarTeamScore = "redteamscore";
					pDialogVarTeamPlayerCount = "redteamplayercount";
					break;
				case TF_TEAM_BLUE:
					pDialogVarTeamScore = "blueteamscore";
					pDialogVarTeamPlayerCount = "blueteamplayercount";
					break;
				default:
					Assert( false );
					break;
			}

			// update # of players on each team
			wchar_t name[64];
			wchar_t string1[1024];
			wchar_t wNumPlayers[6];
			_snwprintf( wNumPlayers, ARRAYSIZE( wNumPlayers ), L"%i", team->Get_Number_Players() );
			if ( !teamName && team )
			{
				g_pVGuiLocalize->ConvertANSIToUnicode( team->Get_Name(), name, sizeof( name ) );
				teamName = name;
			}
			if ( team->Get_Number_Players() == 1 )
			{
				g_pVGuiLocalize->ConstructString( string1, sizeof(string1), g_pVGuiLocalize->Find( "#TF_ScoreBoard_Player" ), 1, wNumPlayers );
			}
			else
			{
				g_pVGuiLocalize->ConstructString( string1, sizeof(string1), g_pVGuiLocalize->Find( "#TF_ScoreBoard_Players" ), 1, wNumPlayers );
			}

			// set # of players for team in dialog
			SetDialogVariable( pDialogVarTeamPlayerCount, string1 );

			// set team score in dialog
			SetDialogVariable( pDialogVarTeamScore, team->Get_Score() );			
		}
	}
}

bool AreEnemyTeams( int iTeam1, int iTeam2 )
{
	if ( iTeam1 == TF_TEAM_RED && iTeam2 == TF_TEAM_BLUE )
		return true;

	if ( iTeam1 == TF_TEAM_BLUE && iTeam2 == TF_TEAM_RED )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Updates the player list
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::UpdatePlayerList()
{
	m_pPlayerListRed->RemoveAll();
	m_pPlayerListBlue->RemoveAll();

	C_TF_PlayerResource *tf_PR = dynamic_cast<C_TF_PlayerResource *>( g_PR );
	if ( !tf_PR )
		return;
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	int localteam = pLocalPlayer->GetTeamNumber();

	for( int playerIndex = 1 ; playerIndex <= MAX_PLAYERS; playerIndex++ )
	{
		if( g_PR->IsConnected( playerIndex ) )
		{
			SectionedListPanel *pPlayerList = NULL;
			switch ( g_PR->GetTeam( playerIndex ) )
			{
			case TF_TEAM_BLUE:
				pPlayerList = m_pPlayerListBlue;
				break;
			case TF_TEAM_RED:
				pPlayerList = m_pPlayerListRed;
				break;
			}
			if ( null == pPlayerList )
				continue;			

			const char *szName = tf_PR->GetPlayerName( playerIndex );
			int score = tf_PR->GetTotalScore( playerIndex );

			KeyValues *pKeyValues = new KeyValues( "data" );

			pKeyValues->SetInt( "playerIndex", playerIndex );
			pKeyValues->SetString( "name", szName );
			pKeyValues->SetInt( "score", score );

			// can only see class information if we're on the same team
			if ( !AreEnemyTeams( g_PR->GetTeam( playerIndex ), localteam ) && !( localteam == TEAM_UNASSIGNED ) )
			{
				// class name
				if( g_PR->IsConnected( playerIndex ) )
				{
						int iClass = tf_PR->GetPlayerClass( playerIndex );
					if ( GetLocalPlayerIndex() == playerIndex && !tf_PR->IsAlive( playerIndex ) ) {
						// If this is local player and he is dead, show desired class (which he will spawn as) rather than current class.
						C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
						int iDesiredClass = pPlayer->m_Shared.GetDesiredPlayerClassIndex();
						// use desired class unless it's random -- if random, his future class is not decided until moment of spawn
						if ( TF_CLASS_RANDOM != iDesiredClass )
						{
							iClass = iDesiredClass;
						}
					} 
					else 
					{
						// for non-local players, show the current class
						iClass = tf_PR->GetPlayerClass( playerIndex );
					}

					if( iClass >= TF_FIRST_NORMAL_CLASS && iClass <= TF_LAST_NORMAL_CLASS )
					{
						pKeyValues->SetString( "class", g_aPlayerClassNames[iClass] );
					}
					else
					{
						pKeyValues->SetString( "class", "" );
					}
				}				
			}
			else
			{
				C_TFPlayer *pPlayerOther = ToTFPlayer( UTIL_PlayerByIndex( playerIndex ) );

#ifdef _X360
				bool bUseTruncatedNames = true;
#else
				bool bUseTruncatedNames = false;
#endif

				if ( pPlayerOther && pPlayerOther->m_Shared.IsPlayerDominated( pLocalPlayer->entindex() ) )
				{
					// if local player is dominated by this player, show a nemesis icon
					pKeyValues->SetInt( "nemesis", m_iImageNemesis );
					pKeyValues->SetString( "class", bUseTruncatedNames ? "#TF_Nemesis_lodef" : "#TF_Nemesis" );
				}
				else if ( pLocalPlayer->m_Shared.IsPlayerDominated( playerIndex) )
				{
					// if this player is dominated by the local player, show the domination icon
					pKeyValues->SetInt( "nemesis", m_iImageDominated );
					pKeyValues->SetString( "class", bUseTruncatedNames ? "#TF_Dominated_lodef" : "#TF_Dominated" );
				}
			}

			// display whether player is alive or dead (all players see this for all other players on both teams)
			pKeyValues->SetInt( "status", tf_PR->IsAlive( playerIndex ) ?  0 : m_iImageDead );

			if ( g_PR->GetPing( playerIndex ) < 1 )
			{
				if ( g_PR->IsFakePlayer( playerIndex ) )
				{
					pKeyValues->SetString( "ping", "#TF_Scoreboard_Bot" );
				}
				else
				{
					pKeyValues->SetString( "ping", "" );
				}
			}
			else
			{
				pKeyValues->SetInt( "ping", g_PR->GetPing( playerIndex ) );
			}
			
			
			// Update their avatar
			if ( pKeyValues && steamapicontext->SteamFriends() && steamapicontext->SteamUtils() )
			{
				player_info_t pi;
				if ( engine->GetPlayerInfo( playerIndex, &pi ) )
				{
					if ( pi.friendsID )
					{
						CSteamID steamIDForPlayer( pi.friendsID, 1, steamapicontext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual );

						// See if the avatar's changed
						int iAvatar = steamapicontext->SteamFriends()->GetFriendAvatar( steamIDForPlayer );
						if ( m_iImageAvatars[playerIndex] != iAvatar )
						{
							m_iImageAvatars[playerIndex] = iAvatar;

							// Now see if we already have that avatar in our list
							int iIndex = m_mapAvatarsToImageList.Find( iAvatar );
							if ( iIndex == m_mapAvatarsToImageList.InvalidIndex() )
							{
								CAvatarImage *pImage = new CAvatarImage();
								pImage->SetAvatarSteamID( steamIDForPlayer );
								pImage->SetSize( 32, 32 );	// Deliberately non scaling
								int iImageIndex = m_pImageList->AddImage( pImage );

								m_mapAvatarsToImageList.Insert( iAvatar, iImageIndex );
							}
						}

						int iIndex = m_mapAvatarsToImageList.Find( iAvatar );

						if ( iIndex != m_mapAvatarsToImageList.InvalidIndex() )
						{
							pKeyValues->SetInt( "avatar", m_mapAvatarsToImageList[iIndex] );
						}
					}
				}
			}
			
			int itemID = pPlayerList->AddItem( 0, pKeyValues );
			Color clr = g_PR->GetTeamColor( g_PR->GetTeam( playerIndex ) );
			pPlayerList->SetItemFgColor( itemID, clr );

			if ( GetLocalPlayerIndex() == playerIndex )
			{
				pPlayerList->SetSelectedItem( itemID );
			}

			pKeyValues->deleteThis();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the spectator list
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::UpdateSpectatorList()
{
	char szSpectatorList[512] = "" ;
	int nSpectators = 0;
	for( int playerIndex = 1 ; playerIndex <= MAX_PLAYERS; playerIndex++ )
	{
		if ( ShouldShowAsSpectator( playerIndex ) )
		{
			if ( nSpectators > 0 )
			{
				Q_strncat( szSpectatorList, ", ", ARRAYSIZE( szSpectatorList ) );
			}

			Q_strncat( szSpectatorList, g_PR->GetPlayerName( playerIndex ), ARRAYSIZE( szSpectatorList ) );
			nSpectators++;
		}
	}

	wchar_t wzSpectators[512] = L"";
	memset( szSpectatorList, 0, sizeof( szSpectatorList ) );
	if ( nSpectators > 0 )
	{
		const char *pchFormat = ( 1 == nSpectators ? "#ScoreBoard_Spectator" : "#ScoreBoard_Spectators" );

		wchar_t wzSpectatorCount[16];
		wchar_t wzSpectatorList[1024];
		_snwprintf( wzSpectatorCount, ARRAYSIZE( wzSpectatorCount ), L"%i", nSpectators );
		g_pVGuiLocalize->ConvertANSIToUnicode( szSpectatorList, wzSpectatorList, sizeof( wzSpectatorList ) );
		g_pVGuiLocalize->ConstructString( wzSpectators, sizeof(wzSpectators), g_pVGuiLocalize->Find( pchFormat), 2, wzSpectatorCount, wzSpectatorList );
	}
	SetDialogVariable( "spectators", wzSpectators );
}

//-----------------------------------------------------------------------------
// Purpose: Updates details about a player
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::UpdatePlayerDetails()
{
	ClearPlayerDetails();

	C_TF_PlayerResource *tf_PR = dynamic_cast<C_TF_PlayerResource *>( g_PR );
	if ( !tf_PR )
		return;
	
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;
	int playerIndex = pLocalPlayer->entindex();

	// Make sure the selected player is still connected. 
	if ( !tf_PR->IsConnected( playerIndex ) ) 
		return;

	SetDialogVariable( "kills", tf_PR->GetPlayerScore( playerIndex ) );
	SetDialogVariable( "deaths", tf_PR->GetDeaths( playerIndex ) );
	SetDialogVariable( "assists",tf_PR->GetKillAssists( playerIndex ) );
	SetDialogVariable( "destruction", tf_PR->GetBuildingsDestroyed( playerIndex ) );
	SetDialogVariable( "captures", tf_PR->GetCaptures( playerIndex ) );
	SetDialogVariable( "defenses", tf_PR->GetDefenses( playerIndex ) );
	SetDialogVariable( "dominations", tf_PR->GetDominations( playerIndex ) );
	SetDialogVariable( "revenge", tf_PR->GetRevenge( playerIndex ) );
	SetDialogVariable( "healing", tf_PR->GetHealPoints( playerIndex ) );
	SetDialogVariable( "invulns", tf_PR->GetInvulns( playerIndex ) );
	SetDialogVariable( "teleports", tf_PR->GetTeleports( playerIndex ) );
	SetDialogVariable( "headshots", tf_PR->GetHeadshots( playerIndex ) );
	SetDialogVariable( "backstabs", tf_PR->GetBackstabs( playerIndex ) );
	SetDialogVariable( "playername", tf_PR->GetPlayerName( playerIndex ) );
	SetDialogVariable( "playerscore", GetPointsString( tf_PR->GetTotalScore( playerIndex ) ) );
	Color clr = g_PR->GetTeamColor( g_PR->GetTeam( playerIndex ) );
	m_pLabelPlayerName->SetFgColor( clr );
	m_pImagePanelHorizLine->SetFillColor( clr );

	int iClass = pLocalPlayer->m_Shared.GetDesiredPlayerClassIndex();
	int iTeam = pLocalPlayer->GetTeamNumber();
	if ( ( iTeam >= FIRST_GAME_TEAM ) && ( iClass >= TF_FIRST_NORMAL_CLASS ) && ( iClass <= TF_LAST_NORMAL_CLASS ) )
	{
		m_pClassImage->SetClass( iTeam, iClass, 0 );
		m_pClassImage->SetVisible( true );
	} 
	else
	{
		m_pClassImage->SetVisible( false );
	}

	// tfp3: The SetVisible call is strange, there should only be one but there isn't an arg in it!
}

//-----------------------------------------------------------------------------
// Purpose: Clears score details
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::ClearPlayerDetails()
{
	m_pClassImage->SetVisible( false );

	SetDialogVariable( "kills", "" ); 
	SetDialogVariable( "deaths", "" );
	SetDialogVariable( "captures", "" );
	SetDialogVariable( "defenses", "" );
	SetDialogVariable( "dominations", "" );
	SetDialogVariable( "revenge", "" );
	SetDialogVariable( "assists", "" );
	SetDialogVariable( "destruction", "" );
	SetDialogVariable( "healing", "" );
	SetDialogVariable( "invulns", "" );
	SetDialogVariable( "teleports", "" );
	SetDialogVariable( "headshots", "" );
	SetDialogVariable( "backstabs", "" );
	SetDialogVariable( "playername", "" );
	SetDialogVariable( "playerscore", "" );
}

//-----------------------------------------------------------------------------
// Purpose: Used for sorting players
//-----------------------------------------------------------------------------
bool CTFClientScoreBoardDialog::TFPlayerSortFunc( vgui::SectionedListPanel *list, int itemID1, int itemID2 )
{
	KeyValues *it1 = list->GetItemData(itemID1);
	KeyValues *it2 = list->GetItemData(itemID2);
	Assert(it1 && it2);

	// first compare score
	int v1 = it1->GetInt("score");
	int v2 = it2->GetInt("score");
	if (v1 > v2)
		return true;
	else if (v1 < v2)
		return false;

	// if score is the same, use player index to get deterministic sort
	int iPlayerIndex1 = it1->GetInt( "playerIndex" );
	int iPlayerIndex2 = it2->GetInt( "playerIndex" );
	return ( iPlayerIndex1 > iPlayerIndex2 );
}

//-----------------------------------------------------------------------------
// Purpose: Returns a localized string of form "1 point", "2 points", etc for specified # of points
//-----------------------------------------------------------------------------
const wchar_t *GetPointsString( int iPoints )
{
	wchar_t wzScoreVal[128];
	static wchar_t wzScore[128];
	_snwprintf( wzScoreVal, ARRAYSIZE( wzScoreVal ), L"%i", iPoints );
	if ( 1 == iPoints ) 
	{
		g_pVGuiLocalize->ConstructString( wzScore, sizeof(wzScore), g_pVGuiLocalize->Find( "#TF_ScoreBoard_Point" ), 1, wzScoreVal );
	}
	else
	{
		g_pVGuiLocalize->ConstructString( wzScore, sizeof(wzScore), g_pVGuiLocalize->Find( "#TF_ScoreBoard_Points" ), 1, wzScoreVal );
	}
	return wzScore;
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether the specified player index is a spectator
//-----------------------------------------------------------------------------
bool CTFClientScoreBoardDialog::ShouldShowAsSpectator( int iPlayerIndex )
{
	C_TF_PlayerResource *tf_PR = dynamic_cast<C_TF_PlayerResource *>( g_PR );
	if ( !tf_PR )
		return false;

	// see if player is connected
	if ( tf_PR->IsConnected( iPlayerIndex ) ) 
	{
		// either spectating or unassigned team should show in spectator list
		int iTeam = tf_PR->GetTeam( iPlayerIndex );
		if ( TEAM_SPECTATOR == iTeam || TEAM_UNASSIGNED == iTeam )
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Event handler
//-----------------------------------------------------------------------------
void CTFClientScoreBoardDialog::FireGameEvent( IGameEvent *event )
{
	const char *type = event->GetName();

	if ( 0 == Q_strcmp( type, "server_spawn" ) )
	{		
		// set server name in scoreboard
		const char *hostname = event->GetString( "hostname" );
		wchar_t wzHostName[256];
		wchar_t wzServerLabel[256];
		g_pVGuiLocalize->ConvertANSIToUnicode( hostname, wzHostName, sizeof( wzHostName ) );
		g_pVGuiLocalize->ConstructString( wzServerLabel, sizeof(wzServerLabel), g_pVGuiLocalize->Find( "#TF_Scoreboard_server" ), 1, wzHostName );
		SetDialogVariable( "server", wzServerLabel );
	}

	if( IsVisible() )
	{
		Update();
	}
}
