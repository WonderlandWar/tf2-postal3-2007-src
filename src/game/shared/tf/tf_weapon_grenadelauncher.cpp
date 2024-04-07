//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_grenadelauncher.h"
#include "tf_fx_shared.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_gamestats.h"
#endif

// hard code these eventually
extern ConVar tf_grenadelauncher_min_vel;
extern ConVar tf_grenadelauncher_max_vel;
extern ConVar tf_grenadelauncher_max_chargetime;

//=============================================================================
//
// Weapon Grenade Launcher tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFGrenadeLauncher, DT_WeaponGrenadeLauncher )

BEGIN_NETWORK_TABLE( CTFGrenadeLauncher, DT_WeaponGrenadeLauncher )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CTFGrenadeLauncher )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_grenadelauncher, CTFGrenadeLauncher );
PRECACHE_WEAPON_REGISTER( tf_weapon_grenadelauncher );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFGrenadeLauncher )
END_DATADESC()
#endif

//=============================================================================
//
// Weapon Grenade Launcher functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadeLauncher::CTFGrenadeLauncher()
{
	m_bReloadsSingly = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadeLauncher::~CTFGrenadeLauncher()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::Spawn( void )
{
	m_iAltFireHint = HINT_ALTFIRE_GRENADELAUNCHER;
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we holster
//-----------------------------------------------------------------------------
bool CTFGrenadeLauncher::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_flChargeBeginTime = 0.0;
	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we deploy
//-----------------------------------------------------------------------------
bool CTFGrenadeLauncher::Deploy( void )
{
	m_flChargeBeginTime = 0.0;
	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::PrimaryAttack( void )
{
	// Check for ammunition.
	if ( m_iClip1 <= 0 && m_iClip1 != -1 )
		return;

	// Are we capable of firing again?
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	if ( !CanAttack() )
	{
		m_flChargeBeginTime = 0;
		return;
	}

	if ( m_flChargeBeginTime <= 0 )
	{
		// Set the weapon mode.
		m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

		// save that we had the attack button down
		m_flChargeBeginTime = gpGlobals->curtime;

		SendWeaponAnim( ACT_VM_PULLBACK );
	}
	else
	{
		float flTotalChargeTime = gpGlobals->curtime - m_flChargeBeginTime;

		if ( flTotalChargeTime >= tf_grenadelauncher_max_chargetime.GetFloat() )
		{
			LaunchGrenade();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::WeaponIdle( void )
{
	if ( m_flChargeBeginTime <= 0.0 || m_iClip1 <= 0 )
		BaseClass::WeaponIdle();
	else
		LaunchGrenade();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::LaunchGrenade( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	CalcIsAttackCritical();

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	pPlayer->SetAnimation( PLAYER_ATTACK1 );
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	FireProjectile( pPlayer );

#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif

	// Set next attack times.
	m_flNextPrimaryAttack = gpGlobals->curtime + m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;

	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );

	// Check the reload mode and behave appropriately.
	if ( m_bReloadsSingly )
	{
		m_iReloadMode.Set( TF_RELOAD_START );
	}
	
    m_flChargeBeginTime = 0.0;
}

float CTFGrenadeLauncher::GetProjectileSpeed( void )
{
	float flForwardSpeed = RemapValClamped( ( gpGlobals->curtime - m_flChargeBeginTime ),
		0.0f,
		tf_grenadelauncher_max_chargetime.GetFloat(),
		tf_grenadelauncher_min_vel.GetFloat(),
		tf_grenadelauncher_max_vel.GetFloat() );

	return flForwardSpeed;
}

//-----------------------------------------------------------------------------
// Purpose: Detonate this demoman's pipebombs
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::SecondaryAttack( void )
{
#ifdef GAME_DLL

	if ( !CanAttack() )
		return;

	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( pOwner )
	{
		CTFWeaponBase *pPipeBombLauncher = pOwner->Weapon_OwnsThisID( TF_WEAPON_PIPEBOMBLAUNCHER );
		if ( pPipeBombLauncher )
			pPipeBombLauncher->SecondaryAttack();
	}

#endif
}

float CTFGrenadeLauncher::GetChargeMaxTime( void )
{
	return tf_grenadelauncher_max_chargetime.GetFloat();
}

bool CTFGrenadeLauncher::Reload( void )
{
	if ( m_flChargeBeginTime > 0 )
		return false;

	return BaseClass::Reload();
}

void CTFGrenadeLauncher::WeaponReset( void )
{
  BaseClass::WeaponReset();
  m_flChargeBeginTime = 0.0;
}