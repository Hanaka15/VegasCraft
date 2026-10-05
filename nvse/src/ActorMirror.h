#pragma once

#include "PCH.h"
#include "Link.h"

namespace vegascraft
{
	// Phase 3: mirror nearby FNV actors into the shared ActorTable for MC ActorProxies.
	class ActorMirror
	{
	public:
		void Tick(Link& link);
		void Clear(Link& link);

	private:
		std::uint32_t seq_{ 0 };
	};
}
