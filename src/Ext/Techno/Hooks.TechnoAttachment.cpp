#include "Body.h"

#include <Ext/TechnoType/Body.h>

DEFINE_HOOK(0x4DA86E, FootClass_AI_UpdateAttachedLocomotion, 0x0)
{
	GET(FootClass*, pThis, ESI);

	if (!TechnoExt::AttachmentAI(pThis))
		pThis->Locomotor->Process();

	return 0x4DA87A;
}

DEFINE_HOOK(0x710460, TechnoClass_Destroy_HandleAttachments, 0x6)
{
	GET(TechnoClass*, pThis, ECX);

	TechnoExt::HandleHostDestruction(pThis);

	auto const pExt = TechnoExt::ExtMap.Find(pThis);
	if (pExt->ParentAttachment)
		pExt->ParentAttachment->ChildDestroyed();

	pExt->ParentAttachment = nullptr;

	return 0;
}

DEFINE_HOOK(0x6F6F20, TechnoClass_Unlimbo_UnlimboAttachments, 0x6)
{
	GET(TechnoClass*, pThis, ESI);

	TechnoExt::UnlimboAttachments(pThis);

	return 0;
}

DEFINE_HOOK(0x6F6B1C, TechnoClass_Limbo_LimboAttachments, 0x6)
{
	GET(TechnoClass*, pThis, ESI);

	TechnoExt::LimboAttachments(pThis);

	return 0;
}

DEFINE_HOOK(0x73F528, UnitClass_CanEnterCell_SkipChildren, 0x0)
{
	enum { IgnoreOccupier = 0x73FC10, Continue = 0x73F530 };

	GET(UnitClass*, pThis, EBX);
	GET(TechnoClass*, pOccupier, ESI);

	if (pThis == pOccupier || TechnoExt::IsParentOf(pThis, pOccupier))
		return IgnoreOccupier;

	return Continue;
}

DEFINE_HOOK(0x51C251, InfantryClass_CanEnterCell_SkipChildren, 0x0)
{
	enum { IgnoreOccupier = 0x51C70F, Continue = 0x51C259 };

	GET(InfantryClass*, pThis, EBP);
	GET(TechnoClass*, pOccupier, ESI);

	if ((TechnoClass*)pThis == pOccupier || TechnoExt::IsParentOf((TechnoClass*)pThis, pOccupier))
		return IgnoreOccupier;

	return Continue;
}

// TODO
// 0x4DEAE0 IC for footclass
// 0x457C90 IC (forceshield) for buildings
// 0x6CCCCA Chrono Warp
// 0x4694BB Temporal warhead
// 0x4696FB Locomotor warhead
// ...