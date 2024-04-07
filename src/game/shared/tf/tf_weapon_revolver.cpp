//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_revolver.h"
#include "tf_fx_shared.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#endif

//=============================================================================
//
// Weapon Revolver tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFRevolver, DT_WeaponRevolver )

BEGIN_NETWORK_TABLE( CTFRevolver, DT_WeaponRevolver )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFRevolver )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_revolver, CTFRevolver );
PRECACHE_WEAPON_REGISTER( tf_weapon_revolver );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFRevolver )
END_DATADESC()
#endif

//=============================================================================
//
// Weapon Revolver functions.
//