#pragma once

#include "PCH.h"
#include "Link.h"

namespace vegascraft
{
	// Phase 3: MC HitActor → FNV damage; FNV hits → MC PlayerHurt input events.
	class Combat
	{
	public:
		void DrainMcEvents(Link& link);
		void OnHostHitPlayer(Link& link, float fnvDamage, std::uint16_t kind, std::uint32_t attackerFormId, bool powerAttack);

		static float ScaleMcToFnv(float mcDamage, std::uint16_t npcLevel);
		static float ScaleFnvToMc(float fnvDamage);

	private:
		std::uint64_t eventTail_{ 0 };
		std::uint64_t inputHead_{ 0 };
	};
}
