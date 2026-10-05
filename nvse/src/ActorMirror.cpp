#include "ActorMirror.h"

namespace vegascraft
{
	void ActorMirror::Clear(Link& link)
	{
		if (!link.IsOpen()) {
			return;
		}
		auto* table = link.At<proto::ActorTable>(proto::kOffActorTable);
		++seq_;
		table->seq = seq_ * 2u - 1u;
		table->count = 0;
		table->seq = seq_ * 2u;
	}

	void ActorMirror::Tick(Link& link)
	{
		if (!link.IsOpen()) {
			return;
		}
		// Hook point: iterate ProcessManager high-process actors within ~64 blocks, fill ActorRecord.
		auto* table = link.At<proto::ActorTable>(proto::kOffActorTable);
		++seq_;
		table->seq = seq_ * 2u - 1u;
		// Leave count as-is until Game hooks populate; ensure seqlock closes.
		table->seq = seq_ * 2u;
	}
}
