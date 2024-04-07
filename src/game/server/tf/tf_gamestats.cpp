//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "tf_gamerules.h"
#include "tf_gamestats.h"
#include "tf_obj_sentrygun.h"
#include "tf_obj_dispenser.h"
#include "tf_obj_sapper.h"
#include "usermessages.h"
#include "player_resource.h"
#include "team.h"
#include "hl2orange.spa.h"

// Must run with -gamestats to be able to turn on/off stats with ConVar below.
static ConVar tf_stats_track( "tf_stats_track", "1", FCVAR_NONE, "Turn on//off tf stats tracking." );
static ConVar tf_stats_verbose( "tf_stats_verbose", "0", FCVAR_NONE, "Turn on//off verbose logging of stats." );

CTFGameStats CTF_GameStats;

const char *g_aClassNames[] =
{
	"TF_CLASS_UNDEFINED",
	"TF_CLASS_SCOUT",
	"TF_CLASS_SNIPER",
	"TF_CLASS_SOLDIER",
	"TF_CLASS_DEMOMAN",
	"TF_CLASS_MEDIC",
	"TF_CLASS_HEAVYWEAPONS",
	"TF_CLASS_PYRO",
	"TF_CLASS_SPY",
	"TF_CLASS_ENGINEER",
	"TF_CLASS_CIVILIAN",
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGameStats::CTFGameStats()
{
	gamestats = this;
	Clear();	
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGameStats::~CTFGameStats()
{
	Clear();
}

//-----------------------------------------------------------------------------
// Purpose: Clear out game stats
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFGameStats::Clear( void )
{
	m_reportedStats.Clear();
	Q_memset( m_aPlayerStats, 0, sizeof( m_aPlayerStats ) );
	CBaseGameStats::Clear();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFGameStats::StatTrackingEnabledForMod( void )
{
	return tf_stats_track.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: Loads previously saved game stats from file
//-----------------------------------------------------------------------------
bool CTFGameStats::LoadFromFile( void )
{
	// We deliberately don't load from previous file.  That's data we've already
	// reported, and for TF stats we don't want to re-accumulate data, just
	// keep sending fresh stuff to server.
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::AppendCustomDataToSaveBuffer( CUtlBuffer &SaveBuffer )
{
	m_reportedStats.AppendCustomDataToSaveBuffer( SaveBuffer );
	// clear stats since we've now reported these
	m_reportedStats.Clear();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::LoadCustomDataFromBuffer( CUtlBuffer &LoadBuffer )
{
	m_reportedStats.LoadCustomDataFromBuffer( LoadBuffer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_LevelInit( void )
{
	CBaseGameStats::Event_LevelInit();

	m_reportedStats.m_pCurrentMap = m_reportedStats.FindOrAddMapStats( STRING( gpGlobals->mapname ) );

	// Get the host ip and port.
	int nIPAddr = 0;
	short nPort = 0;
	ConVar *hostip = cvar->FindVar( "hostip" );
	if ( hostip )
	{
		nIPAddr = hostip->GetInt();
	}			

	ConVar *hostport = cvar->FindVar( "hostip" );
	if ( hostport )
	{
		nPort = hostport->GetInt();
	}			

	m_reportedStats.m_pCurrentMap->Init( STRING( gpGlobals->mapname ), nIPAddr, nPort, gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_LevelShutdown( float flElapsed )
{
	CBaseGameStats::Event_LevelShutdown( flElapsed );

	if ( m_reportedStats.m_pCurrentMap )
	{
		// TFP3: TODO: Is this accurate?
		m_reportedStats.m_pCurrentMap->Shutdown( gpGlobals->curtime );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Resets all stats for this player
//-----------------------------------------------------------------------------
void CTFGameStats::ResetPlayerStats( CTFPlayer *pPlayer )
{
	PlayerStats_t &stats = m_aPlayerStats[pPlayer->entindex()];
	// reset the stats on this player
	stats.Reset();
	// reset the matrix of who killed whom with respect to this player
	ResetKillHistory( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: Resets the kill history for this player
//-----------------------------------------------------------------------------
void CTFGameStats::ResetKillHistory( CTFPlayer *pPlayer )
{
	int iPlayerIndex = pPlayer->entindex();

	// for every other player, set all all the kills with respect to this player to 0
	for ( int i = 0; i < ARRAYSIZE( m_aPlayerStats ); i++ )
	{
		PlayerStats_t &statsOther = m_aPlayerStats[i];
		statsOther.statsKills.iNumKilled[iPlayerIndex] = 0;
		statsOther.statsKills.iNumKilledBy[iPlayerIndex] = 0;
		statsOther.statsKills.iNumKilledByUnanswered[iPlayerIndex] = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Resets per-round stats for all players
//-----------------------------------------------------------------------------
void CTFGameStats::ResetRoundStats()
{
	for ( int i = 0; i < ARRAYSIZE( m_aPlayerStats ); i++ )
	{		
		m_aPlayerStats[i].statsCurrentRound.Reset();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Increments specified stat for specified player by specified amount
//-----------------------------------------------------------------------------
void CTFGameStats::IncrementStat( CTFPlayer *pPlayer, TFStatType_t statType, int iValue )
{
	PlayerStats_t &stats = m_aPlayerStats[pPlayer->entindex()];
	stats.statsCurrentLife.m_iStat[statType] += iValue;
	stats.statsCurrentRound.m_iStat[statType] += iValue;
	stats.statsAccumulated.m_iStat[statType] += iValue;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::SendStatsToPlayer( CTFPlayer *pPlayer, bool bIsAlive )
{
	PlayerStats_t &stats = m_aPlayerStats[pPlayer->entindex()];
	stats.statsCurrentLife.m_iStat[TFSTAT_PLAYTIME] = gpGlobals->curtime - pPlayer->GetSpawnTime();
	stats.statsCurrentLife.m_iStat[TFSTAT_POINTSSCORED] = TFGameRules()->CalcPlayerScore( &stats.statsCurrentLife );
	stats.statsCurrentLife.m_iStat[TFSTAT_MAXSENTRYKILLS] = pPlayer->GetMaxSentryKills();

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_stat_panel" );
	if ( event )
	{
		event->SetInt( "userid", engine->GetPlayerUserId( pPlayer->edict() ) );
		event->SetInt( "class", pPlayer->GetPlayerClass()->GetClassIndex() );
		event->SetInt( "alive", bIsAlive );
		
		for ( int i = 1; i < TFSTAT_MAX; ++i )
		{
			const char *pszKey = g_szStatEventParamName[i];
			event->SetInt( pszKey, stats.statsCurrentLife.m_iStat[i] );
		}
		gameeventmanager->FireEvent( event );
	}

	AccumulateAndResetPerLifeStats( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::AccumulateAndResetPerLifeStats( CTFPlayer *pPlayer )
{
	int iClass = pPlayer->GetPlayerClass()->GetClassIndex();

	PlayerStats_t &stats = m_aPlayerStats[pPlayer->entindex()];

	// add score from previous life and reset current life stats
	int iScore = TFGameRules()->CalcPlayerScore( &stats.statsCurrentLife );
	if ( m_reportedStats.m_pCurrentMap != NULL )
	{
		m_reportedStats.m_pCurrentMap->m_aClassStats[iClass].iScore += iScore;
	}
	stats.statsCurrentRound.m_iStat[TFSTAT_POINTSSCORED] += iScore;
	stats.statsAccumulated.m_iStat[TFSTAT_POINTSSCORED] += iScore;
	stats.statsCurrentLife.Reset();	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerConnected( CBasePlayer *pPlayer )
{
	ResetPlayerStats( ToTFPlayer( pPlayer ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerDisconnected( CBasePlayer *pPlayer )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( !pTFPlayer )
		return;

	ResetPlayerStats( pTFPlayer );

	if ( pPlayer->IsAlive() )
	{
		int iClass = pTFPlayer->GetPlayerClass()->GetClassIndex();
		if ( m_reportedStats.m_pCurrentMap != NULL )
		{
			m_reportedStats.m_pCurrentMap->m_aClassStats[iClass].iTotalTime += (int) ( gpGlobals->curtime - pTFPlayer->GetSpawnTime() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerChangedClass( CTFPlayer *pPlayer )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerSpawned( CTFPlayer *pPlayer )
{	
	// if player is spawning as a member of valid team, increase the spawn count for his class
	int iTeam = pPlayer->GetTeamNumber();
	int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
	if ( TEAM_UNASSIGNED != iTeam && TEAM_SPECTATOR != iTeam )
	{
		if ( m_reportedStats.m_pCurrentMap != NULL )
		{
			m_reportedStats.m_pCurrentMap->m_aClassStats[iClass].iSpawns++;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------	
void CTFGameStats::Event_PlayerForceRespawn( CTFPlayer *pPlayer )
{
	if ( pPlayer->IsAlive() )
	{		
		// send stats to player
		SendStatsToPlayer( pPlayer, true );

		// if player is alive before respawn, add time from this life to class stat
		int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
		if ( m_reportedStats.m_pCurrentMap != NULL )
		{
			m_reportedStats.m_pCurrentMap->m_aClassStats[iClass].iTotalTime += (int) ( gpGlobals->curtime - pPlayer->GetSpawnTime() );
		}
	}

	AccumulateAndResetPerLifeStats( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerLeachedHealth( CTFPlayer *pPlayer, float amount ) 
{
	IncrementStat( pPlayer, TFSTAT_HEALTHLEACHED, (int) amount );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerHealedOther( CTFPlayer *pPlayer, float amount ) 
{
	IncrementStat( pPlayer, TFSTAT_HEALING, (int) amount );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_AssistKill( CTFPlayer *pAttacker, CBaseEntity *pVictim )
{
	// increment player's stat
	IncrementStat( pAttacker, TFSTAT_KILLASSISTS, 1 );
	// increment reported class stats
	int iClass = pAttacker->GetPlayerClass()->GetClassIndex();
	if ( m_reportedStats.m_pCurrentMap != NULL )
	{
		m_reportedStats.m_pCurrentMap->m_aClassStats[iClass].iAssists++;
	}

	if ( pVictim->IsPlayer() )
	{
		// keep track of how many times every player kills every other player
		CTFPlayer *pPlayerVictim = ToTFPlayer( pVictim );
		m_aPlayerStats[pPlayerVictim->entindex()].statsKills.iNumKilledByUnanswered[pPlayerVictim->entindex()] = 0;

	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerInvulnerable( CTFPlayer *pPlayer ) 
{
	IncrementStat( pPlayer, TFSTAT_INVULNS, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerCreatedBuilding( CTFPlayer *pPlayer, CBaseObject *pBuilding )
{
	// sappers are buildings from the code's point of view but not from the player's, don't count them
	CObjectSapper *pSapper = dynamic_cast<CObjectSapper *>( pBuilding );
	if ( pSapper )
		return;

	IncrementStat( pPlayer, TFSTAT_BUILDINGSBUILT, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerDestroyedBuilding( CTFPlayer *pPlayer, CBaseObject *pBuilding )
{
	// sappers are buildings from the code's point of view but not from the player's, don't count them
	CObjectSapper *pSapper = dynamic_cast<CObjectSapper *>( pBuilding );
	if ( pSapper )
		return;

	IncrementStat( pPlayer, TFSTAT_BUILDINGSDESTROYED, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_AssistDestroyBuilding( CTFPlayer *pPlayer, CBaseObject *pBuilding )
{
	// sappers are buildings from the code's point of view but not from the player's, don't count them
	CObjectSapper *pSapper = dynamic_cast<CObjectSapper *>( pBuilding );
	if ( pSapper )
		return;

	IncrementStat( pPlayer, TFSTAT_KILLASSISTS, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_Headshot( CTFPlayer *pKiller )
{
	IncrementStat( pKiller, TFSTAT_HEADSHOTS, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_Backstab( CTFPlayer *pKiller )
{
	IncrementStat( pKiller, TFSTAT_BACKSTABS, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerUsedTeleport( CTFPlayer *pTeleportOwner, CTFPlayer *pTeleportingPlayer )
{
	// We don't count the builder's teleports
	if ( pTeleportOwner != pTeleportingPlayer )
	{
		IncrementStat( pTeleportOwner, TFSTAT_TELEPORTS, 1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerFiredWeapon( CTFPlayer *pPlayer, bool bCritical ) 
{
	// If normal gameplay state, track weapon stats. 
	if ( TFGameRules()->State_Get() == GR_STATE_RND_RUNNING )
	{
		CTFWeaponBase *pTFWeapon = pPlayer->GetActiveTFWeapon();
		if ( pTFWeapon )
		{
			// record shots fired in reported per-weapon stats
			int iWeaponID = pTFWeapon->GetWeaponID();

			if ( m_reportedStats.m_pCurrentMap != NULL )
			{
				TF_Gamestats_WeaponStats_t *pWeaponStats = &m_reportedStats.m_pCurrentMap->m_aWeaponStats[iWeaponID];
				pWeaponStats->iShotsFired++;
				if ( bCritical )
				{
					pWeaponStats->iCritShotsFired++;
				}
			}
		}
	}

	IncrementStat( pPlayer, TFSTAT_SHOTS_FIRED, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerDamage( CBasePlayer *pBasePlayer, const CTakeDamageInfo &info, int iDamageTaken )
{
	CObjectSentrygun *pSentry = NULL;
	CTFPlayer *pTarget = ToTFPlayer( pBasePlayer );
	CTFPlayer *pAttacker = ToTFPlayer( info.GetAttacker() );
	if ( !pAttacker )
	{
		pSentry = dynamic_cast< CObjectSentrygun * >( info.GetAttacker() );
		if ( !pSentry )
			return;

		pAttacker = pSentry->GetOwner();
	}
	// don't count damage to yourself
	if ( pTarget == pAttacker )
		return;
	
	IncrementStat( pAttacker, TFSTAT_DAMAGE, iDamageTaken );

	TF_Gamestats_LevelStats_t::PlayerDamageLump_t damage;
	Vector killerOrg;

	// set the location where the target was hit
	const Vector &org = pTarget->GetAbsOrigin();
	damage.nTargetPosition[ 0 ] = static_cast<int>( org.x );
	damage.nTargetPosition[ 1 ] = static_cast<int>( org.y );
	damage.nTargetPosition[ 2 ] = static_cast<int>( org.z );

	// set the class of the attacker
	CBaseEntity *pInflictor = info.GetInflictor();
	CBasePlayer *pScorer = TFGameRules()->GetDeathScorer( pAttacker, pInflictor, pTarget );

	if ( !pSentry )
	{
		pSentry = dynamic_cast< CObjectSentrygun * >( pInflictor );
	}

	if ( pSentry != NULL )
	{
		killerOrg = pSentry->GetAbsOrigin();
		damage.iAttackClass = TF_CLASS_ENGINEER;
		damage.iWeapon = ( info.GetDamageType() & DMG_BLAST ) ? TF_WEAPON_SENTRY_ROCKET : TF_WEAPON_SENTRY_BULLET;
	} 
	else if ( dynamic_cast<CObjectDispenser *>( pInflictor ) )
	{
		damage.iAttackClass = TF_CLASS_ENGINEER;
		damage.iWeapon = TF_WEAPON_DISPENSER;
	}
	else
	{
		CTFPlayer *pTFAttacker = ToTFPlayer( pScorer );
		if ( pTFAttacker )
		{
			CTFPlayerClass *pAttackerClass = pTFAttacker->GetPlayerClass();
			damage.iAttackClass = ( !pAttackerClass ) ? TF_CLASS_UNDEFINED : pAttackerClass->GetClassIndex();
			killerOrg = pTFAttacker->GetAbsOrigin();
		}
		else
		{
			damage.iAttackClass = TF_CLASS_UNDEFINED;
			killerOrg = org;
		}

		// find the weapon the killer used
		damage.iWeapon = GetWeaponFromDamage( info );
	}

	// If normal gameplay state, track weapon stats. 
	if ( ( TFGameRules()->State_Get() == GR_STATE_RND_RUNNING ) && ( damage.iWeapon != TF_WEAPON_NONE  ) )
	{
		// record hits & damage in reported per-weapon stats
		if ( m_reportedStats.m_pCurrentMap != NULL )
		{
			TF_Gamestats_WeaponStats_t *pWeaponStats = &m_reportedStats.m_pCurrentMap->m_aWeaponStats[damage.iWeapon];
			pWeaponStats->iHits++;
			pWeaponStats->iTotalDamage += iDamageTaken;

			// Try and figure out where the damage is coming from
			Vector vecDamageOrigin = info.GetReportedPosition();
			// If we didn't get an origin to use, try using the attacker's origin
			if ( vecDamageOrigin == vec3_origin )
			{
				if ( pSentry )
				{
					vecDamageOrigin = pSentry->GetAbsOrigin();
				}
				else
				{
					vecDamageOrigin = killerOrg;
				}					
			}
			if ( vecDamageOrigin != vec3_origin )
			{
				pWeaponStats->iHitsWithKnownDistance++;
				int iDistance = (int) vecDamageOrigin.DistTo( pBasePlayer->GetAbsOrigin() );
//				Msg( "Damage distance: %d\n", iDistance );
				pWeaponStats->iTotalDistance += iDistance;
			}
		}
	}

	Assert( damage.iAttackClass != TF_CLASS_UNDEFINED );

	// record the time the damage occurred
	damage.fTime = gpGlobals->curtime;

	// store the attacker's position
	damage.nAttackerPosition[ 0 ] = static_cast<int>( killerOrg.x );
	damage.nAttackerPosition[ 1 ] = static_cast<int>( killerOrg.y );
	damage.nAttackerPosition[ 2 ] = static_cast<int>( killerOrg.z );

	// set the class of the target
	CTFPlayer *pTFPlayer = ToTFPlayer( pTarget );
	CTFPlayerClass *pTargetClass = ( pTFPlayer ) ? pTFPlayer->GetPlayerClass() : NULL;
	damage.iTargetClass = ( !pTargetClass ) ? TF_CLASS_UNDEFINED : pTargetClass->GetClassIndex();

	Assert( damage.iTargetClass != TF_CLASS_UNDEFINED );

	// record the damage done
	damage.iDamage = info.GetDamage();

	// record if it was a crit
	damage.iCrit = ( ( info.GetDamageType() & DMG_CRITICAL ) != 0 );

	// record if it was a kill
	damage.iKill = ( pTarget->GetHealth() <= 0 );

	// add it to the list of damages
	if ( m_reportedStats.m_pCurrentMap != NULL )
	{
		m_reportedStats.m_pCurrentMap->m_aPlayerDamage.AddToTail( damage );
	}	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerKilledOther( CBasePlayer *pAttacker, CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	// This also gets called when the victim is a building.  That gets tracked separately as building destruction, don't count it here
	if ( !pVictim->IsPlayer() )
		return;

	CTFPlayer *pPlayerAttacker = static_cast< CTFPlayer * >( pAttacker );
	
	IncrementStat( pPlayerAttacker, TFSTAT_KILLS, 1 );

	// keep track of how many times every player kills every other player
	CTFPlayer *pPlayerVictim = ToTFPlayer( pVictim );
	
	++m_aPlayerStats[pPlayerVictim->entindex()].statsKills.iNumKilledBy[pAttacker->entindex()];
	++m_aPlayerStats[pPlayerVictim->entindex()].statsKills.iNumKilledByUnanswered[pAttacker->entindex()];

	++m_aPlayerStats[pAttacker->entindex()].statsKills.iNumKilled[pPlayerVictim->entindex()];
	m_aPlayerStats[pAttacker->entindex()].statsKills.iNumKilledByUnanswered[pPlayerVictim->entindex()] = 0;

	int iClass = pPlayerAttacker->GetPlayerClass()->GetClassIndex();
	if ( m_reportedStats.m_pCurrentMap != NULL )
	{
		m_reportedStats.m_pCurrentMap->m_aClassStats[iClass].iKills++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_RoundEnd( int iWinningTeam, bool bFullRound, float flRoundTime )
{
	// only record full rounds, not mini-rounds
	if ( !bFullRound )
		return;

	TF_Gamestats_LevelStats_t *map = m_reportedStats.FindOrAddMapStats( gpGlobals->mapname.ToCStr() );

	map->m_Header.m_iRoundsPlayed++;
	map->m_Header.m_iTotalTime += (int) flRoundTime;
	switch ( iWinningTeam )
	{
	case TF_TEAM_RED:
		map->m_Header.m_iRedWins++;
		break;
	case TF_TEAM_BLUE:
		map->m_Header.m_iBlueWins++;
		break;
	case TEAM_UNASSIGNED:
		map->m_Header.m_iStalemates++;
		break;
	default:
		Assert( false );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerCapturedPoint( CTFPlayer *pPlayer )
{
	// increment player stats
	IncrementStat( pPlayer, TFSTAT_CAPTURES, 1 );
	// increment reported stats
	int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
	if ( m_reportedStats.m_pCurrentMap != NULL )
	{
		m_reportedStats.m_pCurrentMap->m_aClassStats[iClass].iCaptures++;
	}	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerDefendedPoint( CTFPlayer *pPlayer )
{
	IncrementStat( pPlayer, TFSTAT_DEFENSES, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerDominatedOther( CTFPlayer *pAttacker )
{
	IncrementStat( pAttacker, TFSTAT_DOMINATIONS, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerRevenge( CTFPlayer *pAttacker )
{
	IncrementStat( pAttacker, TFSTAT_REVENGE, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerKilled( CBasePlayer *pPlayer, const CTakeDamageInfo &info )
{
	Assert( pPlayer );
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

	IncrementStat( pTFPlayer, TFSTAT_DEATHS, 1 );
	SendStatsToPlayer( pTFPlayer, false );
	AccumulateAndResetPerLifeStats( pTFPlayer );

	TF_Gamestats_LevelStats_t::PlayerDeathsLump_t death;
	Vector killerOrg;

	// set the location where the target died
	const Vector &org = pPlayer->GetAbsOrigin();
	death.nPosition[ 0 ] = static_cast<int>( org.x );
	death.nPosition[ 1 ] = static_cast<int>( org.y );
	death.nPosition[ 2 ] = static_cast<int>( org.z );

	// set the class of the attacker
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CTFPlayer *pScorer = ToTFPlayer( TFGameRules()->GetDeathScorer( pKiller, pInflictor, pPlayer ) );

	if ( dynamic_cast< CObjectSentrygun * >( pInflictor ) != NULL )
	{
		killerOrg = pInflictor->GetAbsOrigin();
	}
	else
	{		
		if ( pScorer )
		{
			CTFPlayerClass *pAttackerClass = pScorer->GetPlayerClass();
			death.iAttackClass = ( !pAttackerClass ) ? TF_CLASS_UNDEFINED : pAttackerClass->GetClassIndex();
			killerOrg = pScorer->GetAbsOrigin();
		}
		else
		{
			death.iAttackClass = TF_CLASS_UNDEFINED;
			killerOrg = org;
		}
	}

	// set the class of the target
	CTFPlayerClass *pTargetClass = ( pTFPlayer ) ? pTFPlayer->GetPlayerClass() : NULL;
	death.iTargetClass = ( !pTargetClass ) ? TF_CLASS_UNDEFINED : pTargetClass->GetClassIndex();

	// find the weapon the killer used
	death.iWeapon = GetWeaponFromDamage( info );

	// calculate the distance to the killer
	death.iDistance = static_cast<unsigned short>( ( killerOrg - org ).Length() );

	// add it to the list of deaths
	TF_Gamestats_LevelStats_t *map = m_reportedStats.m_pCurrentMap;
	if ( map )
	{
		map->m_aPlayerDeaths.AddToTail( death );
		int iClass = ToTFPlayer( pPlayer )->GetPlayerClass()->GetClassIndex();

		if ( m_reportedStats.m_pCurrentMap != NULL )
		{
			m_reportedStats.m_pCurrentMap->m_aClassStats[iClass].iDeaths++;
			m_reportedStats.m_pCurrentMap->m_aClassStats[iClass].iTotalTime += (int) ( gpGlobals->curtime - pTFPlayer->GetSpawnTime() );
		}
	}
}

struct PlayerStats_t *CTFGameStats::FindPlayerStats( CBasePlayer *pPlayer )
{
	return &m_aPlayerStats[pPlayer->entindex()];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void CC_ListDeaths( const CCommand &args )
{
	TF_Gamestats_LevelStats_t *map = CTF_GameStats.m_reportedStats.m_pCurrentMap;
	if ( !map )
		return;

	for( int i = 0; i < map->m_aPlayerDeaths.Count(); i++ )
	{
		Msg( "%s killed %s with %s at (%d,%d,%d), distance %d\n",
            g_aClassNames[ map->m_aPlayerDeaths[ i ].iAttackClass ],
			g_aClassNames[ map->m_aPlayerDeaths[ i ].iTargetClass ],
			WeaponIdToAlias( map->m_aPlayerDeaths[ i ].iWeapon ), 
			map->m_aPlayerDeaths[ i ].nPosition[ 0 ],
			map->m_aPlayerDeaths[ i ].nPosition[ 1 ],
			map->m_aPlayerDeaths[ i ].nPosition[ 2 ],
			map->m_aPlayerDeaths[ i ].iDistance );
	}

	Msg( "\n---------------------------------\n\n" );

	for( int i = 0; i < map->m_aPlayerDamage.Count(); i++ )
	{
		Msg( "%.2f : %s at (%d,%d,%d) caused %d damage to %s with %s at (%d,%d,%d)%s%s\n",
			map->m_aPlayerDamage[ i ].fTime,
			g_aClassNames[ map->m_aPlayerDamage[ i ].iAttackClass ],
			map->m_aPlayerDamage[ i ].nAttackerPosition[ 0 ],
			map->m_aPlayerDamage[ i ].nAttackerPosition[ 1 ],
			map->m_aPlayerDamage[ i ].nAttackerPosition[ 2 ],
			map->m_aPlayerDamage[ i ].iDamage,
			g_aClassNames[ map->m_aPlayerDamage[ i ].iTargetClass ],
			WeaponIdToAlias( map->m_aPlayerDamage[ i ].iWeapon ), 
			map->m_aPlayerDamage[ i ].nTargetPosition[ 0 ],
			map->m_aPlayerDamage[ i ].nTargetPosition[ 1 ],
			map->m_aPlayerDamage[ i ].nTargetPosition[ 2 ],
			map->m_aPlayerDamage[ i ].iCrit ? ", CRIT!" : "",
			map->m_aPlayerDamage[ i ].iKill ? ", KILL" : ""	);
	}

	Msg( "\n---------------------------------\n\n" );
	Msg( "listed %d deaths\n", map->m_aPlayerDeaths.Count() );
	Msg( "listed %d damages\n\n", map->m_aPlayerDamage.Count() );
}

static ConCommand listDeaths("listdeaths", CC_ListDeaths, "lists player deaths", FCVAR_NONE );
