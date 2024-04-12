//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//  
//
//=============================================================================

#include "cbase.h"
#include "basetempentity.h"
#include "tf_fx.h"
#include "tf_shareddefs.h"
#include "coordsize.h"

#define NUM_BULLET_SEED_BITS	8

//-----------------------------------------------------------------------------
// Purpose: Display a blood sprite
//-----------------------------------------------------------------------------
class CTEFireBullets : public CBaseTempEntity
{
public:

	DECLARE_CLASS( CTEFireBullets, CBaseTempEntity );
	DECLARE_SERVERCLASS();

	CTEFireBullets( const char *name );
	virtual	~CTEFireBullets( void );

public:

	CNetworkVar( int, m_iPlayer );
	CNetworkVector( m_vecOrigin );
	CNetworkQAngle( m_vecAngles );
	CNetworkVar( int, m_iWeaponID );
	CNetworkVar( int, m_iMode );
	CNetworkVar( int, m_iSeed );
	CNetworkVar( float, m_flSpread );

};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEFireBullets::CTEFireBullets( const char *name ) : CBaseTempEntity( name )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEFireBullets::~CTEFireBullets( void )
{
}

IMPLEMENT_SERVERCLASS_ST_NOBASE( CTEFireBullets, DT_TEFireBullets )
SendPropVector( SENDINFO(m_vecOrigin), -1, SPROP_COORD_MP_INTEGRAL ),
SendPropAngle( SENDINFO_VECTORELEM( m_vecAngles, 0 ), 7, 0 ),
SendPropAngle( SENDINFO_VECTORELEM( m_vecAngles, 1 ), 7, 0 ),
SendPropInt( SENDINFO( m_iWeaponID ), Q_log2(TF_WEAPON_COUNT)+1, SPROP_UNSIGNED ),
SendPropInt( SENDINFO( m_iMode ), 1, SPROP_UNSIGNED ),
SendPropInt( SENDINFO( m_iSeed ), NUM_BULLET_SEED_BITS, SPROP_UNSIGNED ),
SendPropInt( SENDINFO( m_iPlayer ), 6, SPROP_UNSIGNED ), 	// max 64 players, see MAX_PLAYERS
SendPropFloat( SENDINFO( m_flSpread ), 8, 0, 0.0f, 1.0f ),	
END_SEND_TABLE()

// Singleton
static CTEFireBullets g_TEFireBullets( "Fire Bullets" );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void TE_FireBullets( int iPlayerIndex, const Vector &vOrigin, const QAngle &vAngles, 
					 int iWeaponID, int	iMode, int iSeed, float flSpread )
{
	CPASFilter filter( vOrigin );
	filter.UsePredictionRules();

	g_TEFireBullets.m_iPlayer = iPlayerIndex-1;
	g_TEFireBullets.m_vecOrigin = vOrigin;
	g_TEFireBullets.m_vecAngles = vAngles;
	g_TEFireBullets.m_iSeed = iSeed;
	g_TEFireBullets.m_flSpread = flSpread;
	g_TEFireBullets.m_iMode = iMode;
	g_TEFireBullets.m_iWeaponID = iWeaponID;

	Assert( iSeed < (1 << NUM_BULLET_SEED_BITS) );

	g_TEFireBullets.Create( filter, 0 );
}