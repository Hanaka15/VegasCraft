#include "Game.h"
#include "Coords.h"

namespace vegascraft
{
	Game& Game::Get()
	{
		static Game g;
		return g;
	}

	bool Game::Init()
	{
		// Always kick Prism first — shared-memory setup must not block launch.
		Launcher::StartMinecraft();
		if (!link_.IsOpen()) {
			if (!link_.Create()) {
				Launcher::Logf("shared-memory Create failed");
				return false;
			}
			Launcher::Logf("shared-memory link created");
		}
		return true;
	}

	void Game::Shutdown()
	{
		Launcher::StopMinecraft();
		compositor_.Shutdown();
		link_.Close();
	}

	void Game::OnNewGameOrLoad()
	{
		inGame_ = true;
		exporter_.BumpEpoch();
		context_.OnCellChange(0x000DA726 /* WastelandNV placeholder */, false);
	}

	void Game::OnFrame()
	{
		if (!link_.IsOpen()) {
			return;
		}
		link_.Heartbeat();

		proto::McState mc{};
		const bool haveMc = link_.ReadMcState(mc);

		proto::HostState host{};
		host.flags = inGame_ ? proto::kHostInGame : 0;
		host.worldId = context_.WorldId();
		host.collisionEpoch = exporter_.Epoch();
		if (haveMc && (mc.flags & proto::kMcInWorld)) {
			host.posX = mc.x;
			host.posY = mc.y;
			host.posZ = mc.z;
			host.yaw = mc.yaw;
			host.pitch = mc.pitch;
		} else {
			const auto mcPos = coords::FnvToMc(debugFnvX_, debugFnvY_, debugFnvZ_);
			host.posX = mcPos.x;
			host.posY = mcPos.y;
			host.posZ = mcPos.z;
		}
		host.viewportW = 1280;
		host.viewportH = 720;
		host.gameHour = 12.0f;
		link_.WriteHostState(host);

		puppet_.Update(link_);
		if (haveMc) {
			camera_.Update(mc);
			if (mc.flags & proto::kMcScreenOpen) {
				input_.SetMode(InputBridge::Mode::McScreen);
			} else {
				input_.SetMode(InputBridge::Mode::Gameplay);
			}
			if (mc.flags & proto::kMcSneaking) {
				skills_.OnMcSneakTick();
			}
		}

		exporter_.Tick(link_, debugFnvX_, debugFnvY_, debugFnvZ_, context_.WorldId());
		actors_.Tick(link_);
		combat_.DrainMcEvents(link_);
		context_.Tick(link_);
		npcBlocks_.DrainRenderSolids(link_);
		input_.Flush(link_);
	}

	void Game::OnPresent(void* d3d9Device)
	{
		if (!compositor_.Init(d3d9Device)) {
			return;
		}
		compositor_.OnPresent(link_, d3d9Device);
	}
}
