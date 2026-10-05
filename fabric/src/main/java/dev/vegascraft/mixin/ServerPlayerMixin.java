package dev.vegascraft.mixin;

import dev.vegascraft.VegasCraft;
import dev.vegascraft.combat.SkyCombat;
import dev.vegascraft.combat.FNVActorEntity;
import dev.vegascraft.link.Proto;
import dev.vegascraft.link.VegasLink;
import net.minecraft.world.damagesource.DamageSource;
import net.minecraft.world.entity.Entity;
import net.minecraft.server.level.ServerPlayer;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(ServerPlayer.class)
public abstract class ServerPlayerMixin {
	/** Critical hits on a Skyrim actor are flagged so Skyrim can play them up. */
	@Inject(method = "crit", at = @At("HEAD"))
	private void vegascraft$critSkyrim(Entity entity, CallbackInfo ci) {
		if (entity instanceof FNVActorEntity proxy) {
			proxy.markCritical();
		}
	}

	/** Dying in Minecraft is dying in Skyrim: the host's through the link, a guest's through theirs. */
	@Inject(method = "die", at = @At("HEAD"))
	private void vegascraft$diesInSkyrim(DamageSource source, CallbackInfo ci) {
		ServerPlayer self = (ServerPlayer) (Object) this;
		int attacker = SkyCombat.attackerFormId(source);
		if (!dev.vegascraft.net.SkyNet.isHost(self)) {
			if (net.fabricmc.fabric.api.networking.v1.ServerPlayNetworking.canSend(self, dev.vegascraft.net.SkyNet.Died.TYPE)) {
				net.fabricmc.fabric.api.networking.v1.ServerPlayNetworking.send(self, new dev.vegascraft.net.SkyNet.Died(attacker));
			}
			VegasCraft.LOG.info("VegasCraft: guest {} died ({}); telling their Skyrim", self.getPlainTextName(), source.getMsgId());
			return;
		}
		if (VegasLink.active()) {
			VegasLink.pushEvent(Proto.EV_PLAYER_DIED, attacker, 0, 0, 0, 0, 0);
			VegasCraft.LOG.info("VegasCraft: player died ({}); telling Skyrim", source.getMsgId());
		}
	}
}
