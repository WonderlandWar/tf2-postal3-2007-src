//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Base Rockets.
//
//=============================================================================//
#include "cbase.h"
#include "tf_weaponbase_rocket.h"

// Server specific.
#ifdef GAME_DLL
#include "soundent.h"
#include "te_effect_dispatch.h"
#include "tf_fx.h"
#include "iscorer.h"
extern void SendProxy_Origin( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
extern void SendProxy_Angles( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
#endif

ConVar tf_rocket_homing_speed_lateral("tf_rocket_homing_speed_lateral","1", 0x4000);
ConVar tf_rocket_homing_speed_vertical("tf_rocket_homing_speed_vertical", "0.0", 0x4000);

//=============================================================================
//
// TF Base Rocket tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFBaseRocket, DT_TFBaseRocket )

BEGIN_NETWORK_TABLE( CTFBaseRocket, DT_TFBaseRocket )
// Client specific.
#ifdef CLIENT_DLL
RecvPropVector( RECVINFO( m_vInitialVelocity ) ),

RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
RecvPropQAngles( RECVINFO_NAME( m_angNetworkAngles, m_angRotation ) ),

// Server specific.
#else
SendPropVector( SENDINFO( m_vInitialVelocity ), 12 /*nbits*/, 0 /*flags*/, -3000 /*low value*/, 3000 /*high value*/	),

SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),
SendPropExclude( "DT_BaseEntity", "m_angRotation" ),

SendPropVector	(SENDINFO(m_vecOrigin), -1,  SPROP_COORD_MP_INTEGRAL|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
SendPropQAngles	(SENDINFO(m_angRotation), 6, SPROP_CHANGES_OFTEN, SendProxy_Angles ),

#endif
END_NETWORK_TABLE()

// Server specific.
#ifdef GAME_DLL
BEGIN_DATADESC( CTFBaseRocket )
DEFINE_FUNCTION( RocketTouch ),
DEFINE_THINKFUNC( FlyThink ),
END_DATADESC()
#endif

ConVar tf_rocket_show_radius( "tf_rocket_show_radius", "0", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Render rocket radius." );

//=============================================================================
//
// Shared (client/server) functions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFBaseRocket::CTFBaseRocket()
{
	m_vInitialVelocity.Init();

// Client specific.
#ifdef CLIENT_DLL

	m_flSpawnTime = 0.0f;
		
// Server specific.
#else

	m_flDamage = 0.0f;

#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CTFBaseRocket::~CTFBaseRocket()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseRocket::Precache( void )
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseRocket::Spawn( void )
{
	// Precache.
	Precache();

// Client specific.
#ifdef CLIENT_DLL

	m_flSpawnTime = gpGlobals->curtime;
	BaseClass::Spawn();

// Server specific.
#else

	//Derived classes must have set model.
	Assert( GetModel() );	

	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_FLY, MOVECOLLIDE_FLY_CUSTOM );
	AddEFlags( EFL_NO_WATER_VELOCITY_CHANGE );

	SetCollisionGroup( TFCOLLISION_GROUP_ROCKETS );

	UTIL_SetSize( this, -Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	// Setup attributes.
	m_takedamage = DAMAGE_NO;
	SetGravity( 0.0f );

	// Setup the touch and think functions.
	SetTouch( &CTFBaseRocket::RocketTouch );
	SetThink( &CTFBaseRocket::FlyThink );
	SetNextThink( gpGlobals->curtime );

	// Don't collide with players on the owner's team for the first bit of our life
	m_flCollideWithTeammatesTime = gpGlobals->curtime + 0.25;
	m_bCollideWithTeammates = false;

	m_bIsHoming = false;
	m_hHomingTarget = NULL;
#endif
}

//=============================================================================
//
// Client specific functions.
//
#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseRocket::PostDataUpdate( DataUpdateType_t type )
{
	// Pass through to the base class.
	BaseClass::PostDataUpdate( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		// Now stick our initial velocity and angles into the interpolation history.
		CInterpolatedVar<Vector> &interpolator = GetOriginInterpolator();
		interpolator.ClearHistory();

		CInterpolatedVar<QAngle> &rotInterpolator = GetRotationInterpolator();
		rotInterpolator.ClearHistory();

		float flChangeTime = GetLastChangeTime( LATCH_SIMULATION_VAR );

		// Add a sample 1 second back.
		Vector vCurOrigin = GetLocalOrigin() - m_vInitialVelocity;
		interpolator.AddToHead( flChangeTime - 1.0f, &vCurOrigin, false );

		QAngle vCurAngles = GetLocalAngles();
		rotInterpolator.AddToHead( flChangeTime - 1.0f, &vCurAngles, false );

		// Add the current sample.
		vCurOrigin = GetLocalOrigin();
		interpolator.AddToHead( flChangeTime, &vCurOrigin, false );

		rotInterpolator.AddToHead( flChangeTime - 1.0, &vCurAngles, false );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFBaseRocket::DrawModel( int flags )
{
	// During the first 0.2 seconds of our life, don't draw ourselves.
	if ( gpGlobals->curtime - m_flSpawnTime < 0.2f )
		return 0;

	return BaseClass::DrawModel( flags );
}

//=============================================================================
//
// Server specific functions.
//
#else

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFBaseRocket *CTFBaseRocket::Create( const char *pszClassname, const Vector &vecOrigin, 
									  const QAngle &vecAngles, CBaseEntity *pOwner, CBaseEntity *pHomingTarget )
{
	CTFBaseRocket *pRocket = static_cast<CTFBaseRocket*>( CBaseEntity::Create( pszClassname, vecOrigin, vecAngles, pOwner ) );
	if ( !pRocket )
		return NULL;

	// Initialize the owner.
	pRocket->SetOwnerEntity( pOwner );

	// Spawn.
	pRocket->Spawn();

	// Setup the initial velocity.
	Vector vecForward, vecRight, vecUp;
	AngleVectors( vecAngles, &vecForward, &vecRight, &vecUp );

	Vector vecVelocity = vecForward * 1100.0f;
	pRocket->SetAbsVelocity( vecVelocity );	
	pRocket->SetupInitialTransmittedGrenadeVelocity( vecVelocity );

	// Setup the initial angles.
	QAngle angles;
	VectorAngles( vecVelocity, angles );
	pRocket->SetAbsAngles( angles );

	// Set team.
	pRocket->ChangeTeam( pOwner->GetTeamNumber() );

	if ( pHomingTarget )
	{
		pRocket->m_bIsHoming = true;
		pRocket->SetHomingTarget( pHomingTarget );
	}

	return pRocket;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseRocket::RocketTouch( CBaseEntity *pOther )
{
	// Verify a correct "other."
	Assert( pOther );
	if ( pOther->IsSolidFlagSet( FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS ) )
		return;

	// Handle hitting skybox (disappear).
	const trace_t *pTrace = &CBaseEntity::GetTouchTrace();
	if( pTrace->surface.flags & SURF_SKY )
	{
		UTIL_Remove( this );
		return;
	}

	trace_t trace;
	memcpy( &trace, pTrace, sizeof( trace_t ) );
	Explode( &trace, pOther );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
unsigned int CTFBaseRocket::PhysicsSolidMaskForEntity( void ) const
{ 
	int teamContents = 0;

	if ( m_bCollideWithTeammates == false )
	{
		// Only collide with the other team
		teamContents = ( GetTeamNumber() == TF_TEAM_RED ) ? CONTENTS_BLUETEAM : CONTENTS_REDTEAM;
	}
	else
	{
		// Collide with both teams
		teamContents = CONTENTS_REDTEAM | CONTENTS_BLUETEAM;
	}

	return BaseClass::PhysicsSolidMaskForEntity() | teamContents;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseRocket::Explode( trace_t *pTrace, CBaseEntity *pOther )
{
	// Save this entity as enemy, they will take 100% damage.
	m_hEnemy = pOther;

	// Invisible.
	SetModelName( NULL_STRING );
	AddSolidFlags( FSOLID_NOT_SOLID );
	m_takedamage = DAMAGE_NO;

	// Pull out a bit.
	if ( pTrace->fraction != 1.0 )
	{
		SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 0.6f ) );
	}

	// Play explosion sound and effect.
	Vector vecOrigin = GetAbsOrigin();
	SendDispatchEffect();
	CSoundEnt::InsertSound ( SOUND_COMBAT, vecOrigin, 1024, 3.0 );

	// Damage.
	CBaseEntity *pAttacker = GetOwnerEntity();

	CTakeDamageInfo info( this, pAttacker, vec3_origin, vecOrigin, GetDamage(), GetDamageType() );
	float flRadius = GetRadius();
	RadiusDamage( info, vecOrigin, flRadius, CLASS_NONE, NULL );

	// Debug!
	if ( tf_rocket_show_radius.GetBool() )
	{
		DrawRadius( flRadius );
	}

	// Don't decal players with scorch.
	if ( !pOther->IsPlayer() )
	{
		UTIL_DecalTrace( pTrace, "Scorch" );
	}

	// Remove the rocket.
	UTIL_Remove( this );
}

void CTFBaseRocket::SendDispatchEffect( void )
{
	CDisablePredictionFiltering disabler;

	CEffectData explosionData;

	explosionData.m_vOrigin = GetAbsOrigin();
	explosionData.m_vAngles = GetAbsAngles();
	explosionData.m_fFlags = GetWeaponID();
	//explosionData.m_nEntIndex = entindex();

	DispatchEffect( "TF_Explosion", explosionData );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFBaseRocket::DrawRadius( float flRadius )
{
	Vector pos = GetAbsOrigin();
	int r = 255;
	int g = 0, b = 0;
	float flLifetime = 10.0f;
	bool bDepthTest = true;

	Vector edge, lastEdge;
	NDebugOverlay::Line( pos, pos + Vector( 0, 0, 50 ), r, g, b, !bDepthTest, flLifetime );

	lastEdge = Vector( flRadius + pos.x, pos.y, pos.z );
	float angle;
	for( angle=0.0f; angle <= 360.0f; angle += 22.5f )
	{
		edge.x = flRadius * cos( angle ) + pos.x;
		edge.y = pos.y;
		edge.z = flRadius * sin( angle ) + pos.z;

		NDebugOverlay::Line( edge, lastEdge, r, g, b, !bDepthTest, flLifetime );

		lastEdge = edge;
	}

	lastEdge = Vector( pos.x, flRadius + pos.y, pos.z );
	for( angle=0.0f; angle <= 360.0f; angle += 22.5f )
	{
		edge.x = pos.x;
		edge.y = flRadius * cos( angle ) + pos.y;
		edge.z = flRadius * sin( angle ) + pos.z;

		NDebugOverlay::Line( edge, lastEdge, r, g, b, !bDepthTest, flLifetime );

		lastEdge = edge;
	}

	lastEdge = Vector( pos.x, flRadius + pos.y, pos.z );
	for( angle=0.0f; angle <= 360.0f; angle += 22.5f )
	{
		edge.x = flRadius * cos( angle ) + pos.x;
		edge.y = flRadius * sin( angle ) + pos.y;
		edge.z = pos.z;

		NDebugOverlay::Line( edge, lastEdge, r, g, b, !bDepthTest, flLifetime );

		lastEdge = edge;
	}
}

void CTFBaseRocket::FlyThink( void )
{
	float flThinkTime;

	if ( m_bIsHoming )	// Note:
						// Anything under this block isn't fully complete 
						// because it's based off of IDA's pseudocode.
						// It functions well, but the code itself is bad.
	{
		CBaseEntity *pHomingTarget = m_hHomingTarget;
		if ( pHomingTarget )
		{
			Vector vecHomingTargetOrg = pHomingTarget->WorldSpaceCenter();

			float flHomingSpeedLateral = tf_rocket_homing_speed_lateral.GetFloat();
			float flHomingSpeedVertical = tf_rocket_homing_speed_vertical.GetFloat();
			const Vector vecOrigin = GetAbsOrigin();

			Vector vTargetDir;
			vTargetDir.y = vecHomingTargetOrg.x - vecOrigin.x;
			vTargetDir.z = vecHomingTargetOrg.y - vecOrigin.y;

			Vector vDir;
			vDir.x = vecHomingTargetOrg.z - vecOrigin.z;
			Vector vNewVelocity;
			vNewVelocity.x = VectorNormalize( *(Vector *)&vTargetDir.y );
			const Vector vecVelocity = GetAbsVelocity();
				
			vDir.y = vecVelocity.x;
			vDir.z = vecVelocity.y;

			const float flDist = VectorNormalize( *(Vector *)&vDir.y );
			vNewVelocity.y = vDir.y;
			vNewVelocity.z = vDir.z;
			vTargetDir.x = vecVelocity.z;
			float flNewVelZ;
			float flNewVelX;
			float flNewVelY;
			if (gpGlobals->frametime <= 0.0)
			{
				flNewVelZ = vNewVelocity.z;
				flNewVelX = vTargetDir.x;
				flNewVelY = vNewVelocity.y;
			}
			else if ( 0.0 == flDist )
			{
				flNewVelY = vTargetDir.y;
				flNewVelZ = vTargetDir.z;
				flNewVelX = vDir.x;
			}
			else
			{
				QAngle targetAngles;
				VectorAngles( *(Vector *)&vTargetDir.y, Vector(0,0,1), *(QAngle *)&targetAngles.y );

				const QAngle qAbsAngles = GetAbsAngles();

				QAngle newAngles;
				newAngles.x = qAbsAngles.z;
				newAngles.y = ApproachAngle( targetAngles.y, qAbsAngles.x, flHomingSpeedVertical );
				newAngles.z = ApproachAngle( targetAngles.z, qAbsAngles.y, flHomingSpeedLateral );
				targetAngles.x = newAngles.x;
				AngleVectors( *(QAngle *)&newAngles.y, (Vector *)&vNewVelocity.y );
				vNewVelocity.y = vNewVelocity.y * flDist;
				vNewVelocity.z = vNewVelocity.z * flDist;
				vTargetDir.x = vTargetDir.x * flDist;
				if ( VectorNormalize( *(Vector *)&vNewVelocity.y) >= 0.001 )
				{
					flNewVelZ = vNewVelocity.z;
					flNewVelX = vTargetDir.x;
					flNewVelY = vNewVelocity.y;
				}
				else
				{
					Vector vTempDir = vTargetDir;
					//float *p_y;
					//p_y = &vTargetDir.y;
					if (0.0 == vNewVelocity.x)
						//p_y = &vDir.y;
						vTempDir = vDir;
					flNewVelY = vTempDir.x;
					flNewVelX = vTempDir.z;
					flNewVelZ = vTempDir.y;
				}
			}
			
			vNewVelocity.y = flNewVelY * flDist;
			vNewVelocity.z = flNewVelZ * flDist;
			vTargetDir.x = flNewVelX * flDist;
			SetAbsVelocity( *(Vector *)&vNewVelocity.y );
		}

		QAngle angles;
		VectorAngles( GetAbsVelocity(), angles );
		SetAbsAngles( angles );
		flThinkTime = gpGlobals->curtime;
	}
	else
	{
		flThinkTime = gpGlobals->curtime + 0.1;
	}

	SetNextThink( flThinkTime );

	if ( gpGlobals->curtime > m_flCollideWithTeammatesTime && m_bCollideWithTeammates == false )
	{
		m_bCollideWithTeammates = true;
	}
}

void CTFBaseRocket::SetHomingTarget( CBaseEntity *pHomingTarget )
{
	m_hHomingTarget = pHomingTarget;
}

#endif
