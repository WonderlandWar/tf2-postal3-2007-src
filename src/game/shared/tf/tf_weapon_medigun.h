//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_MEDIGUN_H
#define TF_WEAPON_MEDIGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

#if defined( CLIENT_DLL )
#define CWeaponMedigun C_WeaponMedigun
#endif

// There is a crash that keeps occuring when we try to network an array of healing targets, for now just use a singlular handle
#if 1
#define TFP3_MEDIGUN_FIX
#endif

#define MAX_HEALING_TARGETS			1	//6

#define CLEAR_ALL_TARGETS			-1

//=========================================================
// Beam healing gun
//=========================================================
class CWeaponMedigun : public CTFWeaponBaseGun
{
	DECLARE_CLASS( CWeaponMedigun, CTFWeaponBaseGun );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CWeaponMedigun( void );
	~CWeaponMedigun( void );

	virtual void	Precache();

	virtual bool	Deploy( void );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void	UpdateOnRemove( void );
	virtual void	ItemHolsterFrame( void );
	virtual void	ItemPostFrame( void );
	virtual bool	Lower( void );
	virtual void	PrimaryAttack( void );
	virtual void	SecondaryAttack( void );
	virtual void	WeaponIdle( void );
	void			DrainCharge( void );
	virtual void	WeaponReset( void );

	virtual float	GetTargetRange( void );
	virtual float	GetStickRange( void );
	virtual float	GetHealRate( void );
	virtual bool	AppliesModifier( void ) { return true; }

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_MEDIGUN; }

	bool			IsReleasingCharge( void ) { return (m_bChargeRelease && !m_bHolstered); }

#if defined( CLIENT_DLL )
	// Stop all sounds being output.
	void			StopHealSound( bool bStopHealingSound = true, bool bStopNoTargetSound = true );

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	ClientThink();
	void			UpdateEffects( void );
	void			ForceHealingTargetUpdate( void ) { m_bUpdateHealingTargets = true; }
#ifdef TFP3_MEDIGUN_FIX
	CBaseEntity		*GetHealTarget( void ) { return m_hHealingTarget.Get(); }
#else
	CBaseEntity		*GetHealTarget( void ) { return m_hHealingTargets[0].Get(); }
#endif
	void			ManageChargeEffect( void );
#else

#endif

	float			GetChargeLevel( void ) { return m_flChargeLevel; }

private:
	bool					FindAndHealTargets( void );
	void					MaintainTargetInSlot( int iSlot );
	void					FindNewTargetForSlot( int iSlot );
	void					RemoveHealingTarget( int iSlot, bool bStopHealingSelf = false );
	bool					HealingTarget( CBaseEntity *pTarget );
	bool					CouldHealTarget( CBaseEntity *pTarget );

public:

#ifdef TFP3_MEDIGUN_FIX
#ifdef GAME_DLL
	CNetworkHandle( CBaseEntity, m_hHealingTarget );
#else
	CNetworkHandle( C_BaseEntity, m_hHealingTarget );
#endif
#else
	CNetworkArray( EHANDLE, m_hHealingTargets, MAX_HEALING_TARGETS );
#endif

protected:
	// Networked data.
	CNetworkVar( bool,		m_bHealing );
	CNetworkVar( bool,		m_bAttacking );

	double					m_flNextBuzzTime;
	float					m_flHealEffectLifetime;	// Count down until the healing effect goes off.
	float					m_flReleaseStartedAt;

	CNetworkVar( bool,		m_bHolstered );
	CNetworkVar( bool,		m_bChargeRelease );
	CNetworkVar( float,		m_flChargeLevel );

	float					m_flNextTargetCheckTime;
	
#ifdef GAME_DLL
	CDamageModifier			m_DamageModifier;		// This attaches to whoever we're healing.
	bool					m_bHealingSelf;
#endif

#ifdef CLIENT_DLL
	bool					m_bPlayingSound;
	bool					m_bUpdateHealingTargets;
	struct healingtargeteffects_t
	{
		C_BaseEntity		*pTarget;
		CNewParticleEffect	*pEffect;
	};
	CUtlVector<healingtargeteffects_t> m_hHealingTargetEffects;

	float					m_flFlashCharge;
	bool					m_bOldChargeRelease;

	CNewParticleEffect	*m_pChargeEffect;
	CSoundPatch			*m_pChargedSound;
#endif

private:														
	CWeaponMedigun( const CWeaponMedigun & );
};

#endif // TF_WEAPON_MEDIGUN_H
