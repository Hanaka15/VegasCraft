#include "Game.h"
#include "Controls.h"
#include "Coords.h"

#include <algorithm>
#include <cmath>

namespace vegascraft
{
	Game& Game::Get()
	{
		static Game g;
		return g;
	}

	bool Game::Init()
	{
		if (!link_.IsOpen()) {
			if (!link_.Create()) {
				Launcher::Logf("shared-memory Create failed");
				return false;
			}
			Launcher::Logf("shared-memory link created");
		}
		Launcher::StartMinecraft();
		return true;
	}

	void Game::Shutdown()
	{
		Controls::SetMinecraftOwnsPlayer(false);
		Launcher::StopMinecraft();
		compositor_.Shutdown();
		link_.Close();
	}

	void Game::OnNewGameOrLoad()
	{
		inGame_ = true;
		lookInitialized_ = false;
		Controls::SetMinecraftOwnsPlayer(false);  // re-apply after load clears AltEx flags
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
		const bool mcInWorld = haveMc && (mc.flags & proto::kMcInWorld) != 0;
		const bool mcScreen = haveMc && (mc.flags & proto::kMcScreenOpen) != 0;

		// Puppet when MC is in the mirror world (SkyCraft: minecraftOwnsPlayer).
		const bool puppet = inGame_ && mcInWorld;
		Controls::SetMinecraftOwnsPlayer(puppet);

		if (haveMc) {
			input_.SetMode(mcScreen ? InputBridge::Mode::McScreen : InputBridge::Mode::Gameplay);
		}
		input_.Flush(link_);

		// Host-owned look (SkyCraft): integrate mouse here, push to HostState; MC copies it.
		float dx = 0.f, dy = 0.f;
		input_.ConsumeLook(dx, dy);
		if (puppet && !lookInitialized_) {
			lookYaw_ = haveMc ? mc.yaw : 0.f;
			lookPitch_ = haveMc ? mc.pitch : 0.f;
			lookInitialized_ = true;
		}
		if (puppet && !mcScreen) {
			const float s = 0.5f * 0.6f + 0.2f;
			const float factor = s * s * s * 8.0f * 0.15f;
			lookYaw_ = std::fmod(lookYaw_ + dx * factor, 360.0f);
			if (lookYaw_ < 0.f) {
				lookYaw_ += 360.0f;
			}
			lookPitch_ = std::clamp(lookPitch_ + dy * factor, -90.0f, 90.0f);
			camera_.UpdateLook(lookYaw_, lookPitch_);
		}

		proto::HostState host{};
		host.flags = inGame_ ? proto::kHostInGame : 0;
		host.worldId = context_.WorldId();
		host.collisionEpoch = exporter_.Epoch();
		if (mcInWorld) {
			host.posX = mc.x;
			host.posY = mc.y;
			host.posZ = mc.z;
		} else {
			const auto mcPos = coords::FnvToMc(debugFnvX_, debugFnvY_, debugFnvZ_);
			host.posX = mcPos.x;
			host.posY = mcPos.y;
			host.posZ = mcPos.z;
		}
		host.yaw = lookInitialized_ ? lookYaw_ : (haveMc ? mc.yaw : 0.f);
		host.pitch = lookInitialized_ ? lookPitch_ : (haveMc ? mc.pitch : 0.f);

		std::uint32_t vw = static_cast<std::uint32_t>(::GetSystemMetrics(SM_CXSCREEN));
		std::uint32_t vh = static_cast<std::uint32_t>(::GetSystemMetrics(SM_CYSCREEN));
		if (vw < 640 || vh < 480) {
			vw = 1920;
			vh = 1080;
		}
		host.viewportW = vw;
		host.viewportH = vh;
		host.gameHour = 12.0f;
		link_.WriteHostState(host);

		puppet_.Update(link_);

		if (haveMc && (mc.flags & proto::kMcSneaking)) {
			skills_.OnMcSneakTick();
		}

		exporter_.Tick(link_, debugFnvX_, debugFnvY_, debugFnvZ_, context_.WorldId());
		actors_.Tick(link_);
		combat_.DrainMcEvents(link_);
		context_.Tick(link_);
		npcBlocks_.DrainRenderSolids(link_);
	}

	void Game::OnPresent(bool isLoadingScreen)
	{
		compositor_.OnPresent(link_, isLoadingScreen);
	}
}
