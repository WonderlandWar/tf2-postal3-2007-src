//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:			The Medic's Medikit weapon
//					
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "in_buttons.h"
#include "engine/IEngineSound.h"
#include "tf_gamerules.h"

#if defined( CLIENT_DLL )
#include <vgui_controls/Panel.h>
#include <vgui/isurface.h>
#include "particles_simple.h"
#include "c_tf_player.h"
#include "soundenvelope.h"
#else
#include "ndebugoverlay.h"
#include "tf_player.h"
#include "tf_team.h"
#include "tf_gamestats.h"
#include "ilagcompensationmanager.h"
#endif

#include "tf_weapon_medigun.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Buff ranges
ConVar weapon_medigun_damage_modifier( "weapon_medigun_damage_modifier", "1.5", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Scales the damage a player does while being healed with the medigun." );
ConVar weapon_medigun_construction_rate( "weapon_medigun_construction_rate", "10", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Constructing object health healed per second by the medigun." );
ConVar weapon_medigun_charge_rate( "weapon_medigun_charge_rate", "40", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Amount of time healing it takes to fully charge the medigun." );
ConVar weapon_medigun_chargerelease_rate( "weapon_medigun_chargerelease_rate", "8", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Amount of time it takes the a full charge of the medigun to be released." );

#if !defined (CLIENT_DLL)
ConVar tf_medigun_lagcomp(  "tf_medigun_lagcomp", "1", FCVAR_DEVELOPMENTONLY );
#endif

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifdef TFP3_MEDIGUN_FIX
void RecvProxy_HealingTarget( const CRecvProxyData *pData, void *pStruct, void *pOut )
#else
void RecvProxy_HealingTargets( const CRecvProxyData *pData, void *pStruct, void *pOut )
#endif
{
	CWeaponMedigun *pMedigun = ((CWeaponMedigun*)(pStruct));
	if ( pMedigun != NULL )
	{
		pMedigun->ForceHealingTargetUpdate();
	}

	RecvProxy_IntToEHandle( pData, pStruct, pOut );
}
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_medigun, CWeaponMedigun );
PRECACHE_WEAPON_REGISTER( tf_weapon_medigun );

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponMedigun, DT_WeaponMedigun )

BEGIN_NETWORK_TABLE( CWeaponMedigun, DT_WeaponMedigun )
#if !defined( CLIENT_DLL )
	SendPropFloat( SENDINFO(m_flChargeLevel), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN ),
#ifdef TFP3_MEDIGUN_FIX
	SendPropEHandle( SENDINFO( m_hHealingTarget ) ),
#else
	SendPropArray3( SENDINFO_ARRAY3(m_hHealingTargets), SendPropEHandle( SENDINFO_ARRAY(m_hHealingTargets), MAX_HEALING_TARGETS ) ),
#endif
	SendPropBool( SENDINFO( m_bHealing ) ),
	SendPropBool( SENDINFO( m_bAttacking ) ),
	SendPropBool( SENDINFO( m_bChargeRelease ) ),
	SendPropBool( SENDINFO( m_bHolstered ) ),	
#else
	RecvPropFloat( RECVINFO(m_flChargeLevel) ),
#ifdef TFP3_MEDIGUN_FIX
	RecvPropEHandle( RECVINFO( m_hHealingTarget ), RecvProxy_HealingTarget ),
#else
	RecvPropArray3( RECVINFO_ARRAY(m_hHealingTargets), RecvPropEHandle( RECVINFO( m_hHealingTargets[0] ), RecvProxy_HealingTargets ) ),
#endif
	RecvPropBool( RECVINFO( m_bHealing ) ),
	RecvPropBool( RECVINFO( m_bAttacking ) ),
	RecvPropBool( RECVINFO( m_bChargeRelease ) ),
	RecvPropBool( RECVINFO( m_bHolstered ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponMedigun  )

	DEFINE_PRED_FIELD( m_bHealing, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bAttacking, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bHolstered, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
#ifdef TFP3_MEDIGUN_FIX
	DEFINE_PRED_FIELD( m_hHealingTarget, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
#else
	DEFINE_PRED_ARRAY( m_hHealingTargets, FIELD_EHANDLE, MAX_HEALING_TARGETS, FTYPEDESC_INSENDTABLE ),
#endif
	DEFINE_FIELD( m_flHealEffectLifetime, FIELD_FLOAT ),

	DEFINE_PRED_FIELD( m_flChargeLevel, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bChargeRelease, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),

//	DEFINE_PRED_FIELD( m_bPlayingSound, FIELD_BOOLEAN ),
//	DEFINE_PRED_FIELD( m_bUpdateHealingTargets, FIELD_BOOLEAN ),

END_PREDICTION_DATA()
#endif

#define PARTICLE_PATH_VEL				140.0
#define NUM_PATH_PARTICLES_PER_SEC		300.0f
#define NUM_MEDIGUN_PATH_POINTS		8

extern ConVar tf_max_health_boost;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWeaponMedigun::CWeaponMedigun( void )
{
	WeaponReset();

	SetPredictionEligible( true );
}

CWeaponMedigun::~CWeaponMedigun()
{
#ifdef CLIENT_DLL
	if ( m_pChargedSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pChargedSound );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::WeaponReset( void )
{
	BaseClass::WeaponReset();

	m_flHealEffectLifetime = 0;

	m_bHealing = false;
	m_bAttacking = false;
	m_bHolstered = true;
	m_bChargeRelease = false;

	m_flNextBuzzTime = 0;
	m_flReleaseStartedAt = 0;
	m_flChargeLevel = 0.0f;

	RemoveHealingTarget( -1, true );

#if defined( CLIENT_DLL )
	m_bPlayingSound = false;
	m_bUpdateHealingTargets = false;
	m_bOldChargeRelease = false;

	UpdateEffects();
	ManageChargeEffect();

	m_pChargeEffect = NULL;
	m_pChargedSound = NULL;
#endif

}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::Precache()
{
	BaseClass::Precache();
	PrecacheScriptSound( "WeaponMedigun.NoTarget" );
	PrecacheScriptSound( "WeaponMedigun.Healing" );
	PrecacheScriptSound( "WeaponMedigun.Charged" );
	PrecacheParticleSystem( "medicgun_invulnstatus_fullcharge_blue" );
	PrecacheParticleSystem( "medicgun_invulnstatus_fullcharge_red" );
	PrecacheParticleSystem( "medicgun_beam_red_invun" );
	PrecacheParticleSystem( "medicgun_beam_red" );
	PrecacheParticleSystem( "medicgun_beam_blue_invun" );
	PrecacheParticleSystem( "medicgun_beam_blue" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponMedigun::Deploy( void )
{
	if ( BaseClass::Deploy() )
	{
		m_bHolstered = false;

#ifdef GAME_DLL
		CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
		if ( m_bChargeRelease && pOwner )
		{
			pOwner->m_Shared.RecalculateInvuln();
		}
#endif

#ifdef CLIENT_DLL
		ManageChargeEffect();
#endif

		m_flNextTargetCheckTime = gpGlobals->curtime;

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponMedigun::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	RemoveHealingTarget( -1, true );
	m_bAttacking = false;
	m_bHolstered = true;



#ifdef GAME_DLL
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( pOwner )
	{
		pOwner->m_Shared.RecalculateInvuln( true );
	}
#endif

#ifdef CLIENT_DLL
	UpdateEffects();
	ManageChargeEffect();
#endif

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::UpdateOnRemove( void )
{
	RemoveHealingTarget( -1, true );
	m_bAttacking = false;

#ifdef CLIENT_DLL
	if ( m_bPlayingSound )
	{
		m_bPlayingSound = false;
		StopHealSound();
	}

	UpdateEffects();
#endif



	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CWeaponMedigun::GetTargetRange( void )
{
	return (float)m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flRange;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CWeaponMedigun::GetStickRange( void )
{
	return (GetTargetRange() * 1.2);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CWeaponMedigun::GetHealRate( void )
{
	return (float)m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_nDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponMedigun::HealingTarget( CBaseEntity *pTarget )
{
#ifdef TFP3_MEDIGUN_FIX
	if ( pTarget == m_hHealingTarget )
		return true;
#else
	for ( int i = 0; i < MAX_HEALING_TARGETS; ++i )
	{
		if ( pTarget == m_hHealingTargets[i] )
			return true;
	}
#endif
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponMedigun::CouldHealTarget( CBaseEntity *pTarget )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return false;

	if ( pTarget->IsPlayer() && pTarget->IsAlive() && !HealingTarget(pTarget) )
	{		
		CTFPlayer *pTFPlayer = ToTFPlayer( pTarget );
		if ( !pTFPlayer )
			return false;

		bool bStealthed = pTFPlayer->m_Shared.InCond( TF_COND_STEALTHED );
		bool bDisguised = pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED );

		// We can heal teammates and enemies that are disguised as teammates
		if ( !bStealthed &&
			( pTFPlayer->InSameTeam( pOwner ) ||
			( bDisguised && pTFPlayer->m_Shared.GetDisguiseTeam() == pOwner->GetTeamNumber() ) ) )
		{
			return true;
		}
	}

	return false;
}

// Now make sure there isn't something other than team players in the way.
class CMedigunFilter : public CTraceFilterSimple
{
public:
	CMedigunFilter( CBaseEntity *pShooter ) : CTraceFilterSimple( pShooter, COLLISION_GROUP_WEAPON )
	{
		m_pShooter = pShooter;
	}

	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		// If it hit an edict the isn't the target and is on our team, then the ray is blocked.
		CBaseEntity *pEnt = static_cast<CBaseEntity*>(pHandleEntity);

		// Ignore collisions with the shooter
		if ( pEnt == m_pShooter )
			return false;
		
		if ( pEnt->GetTeam() == m_pShooter->GetTeam() )
			return false;

		return CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask );
	}

	CBaseEntity	*m_pShooter;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::MaintainTargetInSlot( int iSlot )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;
#ifdef TFP3_MEDIGUN_FIX
	CBaseEntity *pTarget = m_hHealingTarget;
#else
	CBaseEntity *pTarget = m_hHealingTargets[iSlot];
#endif
	Assert( pTarget );

	// Make sure the guy didn't go out of range.
	bool bLostTarget = true;
	Vector vecSrc = pOwner->Weapon_ShootPosition( );
	Vector vecTargetPoint = pTarget->WorldSpaceCenter();
	Vector vecPoint;

	// If it's brush built, use absmins/absmaxs
	pTarget->CollisionProp()->CalcNearestPoint( vecSrc, &vecPoint );

	float flDistance = (vecPoint - vecSrc).Length();
	if ( flDistance < GetStickRange() )
	{
		if ( m_flNextTargetCheckTime > gpGlobals->curtime )
			return;

		m_flNextTargetCheckTime = gpGlobals->curtime + 1.0f;

		trace_t tr;
		CMedigunFilter drainFilter( pOwner );

		Vector vecAiming;
		pOwner->EyeVectors( &vecAiming );

		Vector vecEnd = vecSrc + vecAiming * GetTargetRange();
		UTIL_TraceLine( vecSrc, vecEnd, (MASK_SHOT & ~CONTENTS_HITBOX), pOwner, DMG_GENERIC, &tr );

		// Still visible?
		if ( tr.m_pEnt == pTarget )
			return;

		UTIL_TraceLine( vecSrc, vecTargetPoint, MASK_SHOT, &drainFilter, &tr );

		// Still visible?
		if (( tr.fraction == 1.0f) || (tr.m_pEnt == pTarget))
			return;

		// If we failed, try the target's eye point as well
		UTIL_TraceLine( vecSrc, pTarget->EyePosition(), MASK_SHOT, &drainFilter, &tr );
		if (( tr.fraction == 1.0f) || (tr.m_pEnt == pTarget))
			return;
	}

	// We've lost this guy
	if ( bLostTarget )
	{
		RemoveHealingTarget( iSlot );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::FindNewTargetForSlot( int iSlot )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	Vector vecSrc = pOwner->Weapon_ShootPosition( );
#ifdef TFP3_MEDIGUN_FIX
	if ( m_hHealingTarget )
#else
	if ( m_hHealingTargets[iSlot] )
#endif
	{
		RemoveHealingTarget( iSlot );
	}

	// In Normal mode, we heal players under our crosshair
	Vector vecAiming;
	pOwner->EyeVectors( &vecAiming );

	// Find a player in range of this player, and make sure they're healable.
	Vector vecEnd = vecSrc + vecAiming * GetTargetRange();
	trace_t tr;

	UTIL_TraceLine( vecSrc, vecEnd, (MASK_SHOT & ~CONTENTS_HITBOX), pOwner, DMG_GENERIC, &tr );
	if ( tr.fraction != 1.0 && tr.m_pEnt )
	{
		if ( CouldHealTarget( tr.m_pEnt ) )
		{
#ifdef GAME_DLL
			pOwner->SpeakConceptIfAllowed( MP_CONCEPT_MEDIC_STARTEDHEALING );
			if ( tr.m_pEnt->IsPlayer() )
			{
				CTFPlayer *pTarget = ToTFPlayer( tr.m_pEnt );
				pTarget->SpeakConceptIfAllowed( MP_CONCEPT_HEALTARGET_STARTEDHEALING );
			}
#endif
#ifdef TFP3_MEDIGUN_FIX
			m_hHealingTarget.Set( tr.m_pEnt );
#else
			m_hHealingTargets.Set( iSlot, tr.m_pEnt );
#endif	
			m_flNextTargetCheckTime = gpGlobals->curtime + 1.0f;
		}			
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns a pointer to a healable target
//-----------------------------------------------------------------------------
bool CWeaponMedigun::FindAndHealTargets( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return false;

	bool bFound = false;

	// Maintaining beam to existing target?
#ifdef TFP3_MEDIGUN_FIX
	CBaseEntity *pTarget = m_hHealingTarget;
#else
	CBaseEntity *pTarget = m_hHealingTargets[0];
#endif
	if ( pTarget && pTarget->IsAlive() )
	{
		MaintainTargetInSlot( 0 );
	}
	else
	{	
		FindNewTargetForSlot( 0 );
	}
#ifdef TFP3_MEDIGUN_FIX
	CBaseEntity *pNewTarget = m_hHealingTarget;
#else
	CBaseEntity *pNewTarget = m_hHealingTargets[0];
#endif
	if ( pNewTarget && pNewTarget->IsAlive() )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pNewTarget );

#ifdef GAME_DLL
		// HACK: For now, just deal with players
		if ( pTFPlayer )
		{
			if ( pTarget != pNewTarget && pNewTarget->IsPlayer() )
			{
				pTFPlayer->m_Shared.Heal( pOwner, GetHealRate() );
			}

			pTFPlayer->m_Shared.RecalculateInvuln( false );
		}

		if ( m_flReleaseStartedAt && m_flReleaseStartedAt < (gpGlobals->curtime + 0.2) )
		{
			// When we start the release, everyone we heal rockets to full health
			pNewTarget->TakeHealth( pNewTarget->GetMaxHealth(), DMG_GENERIC );
		}
#endif
	
		bFound = true;

		// Charge up our power if we're not releasing it, and our target
		// isn't receiving any benefit from our healing.
		if ( !m_bChargeRelease )
		{
			if ( pTFPlayer )
			{
				int iBoostMax = floor( pTFPlayer->m_Shared.GetMaxBuffedHealth() * 0.95);

				if ( weapon_medigun_charge_rate.GetFloat() )
				{
					float flChargeAmount = gpGlobals->frametime / weapon_medigun_charge_rate.GetFloat();

					// Reduced charge for healing fully healed guys
					if ( pNewTarget->GetHealth() >= iBoostMax && ( TFGameRules() && !TFGameRules()->InSetup() ) )
					{
						flChargeAmount *= 0.5;
					}

					int iTotalHealers = pTFPlayer->m_Shared.GetNumHealers();
					if ( iTotalHealers > 1 )
					{
						flChargeAmount /= (float)iTotalHealers;
					}

					float flNewLevel = min( m_flChargeLevel + flChargeAmount, 1.0 );
#ifdef GAME_DLL
					if ( flNewLevel >= 1.0 && m_flChargeLevel < 1.0 )
					{
						pOwner->SpeakConceptIfAllowed( MP_CONCEPT_MEDIC_CHARGEREADY );

						if ( pTFPlayer )
						{
							pTFPlayer->SpeakConceptIfAllowed( MP_CONCEPT_HEALTARGET_CHARGEREADY );
						}
					}
#endif
					m_flChargeLevel = flNewLevel;
				}
			}
		}
	}

	return bFound;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::ItemHolsterFrame( void )
{
	BaseClass::ItemHolsterFrame();

	DrainCharge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::DrainCharge( void )
{
	// If we're in charge release mode, drain our charge
	if ( m_bChargeRelease )
	{
		CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
		if ( !pOwner )
			return;

		float flChargeAmount = gpGlobals->frametime / weapon_medigun_chargerelease_rate.GetFloat();
		m_flChargeLevel = max( m_flChargeLevel - flChargeAmount, 0.0 );
		if ( !m_flChargeLevel )
		{
			m_bChargeRelease = false;
			m_flReleaseStartedAt = 0;

#ifdef GAME_DLL
			/*
			if ( m_bHealingSelf )
			{
				m_bHealingSelf = false;
				pOwner->m_Shared.StopHealing( pOwner );
			}
			*/

			pOwner->m_Shared.RecalculateInvuln();
#endif
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Overloaded to handle the hold-down healing
//-----------------------------------------------------------------------------
void CWeaponMedigun::ItemPostFrame( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	DrainCharge();

	if ( m_bLowered )
		return;

#if !defined( CLIENT_DLL )
	if ( AppliesModifier() )
	{
		m_DamageModifier.SetModifier( weapon_medigun_damage_modifier.GetFloat() );
	}
#endif

	// Try to start healing
	m_bAttacking = false;

	if ( /*m_bChargeRelease || */ pOwner->m_nButtons & IN_ATTACK )
	{
		PrimaryAttack();
		m_bAttacking = true;
	}
 	else if ( m_bHealing )
 	{
 		// Detach from the player if they release the attack button.
 		RemoveHealingTarget( -1 );
 	}

	if ( pOwner->m_nButtons & IN_ATTACK2 )
	{
		SecondaryAttack();
	}

	WeaponIdle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponMedigun::Lower( void )
{
	// Stop healing if we are
	if ( m_bHealing )
	{
		RemoveHealingTarget( -1, true );
		m_bAttacking = false;

#ifdef CLIENT_DLL
		UpdateEffects();
#endif
	}

	return BaseClass::Lower();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::RemoveHealingTarget( int iSlot, bool bStopHealingSelf )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;
#ifdef TFP3_MEDIGUN_FIX
	if ( m_hHealingTarget )
#else
	if ( m_hHealingTargets[iSlot] )
#endif
	{
		// HACK: For now, just deal with players
#ifdef TFP3_MEDIGUN_FIX
		if ( m_hHealingTarget->IsPlayer() )
#else
		if ( m_hHealingTargets[iSlot]->IsPlayer() )
#endif
		{
			if ( !iSlot || iSlot == -1 )
			{
	#ifdef GAME_DLL
				CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
#ifdef TFP3_MEDIGUN_FIX
				CTFPlayer *pTFPlayer = ToTFPlayer( m_hHealingTarget );
#else
				CTFPlayer *pTFPlayer = ToTFPlayer( m_hHealingTargets[0] );
#endif
				pTFPlayer->m_Shared.StopHealing( pOwner );
				pTFPlayer->m_Shared.RecalculateInvuln( false );

				pOwner->SpeakConceptIfAllowed( MP_CONCEPT_MEDIC_STOPPEDHEALING, pTFPlayer->IsAlive() ? "healtarget:alive" : "healtarget:dead" );
				pTFPlayer->SpeakConceptIfAllowed( MP_CONCEPT_HEALTARGET_STOPPEDHEALING );
	#endif
#ifdef TFP3_MEDIGUN_FIX
				m_hHealingTarget.Set( NULL );
#else
				m_hHealingTargets.Set( 0, NULL );
#endif	
			}
		}
	}

	// Stop the welding animation
	if ( m_bHealing )
	{
		SendWeaponAnim( ACT_MP_ATTACK_STAND_POSTFIRE );
		pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_POST );
	}

#ifndef CLIENT_DLL
	m_DamageModifier.RemoveModifier();
#endif
	m_bHealing = false;

}


//-----------------------------------------------------------------------------
// Purpose: Attempt to heal any player within range of the medikit
//-----------------------------------------------------------------------------
void CWeaponMedigun::PrimaryAttack( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;


	if ( !CanAttack() )
		return;

#ifdef GAME_DLL
	/*
	// Start boosting ourself if we're not
	if ( m_bChargeRelease && !m_bHealingSelf )
	{
		pOwner->m_Shared.Heal( pOwner, GetHealRate() * 2 );
		m_bHealingSelf = true;
	}
	*/
#endif

#if !defined (CLIENT_DLL)
	if ( tf_medigun_lagcomp.GetBool() )
		lagcompensation->StartLagCompensation( pOwner, pOwner->GetCurrentCommand() );
#endif

	if ( FindAndHealTargets() )
	{
		// Start the animation
		if ( !m_bHealing )
		{
#ifdef GAME_DLL
			pOwner->SpeakWeaponFire();
#endif

			SendWeaponAnim( ACT_MP_ATTACK_STAND_PREFIRE );
			pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRE );
		}

		m_bHealing = true;
	}
	else
	{
		RemoveHealingTarget( -1 );
	}
	
#if !defined (CLIENT_DLL)
	if ( tf_medigun_lagcomp.GetBool() )
		lagcompensation->FinishLagCompensation( pOwner );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Burn charge level to generate invulnerability
//-----------------------------------------------------------------------------
void CWeaponMedigun::SecondaryAttack( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	if ( !CanAttack() )
		return;

	// Ensure they have a full charge and are not already in charge release mode
	if ( m_flChargeLevel < 1.0 || m_bChargeRelease )
	{
#ifdef CLIENT_DLL
		// Deny, flash
		if ( !m_bChargeRelease && m_flFlashCharge <= 0 )
		{
			m_flFlashCharge = 10;
			pOwner->EmitSound( "Player.DenyWeaponSelection" );
		}
#endif
		return;
	}

	if ( pOwner->HasTheFlag() )
	{
#ifdef GAME_DLL
		CSingleUserRecipientFilter filter( pOwner );
		TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_NO_INVULN_WITH_FLAG );
#endif
		pOwner->EmitSound( "Player.DenyWeaponSelection" );
		return;
	}

	// Start super charge
	m_bChargeRelease = true;
	m_flReleaseStartedAt = 0;//gpGlobals->curtime;

#ifdef GAME_DLL
	CTF_GameStats.Event_PlayerInvulnerable( pOwner );
	pOwner->m_Shared.RecalculateInvuln();

	pOwner->SpeakConceptIfAllowed( MP_CONCEPT_MEDIC_CHARGEDEPLOYED );
#ifdef TFP3_MEDIGUN_FIX
	if ( m_hHealingTarget && m_hHealingTarget->IsPlayer() )
#else
	if ( m_hHealingTargets[0] && m_hHealingTargets[0]->IsPlayer() )
#endif
	{
#ifdef TFP3_MEDIGUN_FIX
		CTFPlayer *pTFPlayer = ToTFPlayer( m_hHealingTarget );
#else
		CTFPlayer *pTFPlayer = ToTFPlayer( m_hHealingTargets[0] );
#endif
		pTFPlayer->m_Shared.RecalculateInvuln();
		pTFPlayer->SpeakConceptIfAllowed( MP_CONCEPT_HEALTARGET_CHARGEDEPLOYED );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Idle tests to see if we're facing a valid target for the medikit
//			If so, move into the "heal-able" animation. 
//			Otherwise, move into the "not-heal-able" animation.
//-----------------------------------------------------------------------------
void CWeaponMedigun::WeaponIdle( void )
{
	if ( HasWeaponIdleTimeElapsed() )
	{
		// Loop the welding animation
		if ( m_bHealing )
		{
			SendWeaponAnim( ACT_VM_PRIMARYATTACK );
			return;
		}

		return BaseClass::WeaponIdle();
	}
}

#if defined( CLIENT_DLL )
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::StopHealSound( bool bStopHealingSound, bool bStopNoTargetSound )
{
	if ( bStopHealingSound )
	{
		StopSound( "WeaponMedigun.Healing" );
	}

	if ( bStopNoTargetSound )
	{
		StopSound( "WeaponMedigun.NoTarget" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::ManageChargeEffect( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	C_BaseEntity *pEffectOwner = this;

	if ( pLocalPlayer == NULL )
		return;

	if ( pLocalPlayer == GetTFPlayerOwner() )
	{
		pEffectOwner = pLocalPlayer->GetViewModel();
		if ( !pEffectOwner )
			return;
	}

	bool bOwnerTaunting = false;

	if ( GetTFPlayerOwner() && GetTFPlayerOwner()->m_Shared.InCond( TF_COND_TAUNTING ) == true )
	{
		bOwnerTaunting = true;
	}

	if ( GetTFPlayerOwner() && bOwnerTaunting == false && m_bHolstered == false && ( m_flChargeLevel >= 1.0f || m_bChargeRelease == true ) )
	{
		if ( m_pChargeEffect == NULL )
		{
			char *pszEffectName = NULL;

			switch( GetTFPlayerOwner()->GetTeamNumber() )
			{
			case TF_TEAM_BLUE:
				pszEffectName = "medicgun_invulnstatus_fullcharge_blue";
				break;
			case TF_TEAM_RED:
				pszEffectName = "medicgun_invulnstatus_fullcharge_red";
				break;
			default:
				pszEffectName = "";
				break;
			}

			m_pChargeEffect = pEffectOwner->ParticleProp()->Create( pszEffectName, PATTACH_POINT_FOLLOW, "muzzle" );
		}

		if ( m_pChargedSound == NULL )
		{
			CLocalPlayerFilter filter;

			CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

			m_pChargedSound = controller.SoundCreate( filter, entindex(), "WeaponMedigun.Charged" );
			controller.Play( m_pChargedSound, 1.0, 100 );
		}
	}
	else
	{
		if ( m_pChargeEffect != NULL )
		{
			pEffectOwner->ParticleProp()->StopEmission( m_pChargeEffect );
			m_pChargeEffect = NULL;
		}

		if ( m_pChargedSound != NULL )
		{
			CSoundEnvelopeController::GetController().SoundDestroy( m_pChargedSound );
			m_pChargedSound = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void CWeaponMedigun::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( m_bUpdateHealingTargets )
	{
		UpdateEffects();
		m_bUpdateHealingTargets = false;
	}

	// Think?
	if ( m_bHealing )
	{
		ClientThinkList()->SetNextClientThink( GetClientHandle(), CLIENT_THINK_ALWAYS );
	}
	else
	{
		ClientThinkList()->SetNextClientThink( GetClientHandle(), CLIENT_THINK_NEVER );
		m_bPlayingSound = false;
		StopHealSound( true, false );

		// Are they holding the attack button but not healing anyone? Give feedback.
		if ( IsActiveByLocalPlayer() && GetOwner() && GetOwner()->IsAlive() && m_bAttacking && GetOwner() == C_BasePlayer::GetLocalPlayer() && CanAttack() == true )
		{
			if ( gpGlobals->curtime >= m_flNextBuzzTime )
			{
				CLocalPlayerFilter filter;
				EmitSound( filter, entindex(), "WeaponMedigun.NoTarget" );
				m_flNextBuzzTime = gpGlobals->curtime + 0.5f;	// only buzz every so often.
			}
		}
		else
		{
			StopHealSound( false, true );	// Stop the "no target" sound.
		}
	}

	ManageChargeEffect();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::ClientThink()
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	// Don't show it while the player is dead. Ideally, we'd respond to m_bHealing in OnDataChanged,
	// but it stops sending the weapon when it's holstered, and it gets holstered when the player dies.
	CTFPlayer *pFiringPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( !pFiringPlayer || pFiringPlayer->IsPlayerDead() || pFiringPlayer->IsDormant() )
	{
		ClientThinkList()->SetNextClientThink( GetClientHandle(), CLIENT_THINK_NEVER );
		m_bPlayingSound = false;
		StopHealSound();
		return;
	}
		
	// If the local player is the guy getting healed, let him know 
	// who's healing him, and their charge level.
#ifdef TFP3_MEDIGUN_FIX
	if( m_hHealingTarget != NULL )
#else
	if( m_hHealingTargets[0] != NULL )
#endif
	{
#ifdef TFP3_MEDIGUN_FIX
		if ( pLocalPlayer == m_hHealingTarget )
#else
		if ( pLocalPlayer == m_hHealingTargets[0] )
#endif
		{
			pLocalPlayer->SetHealer( pFiringPlayer, m_flChargeLevel );
		}

		if ( !m_bPlayingSound )
		{
			m_bPlayingSound = true;
			CLocalPlayerFilter filter;
			EmitSound( filter, entindex(), "WeaponMedigun.Healing" );
		}
	}

	if ( m_bOldChargeRelease != m_bChargeRelease )
	{
		m_bOldChargeRelease = m_bChargeRelease;
		ForceHealingTargetUpdate();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::UpdateEffects( void )
{
	CTFPlayer *pFiringPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( !pFiringPlayer )
		return;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	C_BaseEntity *pEffectOwner = this;
	if ( pLocalPlayer == pFiringPlayer )
	{
		pEffectOwner = pLocalPlayer->GetViewModel();
		if ( !pEffectOwner )
			return;
	}

	// Remove all the effects
		
	for ( int i = 0; i < m_hHealingTargetEffects.Count(); ++i )
	{
		if ( pEffectOwner )
		{
			pEffectOwner->ParticleProp()->StopEmission( m_hHealingTargetEffects[i].pEffect );
		}
		else
		{
			m_hHealingTargetEffects[i].pEffect->StopEmission();
		}

		m_hHealingTargetEffects.Remove( i );
	}

	// Don't add targets if the medic is dead
	if ( !pEffectOwner || pFiringPlayer->IsPlayerDead() )
		return;

	// Add our targets
	// Loops through the healing targets, and make sure we have an effect for each of them
#ifdef TFP3_MEDIGUN_FIX
	if ( m_hHealingTarget )
#else
	if ( m_hHealingTargets[0] )
#endif
	{
		if ( m_hHealingTargetEffects.Count() <= 0 )
		{			
			const char *pszEffectName;
			if ( pFiringPlayer->GetTeamNumber() == TF_TEAM_RED )
			{
				if ( m_bChargeRelease )
				{
					pszEffectName = "medicgun_beam_red_invun";
				}
				else
				{
					pszEffectName = "medicgun_beam_red";
				}
			}
			else
			{
				if ( m_bChargeRelease )
				{
					pszEffectName = "medicgun_beam_blue_invun";
				}
				else
				{
					pszEffectName = "medicgun_beam_blue";
				}
			}

			CNewParticleEffect *pEffect = pEffectOwner->ParticleProp()->Create( pszEffectName, PATTACH_POINT_FOLLOW, "muzzle" );
#ifdef TFP3_MEDIGUN_FIX
			pEffectOwner->ParticleProp()->AddControlPoint( pEffect, 1, m_hHealingTarget, PATTACH_ABSORIGIN_FOLLOW, NULL, Vector(0,0,50) );
#else
			pEffectOwner->ParticleProp()->AddControlPoint( pEffect, 1, m_hHealingTargets[0], PATTACH_ABSORIGIN_FOLLOW, NULL, Vector(0,0,50) );
#endif
			//m_hHealingTargetEffects.InsertBefore( i, healingtargeteffects_t() );
			m_hHealingTargetEffects.AddToTail( healingtargeteffects_t() );
#ifdef TFP3_MEDIGUN_FIX
			m_hHealingTargetEffects[m_hHealingTargetEffects.Count()-1].pTarget = m_hHealingTarget;
#else
			m_hHealingTargetEffects[m_hHealingTargetEffects.Count()-1].pTarget = m_hHealingTargets[0];
#endif	
			m_hHealingTargetEffects[m_hHealingTargetEffects.Count()-1].pEffect = pEffect;
		}
		else
		{
			for ( int i = 0; i < m_hHealingTargetEffects.Count(); ++i )
			{
#ifdef TFP3_MEDIGUN_FIX
				if ( m_hHealingTargetEffects[i].pTarget == m_hHealingTarget )
#else
				if ( m_hHealingTargetEffects[i].pTarget == m_hHealingTargets[0] )
#endif
					return;

				const char *pszEffectName;
				if ( pFiringPlayer->GetTeamNumber() == TF_TEAM_RED )
				{
					if ( m_bChargeRelease )
					{
						pszEffectName = "medicgun_beam_red_invun";
					}
					else
					{
						pszEffectName = "medicgun_beam_red";
					}
				}
				else
				{
					if ( m_bChargeRelease )
					{
						pszEffectName = "medicgun_beam_blue_invun";
					}
					else
					{
						pszEffectName = "medicgun_beam_blue";
					}
				}

				CNewParticleEffect *pEffect = pEffectOwner->ParticleProp()->Create( pszEffectName, PATTACH_POINT_FOLLOW, "muzzle" );
#ifdef TFP3_MEDIGUN_FIX
				pEffectOwner->ParticleProp()->AddControlPoint( pEffect, 1, m_hHealingTarget, PATTACH_ABSORIGIN_FOLLOW, NULL, Vector(0,0,50) );
#else
				pEffectOwner->ParticleProp()->AddControlPoint( pEffect, 1, m_hHealingTargets[0], PATTACH_ABSORIGIN_FOLLOW, NULL, Vector(0,0,50) );
#endif			
				//m_hHealingTargetEffects.InsertBefore( i, healingtargeteffects_t() );
				m_hHealingTargetEffects.AddToTail( healingtargeteffects_t() );
	
#ifdef TFP3_MEDIGUN_FIX
				m_hHealingTargetEffects[i].pTarget = m_hHealingTarget;
#else
				m_hHealingTargetEffects[i].pTarget = m_hHealingTargets[0];
#endif	
				m_hHealingTargetEffects[i].pEffect = pEffect;
			}
		}
	}
}
#endif
