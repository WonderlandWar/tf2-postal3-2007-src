//========= Copyright © 1996-2004, Valve LLC, All rights reserved. ============
//
//=============================================================================

#include "cbase.h"
#include "tf_player.h"
#include "tf_playerclass.h"

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFPlayerClass::CTFPlayerClass()
{
	m_bIsBuilding = false;
	m_Building = NULL;
	m_flBuildingWaitTime = 0.0;
	real_owner = NULL;
	m_fBuildings = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Deconstructor
//-----------------------------------------------------------------------------
CTFPlayerClass::~CTFPlayerClass()
{
}