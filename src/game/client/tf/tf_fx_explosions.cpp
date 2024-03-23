//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: Game-specific explosion effects
//
//=============================================================================//
#include "cbase.h"
#include "c_te_effect_dispatch.h"
#include "tempent.h"
#include "c_te_legacytempents.h"
#include "tf_shareddefs.h"
#include "engine/IEngineSound.h"
#include "tf_weapon_parse.h"
#include "c_basetempentity.h"
#include "tier0/vprof.h"

//--------------------------------------------------------------------------------------------------------------
CTFWeaponInfo *GetTFWeaponInfo( int iWeapon )
{
	// Get the weapon information.
	const char *pszWeaponAlias = WeaponIdToAlias( iWeapon );
	if ( !pszWeaponAlias )
	{
		return NULL;
	}

	WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( pszWeaponAlias );
	if ( hWpnInfo == GetInvalidWeaponInfoHandle() )
	{
		return NULL;
	}

	CTFWeaponInfo *pWeaponInfo = static_cast<CTFWeaponInfo*>( GetFileWeaponInfoFromHandle( hWpnInfo ) );
	return pWeaponInfo;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TF_ExplosionCallback( const CEffectData &data )
{
	int iWeaponID = data.m_fFlags;

	// Get the weapon information.
	CTFWeaponInfo *pWeaponInfo = NULL;
	switch( iWeaponID )
	{
	case TF_WEAPON_GRENADE_PIPEBOMB:
	case TF_WEAPON_GRENADE_DEMOMAN:
		pWeaponInfo = GetTFWeaponInfo( TF_WEAPON_PIPEBOMBLAUNCHER );
		break;
	case TF_WEAPON_GRENADE_MIRVBOMB:
		pWeaponInfo = GetTFWeaponInfo( TF_WEAPON_GRENADE_NORMAL );
		break;
	default:
		pWeaponInfo = GetTFWeaponInfo( iWeaponID );
		break;
	}

	// Base explosion effect and sound.
	char *pszEffect = "explosion";
	char *pszSound = "BaseExplosionEffect.Sound";

	if ( pWeaponInfo )
	{
		if ( Q_strlen( pWeaponInfo->m_szExplosionEffect ) > 0 )
		{
			pszEffect = pWeaponInfo->m_szExplosionEffect;
		}
		
		// Sound.
		if ( Q_strlen( pWeaponInfo->m_szExplosionSound ) > 0 )
		{
			pszSound = pWeaponInfo->m_szExplosionSound;
		}
	}
	
	if ( (enginetrace->GetPointContents( data.m_vOrigin ) & CONTENTS_WATER) )
		pszEffect = "ExplosionCore_underwater";

	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, pszSound, &data.m_vOrigin );

	DispatchParticleEffect( pszEffect, data.m_vOrigin, data.m_vAngles );
}

DECLARE_CLIENT_EFFECT( "TF_Explosion", TF_ExplosionCallback );