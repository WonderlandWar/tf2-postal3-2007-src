//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#ifndef TF_WEAPON_MINIGUN_H
#define TF_WEAPON_MINIGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

#ifdef CLIENT_DLL
#include "particles_new.h"
#endif

// Client specific.
#ifdef CLIENT_DLL
#define CTFMinigun C_TFMinigun
#endif

enum MinigunState_t
{
	// Firing states.
	AC_STATE_IDLE = 0,
	AC_STATE_STARTFIRING,
	AC_STATE_FIRING,
	AC_STATE_SPINNING,
	AC_STATE_DRYFIRE
};

//=============================================================================
//
// TF Weapon Minigun
//
class CTFMinigun : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFMinigun, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

// Server specific.
#ifndef CLIENT_DLL
	DECLARE_DATADESC();
#endif

	CTFMinigun();
	~CTFMinigun();

	virtual void	Precache( void );
	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_MINIGUN; }
	virtual void	PrimaryAttack();
	virtual void	SecondaryAttack();
	void			SharedAttack();
	virtual void	WeaponIdle();
	virtual bool	SendWeaponAnim( int iActivity );
	virtual bool	CanHolster( void );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual bool	Lower( void );
	virtual void	HandleFireOnEmpty( void );
	virtual void	WeaponReset( void );
		
	virtual void	ItemPreFrame( void );
	virtual void	ItemPostFrame( void );

	virtual int GetCustomDamageType() const { return TF_DMG_CUSTOM_MINIGUN; }

	float			GetFiringTime( void ) { return (m_flStartedFiringAt >= 0) ? (gpGlobals->curtime - m_flStartedFiringAt) : 0; }
		
	// Firing sound
	void				WeaponSoundUpdate( void );
	
	void				UpdateBarrelMovement( void );

#ifdef CLIENT_DLL
	float GetBarrelRotation();
#endif

private:
	
	CTFMinigun( const CTFMinigun & ) {}

	void WindUp( void );
	void WindDown( void );

#ifdef CLIENT_DLL
	// Barrel spinning
	virtual CStudioHdr *OnNewModel( void );
	virtual void		StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask );
	virtual void		UpdateOnRemove( void );
	void				CreateMove( float flInputSampleTime, CUserCmd *pCmd, const QAngle &vecOldViewAngles );
#endif

private:
	virtual void PlayWeaponShootSound( void ) {}	// override base class call to play shoot sound; we handle that ourselves separately

	CNetworkVar( MinigunState_t, m_iWeaponState );
	CNetworkVar( bool, m_bCritShot );

	float			m_flStartedFiringAt;
	float	m_flBarrelCurrentVelocity;
	float	m_flBarrelTargetVelocity;
	int		m_iBarrelBone;
	float	m_flBarrelAngle;
	CSoundPatch		*m_pSoundCur;				// the weapon sound currently being played
	int				m_iMinigunSoundCur;			// the enum value of the weapon sound currently being played
};

#endif // TF_WEAPON_MINIGUN_H
