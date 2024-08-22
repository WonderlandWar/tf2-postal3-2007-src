//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: A blood spray effect to expose successful hits.
//
//=============================================================================//

#include "cbase.h"
#include "ClientEffectPrecacheSystem.h"
#include "FX_Sparks.h"
#include "iefx.h"
#include "c_te_effect_dispatch.h"
#include "particles_ez.h"
#include "decals.h"
#include "engine/IEngineSound.h"
#include "fx_quad.h"
#include "engine/IVDebugOverlay.h"
#include "shareddefs.h"
#include "fx_blood.h"
#include "view.h"
#include "c_tf_player.h"
#include "debugoverlay_shared.h"
#include "tf_gamerules.h"
#include "c_basetempentity.h"
#include "tier0/vprof.h"

//-----------------------------------------------------------------------------
// Purpose: Intercepts the blood spray message.
//-----------------------------------------------------------------------------
void TFBloodSprayCallback( const CEffectData &data )
{
	Vector vecOrigin = data.m_vOrigin;
	Vector vecNormal = data.m_vNormal;
	ClientEntityHandle_t hEntity = data.m_hEntity;
	
	QAngle	vecAngles;
	VectorAngles( -vecNormal, vecAngles );

	// determine if the bleeding player is underwater
	bool bUnderwater = false;
	C_TFPlayer *pPlayer = dynamic_cast<C_TFPlayer*>( ClientEntityList().GetBaseEntityFromHandle( hEntity ) );
	if ( pPlayer && ( WL_Eyes == pPlayer->GetWaterLevel() )	)
	{
		bUnderwater = true;
	}
	 
	if ( !bUnderwater && TFGameRules() && TFGameRules()->IsBirthday() && RandomFloat(0,1) < 0.2 )
	{
		DispatchParticleEffect( "bday_blood", vecOrigin, vecAngles, pPlayer );
	}
	else
	{
		DispatchParticleEffect( bUnderwater ? "water_blood_impact_red_01" : "blood_impact_red_01", vecOrigin, vecAngles, pPlayer );
	}

	// if underwater, don't add additional spray
	if ( bUnderwater )
		return;

	// Now throw out a spray away from the view
	// Get the distance to the view
	float flDistance = (vecOrigin - MainViewOrigin()).Length();
	float flLODDistance = 0.25 * (flDistance / 512);

	Vector right, up;
	if (vecNormal != Vector(0, 0, 1) )
	{
		right = vecNormal.Cross( Vector(0, 0, 1) );
		up = right.Cross( vecNormal );
	}
	else
	{
		right = Vector(0, 0, 1);
		up = right.Cross( vecNormal );
	}

	// If the normal's too close to being along the view, push it out
	Vector vecForward, vecRight;
	AngleVectors( MainViewAngles(), &vecForward, &vecRight, NULL );
	float flDot = DotProduct( vecNormal, vecForward );
	if ( fabs(flDot) > 0.5 )
	{
		float flPush = random->RandomFloat(0.5, 1.5) + flLODDistance;
		float flRightDot = DotProduct( vecNormal, vecRight );

		// If we're up close, randomly move it around. If we're at a distance, always push it to the side
		// Up close, this can move it back towards the view, but the random chance still looks better
		if ( ( flDistance >= 512 && flRightDot > 0 ) || ( flDistance < 512 && RandomFloat(0,1) > 0.5 ) )
		{
			// Turn it to the right
			vecNormal += (vecRight * flPush);
		}
		else
		{
			// Turn it to the left
			vecNormal -= (vecRight * flPush);
		}
	}

	VectorAngles( vecNormal, vecAngles );

	if ( flDistance < 400 )
	{
		DispatchParticleEffect( "blood_spray_red_01", vecOrigin, vecAngles, pPlayer );
	}
	else
	{
		DispatchParticleEffect( "blood_spray_red_01_far", vecOrigin, vecAngles, pPlayer );
	}
}

DECLARE_CLIENT_EFFECT( "tf_bloodimpact", TFBloodSprayCallback );