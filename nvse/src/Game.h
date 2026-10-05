#pragma once

#include "ActorMirror.h"
#include "CameraDriver.h"
#include "Combat.h"
#include "Compositor.h"
#include "Dig.h"
#include "InputBridge.h"
#include "Launcher.h"
#include "Link.h"
#include "NpcBlocks.h"
#include "PlayerPuppet.h"
#include "Skills.h"
#include "WorldContext.h"
#include "WorldExporter.h"

namespace vegascraft
{
	class Game
	{
	public:
		static Game& Get();

		bool Init();
		void Shutdown();
		void OnFrame();
		void OnPresent(void* d3d9Device);
		void OnNewGameOrLoad();

		Link& GetLink() { return link_; }
		WorldExporter& Exporter() { return exporter_; }
		WorldContext& Context() { return context_; }

	private:
		Link link_;
		PlayerPuppet puppet_;
		WorldExporter exporter_;
		CameraDriver camera_;
		InputBridge input_;
		ActorMirror actors_;
		Combat combat_;
		Compositor compositor_;
		NpcBlocks npcBlocks_;
		WorldContext context_;
		Dig dig_;
		Skills skills_;

		bool inGame_{ false };
		double debugFnvX_{ 0 }, debugFnvY_{ 0 }, debugFnvZ_{ 128 };
	};
}
