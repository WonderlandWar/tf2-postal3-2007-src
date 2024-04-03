//========= Copyright © 1996-2004, Valve LLC, All rights reserved. ============
//
//=============================================================================

#ifndef TF_PLAYERCLASS_H
#define TF_PLAYERCLASS_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_playerclass_shared.h"

//-----------------------------------------------------------------------------
// TF Player Class
//-----------------------------------------------------------------------------
class CTFPlayerClass : public CTFPlayerClassShared
{
public:

	CTFPlayerClass();
	~CTFPlayerClass();

	void Engineer_RemoveBuildings( void );
	bool Engineer_HasBuilding( int );
	bool Engineer_IsBuilding( void );

private:
	
	bool m_bIsBuilding;
    CHandle<CBaseEntity> m_Building;
    float m_flBuildingWaitTime;
    class CHandle<CBaseEntity> real_owner;
    unsigned int m_fBuildings;

};

#endif // TF_PLAYERCLASS_H