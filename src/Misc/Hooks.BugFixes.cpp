#include <AnimClass.h>
#include <BuildingClass.h>
#include <TechnoClass.h>
#include <FootClass.h>
#include <UnitClass.h>
#include <ScenarioClass.h>
#include <VoxelAnimClass.h>
#include <BulletClass.h>
#include <Ext/Rules/Body.h>

#include <Utilities/Macro.h>
#include <Utilities/Debug.h>
#include <Utilities/TemplateDef.h>

//Replace: checking of HasExtras = > checking of (HasExtras && Shadow)
DEFINE_HOOK(0x423365, Phobos_BugFixes_SHPShadowCheck, 0x8)
{
	GET(AnimClass*, pAnim, ESI);
	return (pAnim->Type->Shadow && pAnim->HasExtras) ?
		0x42336D :
		0x4233EE;
}

/*
	Allow usage of TileSet of 255 and above without making NE-SW broken bridges unrepairable

	When TileSet number crosses 255 in theater INI files, the NE-SW broken bridges
	become non-repairable. The reason is that the NonMarbleMadnessTile array of size 256
	overflows when filled and affects the variables like BridgeTopRight1 and BridgeBottomLeft2
	that come after it. This patch removes the filling of the unused NonMarbleMadnessTile array
	and its references.

	Author : E1 Elite
*/
DEFINE_LJMP(0x545CE2, 0x545CE9) //Phobos_BugFixes_Tileset255_RemoveNonMMArrayFill
DEFINE_LJMP(0x546C23, 0x546C8B) //Phobos_BugFixes_Tileset255_RefNonMMArray


// WWP's shit code! Wrong check.
// To avoid units dying when they are already dead.
DEFINE_HOOK(0x5F53AA, ObjectClass_ReceiveDamage_DyingFix, 0x6)
{
	enum { PostMortem = 0x5F583E, ContinueCheck = 0x5F53B0 };

	GET(int, health, EAX);
	GET(ObjectClass*, pThis, ESI);

	if (health <= 0 || !pThis->IsAlive)
		return PostMortem;

	return ContinueCheck;
}

DEFINE_HOOK(0x4D7431, FootClass_ReceiveDamage_DyingFix, 0x5)
{
	GET(FootClass*, pThis, ESI);
	GET(DamageState, result, EAX);

	if (result != DamageState::PostMortem && (pThis->IsSinking || (!pThis->IsAttackedByLocomotor && pThis->IsCrashing)))
		R->EAX(DamageState::PostMortem);

	return 0;
}

DEFINE_HOOK(0x737D57, UnitClass_ReceiveDamage_DyingFix, 0x7)
{
	GET(UnitClass*, pThis, ESI);
	GET(DamageState, result, EAX);

	if (result != DamageState::PostMortem && pThis->DeathFrameCounter > 0)
		R->EAX(DamageState::PostMortem);

	return 0;
}

// Restore DebrisMaximums logic (issue #109)
// Author: Otamaa
DEFINE_HOOK(0x702299, TechnoClass_ReceiveDamage_DebrisMaximumsFix, 0xA)
{
	GET(TechnoClass* const, pThis, ESI);

	auto pType = pThis->GetTechnoType();

	// If DebrisMaximums has one value, then legacy behavior is used
	if (pType->DebrisMaximums.Count == 1 &&
		pType->DebrisMaximums.GetItem(0) > 0 &&
		pType->DebrisTypes.Count > 0)
	{
		return 0;
	}

	auto totalSpawnAmount = ScenarioClass::Instance->Random.RandomRanged(
		pType->MinDebris, pType->MaxDebris);

	if (pType->DebrisTypes.Count > 0 && pType->DebrisMaximums.Count > 0)
	{
		auto cord = pThis->GetCoords();
		for (int currentIndex = 0; currentIndex < pType->DebrisTypes.Count; ++currentIndex)
		{
			if (pType->DebrisMaximums.GetItem(currentIndex) > 0)
			{
				int adjustedMaximum = Math::min(pType->DebrisMaximums.GetItem(currentIndex), pType->MaxDebris);
				int amountToSpawn = ScenarioClass::Instance->Random.Random() % (adjustedMaximum + 1); //0x702337
				amountToSpawn = Math::min(amountToSpawn, totalSpawnAmount);
				totalSpawnAmount -= amountToSpawn;

				for (; amountToSpawn > 0; --amountToSpawn)
				{
					GameCreate<VoxelAnimClass>(pType->DebrisTypes.GetItem(currentIndex),
						&cord, pThis->Owner);
				}

				if (totalSpawnAmount < 1)
					break;
			}
		}
	}

	R->EBX(totalSpawnAmount);

	return 0x7023E5;
}

// issue #112 Make FireOnce=yes work on other TechnoTypes
// Author: Starkku
DEFINE_HOOK(0x4C7518, EventClass_Execute_StopUnitDeployFire, 0x9)
{
	GET(TechnoClass* const, pThis, ESI);

	auto const pUnit = abstract_cast<UnitClass*>(pThis);
	if (pUnit && pUnit->CurrentMission == Mission::Unload && pUnit->Type->DeployFire && !pUnit->Type->IsSimpleDeployer)
		pUnit->QueueMission(Mission::Guard, true);

	// Restore overridden instructions
	GET(Mission, eax, EAX);
	return eax == Mission::Construction ? 0x4C8109 : 0x4C7521;
}

DEFINE_HOOK(0x73DD12, UnitClass_Mission_Unload_DeployFire, 0x6)
{
	GET(UnitClass*, pThis, ESI);

	int weaponIndex = pThis->GetTechnoType()->DeployFireWeapon;

	if (pThis->GetFireError(pThis->Target, weaponIndex, true) == FireError::OK)
	{
		pThis->Fire(pThis->Target, weaponIndex);
		auto const pWeapon = pThis->GetWeapon(weaponIndex);

		if (pWeapon && pWeapon->WeaponType->FireOnce)
			pThis->QueueMission(Mission::Guard, true);
	}

	return 0x73DD3C;
}

// issue #250: Building placement hotkey not responding
// Author: Uranusian
DEFINE_LJMP(0x4ABBD5, 0x4ABBD5 + 7); // DisplayClass_MouseLeftRelease_HotkeyFix

DEFINE_HOOK(0x4FB2DE, HouseClass_PlaceObject_HotkeyFix, 0x6)
{
	GET(TechnoClass*, pObject, ESI);

	pObject->ClearSidebarTabObject();

	return 0;
}

// Issue #46: Laser is mirrored relative to FireFLH
// Author: Starkku
DEFINE_HOOK(0x6FF2BE, TechnoClass_Fire_At_BurstOffsetFix_1, 0x6)
{
	GET(TechnoClass*, pThis, ESI);

	--pThis->CurrentBurstIndex;

	return 0x6FF2D1;
}

DEFINE_HOOK(0x6FF660, TechnoClass_Fire_At_BurstOffsetFix_2, 0x6)
{
	GET(TechnoClass*, pThis, ESI);
	GET_BASE(int, weaponIndex, 0xC);

	++pThis->CurrentBurstIndex;
	pThis->CurrentBurstIndex %= pThis->GetWeapon(weaponIndex)->WeaponType->Burst;

	return 0;
}

// issue #290: Undeploy building into a unit plays EVA_NewRallyPointEstablished
// Author: secsome
DEFINE_HOOK(0x44377E, BuildingClass_ActiveClickWith, 0x6)
{
	GET(BuildingClass*, pThis, ESI);
	GET_STACK(CellStruct*, pCell, STACK_OFFS(0x84, -0x8));

	if (pThis->GetTechnoType()->UndeploysInto)
		pThis->SetRallypoint(pCell, false);
	else if (pThis->IsUnitFactory())
		pThis->SetRallypoint(pCell, true);

	return 0x4437AD;
}

// issue #232: Naval=yes overrides WaterBound=no and prevents move orders onto Land cells
// Author: Uranusian
DEFINE_LJMP(0x47CA05, 0x47CA33); // CellClass_IsClearToBuild_SkipNaval

// bugfix: DeathWeapon not properly detonates
// Author: Uranusian
DEFINE_HOOK(0x70D77F, TechnoClass_FireDeathWeapon_ProjectileFix, 0x8)
{
	GET(BulletClass*, pBullet, EBX);
	GET(CoordStruct*, pCoord, EAX);

	pBullet->SetLocation(*pCoord);
	pBullet->Explode(true);

	return 0x70D787;
}

// Fix [JumpjetControls] obsolete in RA2/YR
// Author: Uranusian
DEFINE_HOOK(0x7115AE, TechnoTypeClass_CTOR_JumpjetControls, 0xA)
{
	GET(TechnoTypeClass*, pThis, ESI);
	auto pRules = RulesClass::Instance();
	auto pRulesExt = RulesExt::Global();

	pThis->JumpjetTurnRate = pRules->TurnRate;
	pThis->JumpjetSpeed = pRules->Speed;
	pThis->JumpjetClimb = static_cast<float>(pRules->Climb);
	pThis->JumpjetCrash = static_cast<float>(pRulesExt->JumpjetCrash);
	pThis->JumpjetHeight = pRules->CruiseHeight;
	pThis->JumpjetAccel = static_cast<float>(pRules->Acceleration);
	pThis->JumpjetWobbles = static_cast<float>(pRules->WobblesPerSecond);
	pThis->JumpjetNoWobbles = pRulesExt->JumpjetNoWobbles;
	pThis->JumpjetDeviation = pRules->WobbleDeviation;

	return 0x711601;
}

// skip vanilla JumpjetControls and make it earlier load
DEFINE_LJMP(0x668EB5, 0x668EBD); // RulesClass_Process_SkipJumpjetControls

DEFINE_HOOK(0x52D0F9, InitRules_EarlyLoadJumpjetControls, 0x6)
{
	GET(RulesClass*, pThis, ECX);
	GET(CCINIClass*, pINI, EAX);

	pThis->Read_JumpjetControls(pINI);

	return 0;
}

DEFINE_HOOK(0x6744E4, RulesClass_ReadJumpjetControls_Extra, 0x7)
{
	auto pRulesExt = RulesExt::Global();
	if (!pRulesExt)
		return 0;

	GET(CCINIClass*, pINI, EDI);
	INI_EX exINI(pINI);

	pRulesExt->JumpjetCrash.Read(exINI, "JumpjetControls", "Crash");
	pRulesExt->JumpjetNoWobbles.Read(exINI, "JumpjetControls", "NoWobbles");

	return 0;
}