
//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "winerror.h"
#include "tf_hud_statpanel.h"
#include "tf_hud_winpanel.h"
#include <vgui/IVGUI.h>
#include "vgui_controls/AnimationController.h"
#include "iclientmode.h"
#include "c_tf_playerresource.h"
#include <vgui_controls/Label.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "tf/c_tf_player.h"
#include "tf/c_tf_team.h"
#include "FileSystem.h"
#include "dmxloader/dmxloader.h"
#include "fmtstr.h"
#include "tf_statsummary.h"
#include "usermessages.h"
#include "hud_macros.h"
#include "ixboxsystem.h"
#include "achievementmgr.h"
#include "tf_hud_freezepanel.h"
#include "tf_gamestats_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT_DEPTH( CTFStatPanel, 1 );

BEGIN_DMXELEMENT_UNPACK( RoundStats_t )
	DMXELEMENT_UNPACK_FIELD( "iNumShotsHit", "0", int, m_iStat[TFSTAT_SHOTS_HIT] )
	DMXELEMENT_UNPACK_FIELD( "iNumShotsFired", "0", int, m_iStat[TFSTAT_SHOTS_FIRED] )
	DMXELEMENT_UNPACK_FIELD( "iNumberOfKills", "0", int, m_iStat[TFSTAT_KILLS] )
	DMXELEMENT_UNPACK_FIELD( "iNumDeaths", "0", int, m_iStat[TFSTAT_DEATHS] )
	DMXELEMENT_UNPACK_FIELD( "iDamageDealt", "0", int, m_iStat[TFSTAT_DAMAGE] )
	DMXELEMENT_UNPACK_FIELD( "iPlayTime", "0", int, m_iStat[TFSTAT_PLAYTIME] )
	DMXELEMENT_UNPACK_FIELD( "iPointCaptures", "0", int, m_iStat[TFSTAT_CAPTURES] )
	DMXELEMENT_UNPACK_FIELD( "iPointDefenses", "0", int, m_iStat[TFSTAT_DEFENSES] )
	DMXELEMENT_UNPACK_FIELD( "iDominations", "0", int, m_iStat[TFSTAT_DOMINATIONS] )
	DMXELEMENT_UNPACK_FIELD( "iRevenge", "0", int, m_iStat[TFSTAT_REVENGE] )
	DMXELEMENT_UNPACK_FIELD( "iPointsScored", "0", int, m_iStat[TFSTAT_POINTSSCORED] )
	DMXELEMENT_UNPACK_FIELD( "iBuildingsDestroyed", "0", int, m_iStat[TFSTAT_BUILDINGSDESTROYED] )
	DMXELEMENT_UNPACK_FIELD( "iHeadshots", "0", int, m_iStat[TFSTAT_HEADSHOTS] )
	DMXELEMENT_UNPACK_FIELD( "iHealthPointsHealed", "0", int, m_iStat[TFSTAT_HEALING] )
	DMXELEMENT_UNPACK_FIELD( "iNumInvulnerable", "0", int, m_iStat[TFSTAT_INVULNS] )
	DMXELEMENT_UNPACK_FIELD( "iKillAssists", "0", int, m_iStat[TFSTAT_KILLASSISTS] )
	DMXELEMENT_UNPACK_FIELD( "iBackstabs", "0", int, m_iStat[TFSTAT_BACKSTABS] )
	DMXELEMENT_UNPACK_FIELD( "iHealthPointsLeached", "0", int, m_iStat[TFSTAT_HEALTHLEACHED] )
	DMXELEMENT_UNPACK_FIELD( "iBuildingsBuilt", "0", int, m_iStat[TFSTAT_BUILDINGSBUILT] )
	DMXELEMENT_UNPACK_FIELD( "iSentryKills", "0", int, m_iStat[TFSTAT_MAXSENTRYKILLS] )
	DMXELEMENT_UNPACK_FIELD( "iNumTeleports", "0", int, m_iStat[TFSTAT_TELEPORTS] )
END_DMXELEMENT_UNPACK( RoundStats_t, s_RoundStatsUnpack )

BEGIN_DMXELEMENT_UNPACK( ClassStats_t )
	DMXELEMENT_UNPACK_FIELD( "iPlayerClass", "0", int, iPlayerClass )
	DMXELEMENT_UNPACK_FIELD( "iNumberOfRounds", "0", int, iNumberOfRounds )
	// RoundStats_t		accumulated;
	// RoundStats_t		max;
END_DMXELEMENT_UNPACK( ClassStats_t, s_ClassStatsUnpack )

// priority order of stats to display record for; earlier position in list is highest
TFStatType_t g_statPriority[] = { TFSTAT_HEADSHOTS, TFSTAT_BACKSTABS, TFSTAT_MAXSENTRYKILLS, TFSTAT_HEALING, TFSTAT_KILLS, TFSTAT_KILLASSISTS,  
	TFSTAT_DAMAGE, TFSTAT_DOMINATIONS, TFSTAT_INVULNS, TFSTAT_BUILDINGSDESTROYED, TFSTAT_CAPTURES, TFSTAT_DEFENSES, TFSTAT_REVENGE, TFSTAT_TELEPORTS, TFSTAT_BUILDINGSBUILT, 
	TFSTAT_HEALTHLEACHED, TFSTAT_POINTSSCORED, TFSTAT_PLAYTIME };
// stat types that we don't display records for, kept in this list just so we can assert all stats appear in one list or the other
TFStatType_t g_statUnused[] = { TFSTAT_DEATHS, TFSTAT_UNDEFINED, TFSTAT_SHOTS_FIRED, TFSTAT_SHOTS_HIT };

// localization keys for stat panel text, must be in same order as TFStatType_t
const char *g_szLocalizedRecordText[] =
{
	"",
	"[shots hit]",
	"[shots fired]",
	"#StatPanel_Kills",	
	"[deaths]",
	"#StatPanel_DamageDealt",
	"#StatPanel_Captures",
	"#StatPanel_Defenses",
	"#StatPanel_Dominations",
	"#StatPanel_Revenge",
	"#StatPanel_PointsScored",
	"#StatPanel_BuildingsDestroyed",
	"#StatPanel_Headshots",
	"#StatPanel_PlayTime",
	"#StatPanel_Healing",
	"#StatPanel_Invulnerable",
	"#StatPanel_KillAssists",
	"#StatPanel_Backstabs",
	"#StatPanel_HealthLeached",
	"#StatPanel_BuildingsBuilt",
	"#StatPanel_SentryKills",		
	"#StatPanel_Teleports"
};


static CTFStatPanel *statPanel = NULL;

//-----------------------------------------------------------------------------
// Purpose: Returns the static stats panel
//-----------------------------------------------------------------------------
CTFStatPanel *GetStatPanel()
{
	return statPanel;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFStatPanel::CTFStatPanel( const char *pElementName )
	: EditablePanel( NULL, "StatPanel" ), CHudElement( pElementName )
{
	// Assert that all defined stats are in our prioritized list or explicitly unused
	Assert( ARRAYSIZE( g_statPriority ) + ARRAYSIZE( g_statUnused ) == TFSTAT_MAX );

	ResetDisplayedStat();
	m_iCurStatClassIndex = -1;
	m_bStatsChanged = true;
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	SetVisible( false );
	SetScheme( "ClientScheme" );
	statPanel = this;

	m_pClassImage = new CTFClassImage( this, "StatPanelClassImage" );

	// Read stats from disk.  (Definitive stat store for X360; for PC, whatever we get from Steam is authoritative.)
	ReadStats();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFStatPanel::~CTFStatPanel()
{
	if ( statPanel == this )
		statPanel = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatPanel::Reset()
{
	if ( gpGlobals->curtime > m_flTimeHide )
	{
		Hide();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Resets which stat is being displayed
//-----------------------------------------------------------------------------
void CTFStatPanel::ResetDisplayedStat()
{
	m_iCurStatValue = 0;
	m_iCurStatTeam = TEAM_UNASSIGNED;
	m_statType = TFSTAT_UNDEFINED;
	m_recordBreakType = RECORDBREAK_NONE;
	m_bDisplayAfterSpawn = 0;
	m_flTimeHide = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatPanel::Init()
{
	// listen for events
	ListenForGameEvent( "teamplay_stat_panel" );
	ListenForGameEvent( "player_spawn" );
	ListenForGameEvent( "game_newmap" );
	
	Hide();

	// TFP3: Doesn't show in IDA
	//CHudElement::Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatPanel::UpdateStats( int iClass, RoundStats_t &stats, bool bShowNow )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	int i;
	for( i = 0; i < m_aClassStats.Count(); i++ )
	{
		if ( m_aClassStats[i].iPlayerClass == iClass )
		{
			break;
		}
	}
	
	if ( i >= m_aClassStats.Count() )
	{		
		ClassStats_t src;
		src.iPlayerClass = iClass;
		statPanel->m_aClassStats.AddToTail( src );
	}

	ClassStats_t &classStats = m_aClassStats[i];
	
	classStats.accumulated.m_iStat[TFSTAT_MAXSENTRYKILLS] = 0;
	m_flTimeHide = 0.0;
	m_iCurStatValue = 0;
	m_iCurStatTeam = 0;
	m_statType = TFSTAT_UNDEFINED;
	m_recordBreakType = RECORDBREAK_NONE;
	m_bDisplayAfterSpawn = false;
	m_iCurStatClassIndex = i;
		
	// run through all stats we keep records for, update the max value, and if a record is set,
	// remember the highest priority record
	for ( int i= ARRAYSIZE( g_statPriority )-1; i >= 0; i-- )
	{
		TFStatType_t statType = g_statPriority[i];
		int iCur = stats.m_iStat[statType];
		int iMax = classStats.max.m_iStat[statType];
		if ( iCur > iMax )
		{
			// Record was set, remember what stat set a record.
			classStats.max.m_iStat[statType] = iCur;
			m_iCurStatValue = iCur;
			m_statType = statType;
			m_recordBreakType = RECORDBREAK_BEST;
		}
		else if ( ( iCur > 0 ) && ( m_recordBreakType <= RECORDBREAK_TIE ) && ( iCur == iMax ) )
		{
			// if we haven't broken a record and we tied this one, display it
			m_iCurStatValue = iCur;
			m_statType = statType;
			m_recordBreakType = RECORDBREAK_TIE;
		}
		else if ( ( iCur > 0 ) && ( m_recordBreakType <= RECORDBREAK_CLOSE ) && ( iCur >= (int) ( (float) iMax * 0.8f ) ) )
		{
			// if we haven't broken a record or tied a record but we came close to this one, display it
			m_iCurStatValue = iCur;
			m_statType = statType;
			m_recordBreakType = RECORDBREAK_CLOSE;
		}
	}

	m_bStatsChanged = true;

	if ( m_statType > TFSTAT_UNDEFINED )
	{
		m_iCurStatTeam = pPlayer->GetTeamNumber();
		if ( bShowNow )
		{
			// show the panel now if dead or very recently spawned
			vgui::ivgui()->AddTickSignal( GetVPanel(), 1000 );
			ShowStatPanel( m_iCurStatClassIndex, m_iCurStatTeam, m_iCurStatValue, m_statType, m_recordBreakType );
			m_flTimeHide = gpGlobals->curtime + 20.0f;
		}
		else
		{
			m_bDisplayAfterSpawn = true;
		}
		WriteStats();
	}

	GStatsSummaryPanel()->SetStats( m_aClassStats );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatPanel::TestStatPanel( TFStatType_t statType )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;
	
	int i;
	for( i = 0; i < m_aClassStats.Count(); i++ )
	{
		if ( m_aClassStats[i].iPlayerClass == pPlayer->GetPlayerClass()->GetClassIndex() )
		{
			break;
		}
	}
	
	if ( i >= m_aClassStats.Count() )
	{		
		ClassStats_t src;
		src.iPlayerClass = pPlayer->GetPlayerClass()->GetClassIndex();
		statPanel->m_aClassStats.AddToTail( src );
	}

	ClassStats_t &classStats = m_aClassStats[i];
	
	m_iCurStatClassIndex = i;
	m_iCurStatValue = classStats.max.m_iStat[statType];
	m_iCurStatTeam = pPlayer->GetTeamNumber();

	ShowStatPanel( m_iCurStatClassIndex, m_iCurStatTeam, m_iCurStatValue, statType, RECORDBREAK_BEST );
}

//-----------------------------------------------------------------------------
// Purpose: Writes stat file.  Used as primary storage for X360.  For PC,
//			Steam is authoritative but we write stat file for debugging (although
//			we never read it).
//-----------------------------------------------------------------------------
void CTFStatPanel::WriteStats( void )
{
	if ( !m_bStatsChanged )
		return;

	MEM_ALLOC_CREDIT();

	DECLARE_DMX_CONTEXT();
	CDmxElement *pPlayerStats = CreateDmxElement( "PlayerStats" );
	CDmxElementModifyScope modify( pPlayerStats );

	pPlayerStats->SetValue( "iVersion", static_cast<int>( PLAYERSTATS_FILE_VERSION ) );

	CDmxAttribute *pClassStatsList = pPlayerStats->AddAttribute( "aClassStats" );
	CUtlVector< CDmxElement* >& classStats = pClassStatsList->GetArrayForEdit<CDmxElement*>();

	modify.Release();

	for( int i = 0; i < m_aClassStats.Count(); i++ )
	{
		const ClassStats_t &stat = m_aClassStats[ i ];

		// strip out any garbage class data
		if ( ( stat.iPlayerClass > TF_LAST_NORMAL_CLASS ) || ( stat.iPlayerClass < TF_FIRST_NORMAL_CLASS ) )
			continue;

		CDmxElement *pClass = CreateDmxElement( "ClassStats_t" );
		classStats.AddToTail( pClass );

		CDmxElementModifyScope modifyClass( pClass );

		pClass->SetValue( "comment: classname", g_aPlayerClassNames_NonLocalized[ stat.iPlayerClass ] );
		pClass->AddAttributesFromStructure( &stat, s_ClassStatsUnpack );
		
		CDmxElement *pAccumulated = CreateDmxElement( "RoundStats_t" );
		pAccumulated->AddAttributesFromStructure( &stat.accumulated, s_RoundStatsUnpack );
		pClass->SetValue( "accumulated", pAccumulated );

		CDmxElement *pMax = CreateDmxElement( "RoundStats_t" );
		pMax->AddAttributesFromStructure( &stat.max, s_RoundStatsUnpack );
		pClass->SetValue( "max", pMax );
	}

	if ( IsX360() )
	{
#ifdef _X360
		if ( XBX_GetStorageDeviceId() == XBX_INVALID_STORAGE_ID || XBX_GetStorageDeviceId() == XBX_STORAGE_DECLINED )
			return;
#endif
	}

	char szFilename[_MAX_PATH];

	if ( IsX360() )
		Q_snprintf( szFilename, sizeof( szFilename ), "cfg:/tf2_playerstats.dmx" );
	else
		Q_snprintf( szFilename, sizeof( szFilename ), "tf2_playerstats.dmx" );

	{
		MEM_ALLOC_CREDIT();
		CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
		if ( SerializeDMX( buf, pPlayerStats, szFilename ) )
		{
			filesystem->WriteFile( szFilename, "MOD", buf );
		}
	}

	CleanupDMX( pPlayerStats );

	if ( IsX360() )
	{
		xboxsystem->FinishContainerWrites();
	}

	m_bStatsChanged = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFStatPanel::ReadStats( void )
{
	CDmxElement *pPlayerStats;

	DECLARE_DMX_CONTEXT();

	if ( IsX360() )
	{
#ifdef _X360
		if ( XBX_GetStorageDeviceId() == XBX_INVALID_STORAGE_ID || XBX_GetStorageDeviceId() == XBX_STORAGE_DECLINED )
			return false;
#endif
	}

	MEM_ALLOC_CREDIT();

	bool bOk = UnserializeDMX( "tf2_playerstats.dmx", "MOD", true, &pPlayerStats );

	if ( !bOk )
		return false;

	int iVersion = pPlayerStats->GetValue< int >( "iVersion" );
	if ( iVersion > PLAYERSTATS_FILE_VERSION )
	{
		// file is beyond our comprehension
		return false;
	}
	
	const CUtlVector< CDmxElement* > &aClassStatsList = pPlayerStats->GetArray< CDmxElement * >( "aClassStats" );
	int iCount = aClassStatsList.Count();
	m_aClassStats.SetCount( iCount );
	for( int i = 0; i < m_aClassStats.Count(); i++ )
	{
		CDmxElement *pClass = aClassStatsList[ i ];
		ClassStats_t &stat = m_aClassStats[ i ];

		pClass->UnpackIntoStructure( &stat, s_ClassStatsUnpack );

		CDmxElement *pAccumulated = pClass->GetValue< CDmxElement * >( "accumulated" );
		pAccumulated->UnpackIntoStructure( &stat.accumulated, s_RoundStatsUnpack );

		CDmxElement *pMax = pClass->GetValue< CDmxElement * >( "max" );
		pMax->UnpackIntoStructure( &stat.max, s_RoundStatsUnpack );
	}

	CleanupDMX( pPlayerStats );

	GStatsSummaryPanel()->SetStats( m_aClassStats );

	m_bStatsChanged = false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatPanel::ShowStatPanel( int iClassIndex, int iTeam, int iCurStatValue, TFStatType_t statType, RecordBreakType_t recordBreakType )
{
	ClassStats_t &classStats = m_aClassStats[iClassIndex];
	vgui::Label *pLabel = dynamic_cast<Label *>( FindChildByName( "summaryLabel" ) );
	if ( !pLabel )
		return;

	const char *pRecordTextSuffix[RECORDBREAK_MAX] = { "", "close", "tie", "best" };

	const char *pLocalizedTitle = m_bDisplayAfterSpawn ? "#StatPanel_Title_Alive" : "#StatPanel_Title_Dead";
	SetDialogVariable( "title", g_pVGuiLocalize->Find( pLocalizedTitle ) );
	SetDialogVariable( "stattextlarge", "" );
	SetDialogVariable( "stattextsmall", "" );
	if ( recordBreakType == RECORDBREAK_CLOSE )
	{		
		// if we are displaying that the player got close to a record, show current & best values
		char szCur[32],szBest[32];
		wchar_t wzCur[32],wzBest[32];
		GetStatValueAsString( iCurStatValue, statType, szCur, ARRAYSIZE( szCur ) );
		GetStatValueAsString( classStats.max.m_iStat[statType], statType, szBest, ARRAYSIZE( szBest ) );
		g_pVGuiLocalize->ConvertANSIToUnicode( szCur, wzCur, sizeof( wzCur ) );
		g_pVGuiLocalize->ConvertANSIToUnicode( szBest, wzBest, sizeof( wzBest ) );
		wchar_t *wzFormat = g_pVGuiLocalize->Find( "#StatPanel_Format_Close" );
		wchar_t wzText[256];
		g_pVGuiLocalize->ConstructString( wzText, sizeof( wzText ), wzFormat, 2, wzCur, wzBest );
		SetDialogVariable( "stattextsmall", wzText );
	}
	else
	{
		// player broke or tied a record, just show current value
		char szValue[32];
		GetStatValueAsString( iCurStatValue, statType, szValue, ARRAYSIZE( szValue ) );
		SetDialogVariable( "stattextlarge", szValue );
	}

	SetDialogVariable( "statdesc", g_pVGuiLocalize->Find( CFmtStr( "%s_%s", g_szLocalizedRecordText[statType], 
		pRecordTextSuffix[recordBreakType] ) ) );

	iClassIndex = classStats.iPlayerClass;

	if ( ( iClassIndex >= TF_FIRST_NORMAL_CLASS ) && ( iClassIndex <= TF_LAST_NORMAL_CLASS ) )
	{
		// Set the class name. We can't use a dialog variable because it's a string that's already
		// been set using a dialog variable, and apparently we don't support nested dialog variables.
		wchar_t szOriginalSummary[ 256 ];
		wchar_t szSummary[ 256 ];

		// This is the field that "statdesc" completed for us
		pLabel->GetText( szOriginalSummary, sizeof( szOriginalSummary ) );
		const wchar_t *pszPlayerClass = g_pVGuiLocalize->Find( g_aPlayerClassNames[ iClassIndex ] );
		g_pVGuiLocalize->ConstructString( szSummary, sizeof( szSummary ), szOriginalSummary, 1, pszPlayerClass );
		pLabel->SetText( szSummary );	
	}

	if ( m_pClassImage )
	{
		m_pClassImage->SetClass( iTeam, iClassIndex, 0 );
	}

	Show();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatPanel::FireGameEvent( IGameEvent * event )
{
	const char *pEventName = event->GetName();

	if ( Q_strcmp( "player_spawn", pEventName ) == 0 )
	{
		Msg( "got player_spawn event\n" );
		int iUserID = event->GetInt( "userid" );
		if ( !C_TFPlayer::GetLocalTFPlayer() || ( C_TFPlayer::GetLocalTFPlayer()->GetUserID() != iUserID ) )
			return;

		// TFP3: Is m_bDisplayAfterSpawn correct?
		if ( m_bDisplayAfterSpawn )
		{
			// show the panel now if dead or very recently spawned
			vgui::ivgui()->AddTickSignal( GetVPanel(), 1000 );
			ShowStatPanel( m_iCurStatClassIndex, m_iCurStatTeam, m_iCurStatValue, m_statType, m_recordBreakType );
			m_flTimeHide = gpGlobals->curtime + 12.0f;
			m_bDisplayAfterSpawn = false; // TFP3: Is m_bDisplayAfterSpawn correct?
		}
		else
		{
			// hide panel if we're currently showing it
			Hide();
		}
	}
	else if ( Q_strcmp( "teamplay_stat_panel", pEventName ) == 0 )
	{
		RoundStats_t stats;
		int userid = event->GetInt( "userid" );
		int iClass = event->GetInt( "class" );
		bool bShowNow = event->GetInt( "alive" ) == 0;
		
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pPlayer && pPlayer->GetUserID() == userid )
		{
			for ( int i = 0; i < TFSTAT_MAX; ++i )
			{
				stats.m_iStat[i] = event->GetInt( g_szStatEventParamName[i] );
			}
			UpdateStats( iClass, stats, bShowNow );
		}
	}
	else if ( Q_strcmp( "game_newmap", pEventName ) == 0 )
	{
		WriteStats();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/StatPanel_Base.res" );

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	vgui::Panel *pStatBox = FindChildByName("StatBox");
	if ( pStatBox )
	{
		// Dirty hack: Make the statbox update now, and then change its bgColor.
		// When it then gets ApplySchemeSetting called shortly after this, it doesn't
		// reapply the scheme because its dirty-scheme flag has been removed.
		pStatBox->ApplySchemeSettings( pScheme );
		pStatBox->SetBgColor( GetSchemeColor("TransparentLightBlack", pScheme) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatPanel::OnTick()
{
	// see if it's time to hide the panel
	if ( m_flTimeHide > 0 && gpGlobals->curtime > m_flTimeHide )
	{
		Hide();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatPanel::Show()
{
	bool bShow = true;
	CTFWinPanel *pWinPanel = GET_HUDELEMENT( CTFWinPanel );
	if ( !m_bDisplayAfterSpawn && pWinPanel && pWinPanel->IsVisible() )
	{
		bShow = false;
	}

	SetVisible( bShow );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatPanel::Hide()
{
	SetVisible( false );
	if ( m_flTimeHide > 0 )
	{
		m_flTimeHide = 0;
		vgui::ivgui()->RemoveTickSignal( GetVPanel() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFStatPanel::ShouldDraw( void )
{
	if ( !IsVisible() )
		return false;

	if ( IsTakingAFreezecamScreenshot() )
		return false;

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatPanel::ResetStats( void )
{
	m_flTimeHide = 0.0;
	m_bStatsChanged = true;
	m_iCurStatValue = 0;
	m_iCurStatTeam = 0;
	m_statType = TFSTAT_UNDEFINED;
	m_recordBreakType = RECORDBREAK_NONE;
	m_iCurStatClassIndex = -1;
	m_bDisplayAfterSpawn = false;
	GStatsSummaryPanel()->SetStats( m_aClassStats );
	WriteStats();
}

//-----------------------------------------------------------------------------
// Purpose: Renders stat value as string
//-----------------------------------------------------------------------------
void CTFStatPanel::GetStatValueAsString( int iValue, TFStatType_t statType, char *value, int valuelen )
{
	if ( TFSTAT_PLAYTIME == statType )
	{
		// Format time as a time string
		Q_strncpy( value, FormatSeconds( iValue ), valuelen );
	}
	else
	{
		// all other stats are just displayed as #'s
		Q_snprintf( value, valuelen, "%d", iValue );
	}
}

/**********************************************************************************/

void TestStatPanel( const CCommand &args )
{
	int iPanelType;

	if( args.ArgC() < 2 )
	{
		ConMsg( "Usage:  teststatpanel < panel type >\n" );
		ConMsg( "Usable panel types are %d to %d\n", TFSTAT_UNDEFINED + 1, TFSTAT_MAX - 1 );
		return;
	}

	if ( statPanel )
	{
		iPanelType = atoi( args.Arg( 1 ) );

		if ( ( iPanelType <= TFSTAT_UNDEFINED ) || ( iPanelType >= TFSTAT_MAX ) )
		{
			ConMsg( "Usage:  teststatpanel < panel type >\n" );
			ConMsg( "Usable panel types are %d to %d\n", TFSTAT_UNDEFINED + 1, TFSTAT_MAX - 1 );
			return;
		}

		statPanel->TestStatPanel( (TFStatType_t) iPanelType );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void HideStatPanel()
{
	if ( statPanel )
	{
		statPanel->Hide();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ResetPlayerStats()
{
	if ( statPanel )
	{
		statPanel->ResetStats();
	}
}

ConCommand teststatpanel( "teststatpanel", TestStatPanel, "", FCVAR_DEVELOPMENTONLY );
ConCommand hidestatpanel( "hidestatpanel", HideStatPanel, "", FCVAR_DEVELOPMENTONLY );
ConCommand resetplayerstats( "resetplayerstats", ResetPlayerStats, "" );
