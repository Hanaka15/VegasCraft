#pragma once

#include "PCH.h"

namespace vegascraft
{
	// Phase 6: map Minecraft actions onto FNV Actor Values / skills.
	class Skills
	{
	public:
		enum class Av : std::uint32_t
		{
			MeleeWeapons = 26,
			Guns = 32,
			EnergyWeapons = 33,
			Unarmed = 54,
			Repair = 48,
			Sneak = 51,
			// Armor skills in FNV are less granular; use DamageResistance progression via scripts / AV.
		};

		void Advance(Av skill, float uses);
		void OnMcHit(std::uint32_t weaponClass, bool ranged);
		void OnMcCraft();
		void OnMcSneakTick();
		void OnMcArmorHit(bool heavy);

	private:
		void ModAV(Av skill, float amount);
	};
}
