#include "Combat.h"

namespace vegascraft
{
	float Combat::ScaleMcToFnv(float mcDamage, std::uint16_t npcLevel)
	{
		return mcDamage * (5.0f + 0.25f * static_cast<float>(npcLevel));
	}

	float Combat::ScaleFnvToMc(float fnvDamage)
	{
		return fnvDamage / 5.0f;
	}

	void Combat::OnHostHitPlayer(Link& link, float fnvDamage, std::uint16_t kind, std::uint32_t attackerFormId, bool powerAttack)
	{
		auto* base = link.Base();
		if (!base) {
			return;
		}
		const float mcDmg = ScaleFnvToMc(fnvDamage);
		const std::int32_t a = static_cast<std::int32_t>(mcDmg * 100.0f);
		const std::int32_t b = static_cast<std::int32_t>(attackerFormId);
		std::uint32_t flags = 0;
		if (powerAttack) {
			flags |= proto::kHurtPowerAttack;
		}
		const std::uint64_t off = proto::kOffInputRing + 0x80 + (inputHead_ % proto::kInputRingEntries) * 16ull;
		std::uint8_t* dst = base + off;
		const std::uint16_t type = proto::kInHurt;
		std::memcpy(dst + 0, &type, 2);
		std::memcpy(dst + 2, &kind, 2);
		std::memcpy(dst + 4, &a, 4);
		std::memcpy(dst + 8, &b, 4);
		std::memcpy(dst + 12, &flags, 4);
		++inputHead_;
		*reinterpret_cast<std::uint64_t*>(base + proto::kOffInputRing) = inputHead_;
	}

	void Combat::DrainMcEvents(Link& link)
	{
		auto* base = link.Base();
		if (!base) {
			return;
		}
		const std::uint64_t head = *reinterpret_cast<std::uint64_t*>(base + proto::kOffEventRing);
		while (eventTail_ < head) {
			const std::uint64_t off = proto::kOffEventRing + 0x80 + (eventTail_ % proto::kEventRingEntries) * 32ull;
			proto::McEvent ev{};
			std::memcpy(&ev, base + off, sizeof(ev));
			++eventTail_;
			switch (ev.type) {
			case proto::kEvHitActor: {
				const float scaled = ScaleMcToFnv(ev.a, 10);
				(void)scaled;
				// ApplyDamage(ev.formId, scaled, ev.flags, ev.weapon);
				break;
			}
			case proto::kEvPlayerDied:
				// Kill FNV player / trigger death camera.
				break;
			case proto::kEvExplosion:
				// Dig::OnExplosion(ev.a, ev.b, ev.c, ev.d);
				break;
			case proto::kEvSkillUse:
				// Skills::Advance from MC formId/uses.
				break;
			case proto::kEvArrowStuck:
				break;
			default:
				break;
			}
		}
		*reinterpret_cast<std::uint64_t*>(base + proto::kOffEventRing + 0x40) = eventTail_;
	}
}
