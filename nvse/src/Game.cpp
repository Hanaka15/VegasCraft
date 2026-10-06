#include "Game.h"
#include "Controls.h"
#include "Coords.h"
#include "FnvPlayer.h"

#include <algorithm>
#include <cmath>

namespace vegascraft
{
	namespace
	{
		// FNV units — bigger jumps mean the engine moved the player (load, door, fast travel).
		constexpr double kFnvTeleportThreshold = 300.0;
	}

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
		puppet_.SetEnabled(false);
		Launcher::StopMinecraft();
		compositor_.Shutdown();
		link_.Close();
	}

	void Game::OnNewGameOrLoad()
	{
		inGame_ = true;
		lookInitialized_ = false;
		teleportPending_ = true;
		haveLastPuppetFnv_ = false;
		Controls::SetMinecraftOwnsPlayer(false);  // re-apply after load clears AltEx flags
		exporter_.BumpEpoch();
		context_.OnCellChange(0x000DA726 /* WastelandNV placeholder */, false);
		Launcher::Logf("OnNewGameOrLoad: teleport pending (epoch=%u)", exporter_.Epoch());
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

		double fnvX = lastFnvX_, fnvY = lastFnvY_, fnvZ = lastFnvZ_;
		float fnvPitchDeg = 0.f, fnvYawDeg = 0.f;
		const bool haveFnv = FnvPlayer::TryRead(fnvX, fnvY, fnvZ, fnvPitchDeg, fnvYawDeg);
		if (haveFnv) {
			lastFnvX_ = fnvX;
			lastFnvY_ = fnvY;
			lastFnvZ_ = fnvZ;
			haveLastFnv_ = true;
			// Engine moved the player far from where we last puppeted → resync MC.
			if (haveLastPuppetFnv_) {
				const double dx = fnvX - lastPuppetFnvX_;
				const double dy = fnvY - lastPuppetFnvY_;
				const double dz = fnvZ - lastPuppetFnvZ_;
				if (dx * dx + dy * dy + dz * dz > kFnvTeleportThreshold * kFnvTeleportThreshold) {
					teleportPending_ = true;
					Launcher::Logf("FNV moved player (%.0f units); teleport pending", std::sqrt(dx * dx + dy * dy + dz * dz));
				}
			}
		}

		const auto fnvMc = coords::FnvToMc(
			haveLastFnv_ ? lastFnvX_ : 0.0,
			haveLastFnv_ ? lastFnvY_ : 0.0,
			haveLastFnv_ ? lastFnvZ_ : 128.0);

		if (teleportPending_ && inGame_ && haveLastFnv_) {
			++teleportSeq_;
			teleportPending_ = false;
			lookYaw_ = coords::FnvYawToMc(fnvYawDeg);
			lookPitch_ = coords::FnvPitchToMc(fnvPitchDeg);
			lookInitialized_ = true;
			Launcher::Logf(
				"teleportSeq=%u → MC (%.1f, %.1f, %.1f) from FNV (%.0f, %.0f, %.0f)",
				teleportSeq_, fnvMc.x, fnvMc.y, fnvMc.z, lastFnvX_, lastFnvY_, lastFnvZ_);
		}

		const bool arriving = haveMc && mcInWorld && mc.teleportAck != teleportSeq_;
		const bool puppet = inGame_ && mcInWorld && haveMc && mc.teleportAck == teleportSeq_;
		Controls::SetMinecraftOwnsPlayer(puppet || arriving);
		puppet_.SetEnabled(puppet);

		if (haveMc) {
			input_.SetMode(mcScreen ? InputBridge::Mode::McScreen : InputBridge::Mode::Gameplay);
		}
		input_.Flush(link_);

		float dx = 0.f, dy = 0.f;
		input_.ConsumeLook(dx, dy);
		if ((puppet || arriving) && !lookInitialized_ && haveLastFnv_) {
			lookYaw_ = coords::FnvYawToMc(fnvYawDeg);
			lookPitch_ = coords::FnvPitchToMc(fnvPitchDeg);
			lookInitialized_ = true;
		}
		if ((puppet || arriving) && !mcScreen) {
			const float s = 0.5f * 0.6f + 0.2f;
			const float factor = s * s * s * 8.0f * 0.15f;
			lookYaw_ = std::fmod(lookYaw_ + dx * factor, 360.0f);
			if (lookYaw_ < 0.f) {
				lookYaw_ += 360.0f;
			}
			// Wine/Proton GetCursorPos Y is opposite Raw Input mouseInputY used by SkyCraft.
			lookPitch_ = std::clamp(lookPitch_ - dy * factor, -90.0f, 90.0f);
			camera_.UpdateLook(lookYaw_, lookPitch_);
		}

		// HostState.pos is always the FNV player's feet (SkyCraft). Never echo MC here —
		// that was teleporting MC to (0,0,0) on first link.
		proto::HostState host{};
		host.flags = inGame_ ? proto::kHostInGame : 0;
		host.worldId = context_.WorldId();
		host.collisionEpoch = exporter_.Epoch();
		host.posX = fnvMc.x;
		host.posY = fnvMc.y;
		host.posZ = fnvMc.z;
		host.yaw = lookInitialized_ ? lookYaw_ : 0.f;
		host.pitch = lookInitialized_ ? lookPitch_ : 0.f;
		host.teleportSeq = teleportSeq_;

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
		if (puppet && haveMc) {
			const auto pup = coords::McToFnv(mc.x, mc.y, mc.z);
			lastPuppetFnvX_ = pup.x;
			lastPuppetFnvY_ = pup.y;
			lastPuppetFnvZ_ = pup.z;
			haveLastPuppetFnv_ = true;
		}

		if (haveMc && (mc.flags & proto::kMcSneaking)) {
			skills_.OnMcSneakTick();
		}

		// Collision around MC feet while puppeted so the floor follows the walk; else FNV feet.
		double exportX = lastFnvX_, exportY = lastFnvY_, exportZ = lastFnvZ_;
		if (puppet && haveMc) {
			const auto feet = coords::McToFnv(mc.x, mc.y, mc.z);
			exportX = feet.x;
			exportY = feet.y;
			exportZ = feet.z;
		} else if (!haveLastFnv_) {
			exportX = 0;
			exportY = 0;
			exportZ = 128;
		}
		exporter_.Tick(link_, exportX, exportY, exportZ, context_.WorldId());
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
