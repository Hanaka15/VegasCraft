#include "Skills.h"

namespace vegascraft
{
	void Skills::ModAV(Av skill, float amount)
	{
		(void)skill;
		(void)amount;
		// Hook: PlayerCharacter::ModActorValue / AdvanceSkill equivalent.
	}

	void Skills::Advance(Av skill, float uses)
	{
		ModAV(skill, uses);
	}

	void Skills::OnMcHit(std::uint32_t weaponClass, bool ranged)
	{
		if (ranged || weaponClass == 5) {
			Advance(Av::Guns, 1.0f);
			return;
		}
		if (weaponClass == 0) {
			Advance(Av::Unarmed, 1.0f);
			return;
		}
		Advance(Av::MeleeWeapons, 1.0f);
	}

	void Skills::OnMcCraft()
	{
		Advance(Av::Repair, 1.0f);
	}

	void Skills::OnMcSneakTick()
	{
		Advance(Av::Sneak, 0.05f);
	}

	void Skills::OnMcArmorHit(bool heavy)
	{
		(void)heavy;
		// FNV: grant XP via script or light/heavy-equivalent AV when RE finds a clean path.
	}
}
