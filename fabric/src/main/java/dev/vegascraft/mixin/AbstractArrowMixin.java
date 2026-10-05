package dev.vegascraft.mixin;

import com.llamalad7.mixinextras.injector.wrapoperation.Operation;
import com.llamalad7.mixinextras.injector.wrapoperation.WrapOperation;
import dev.vegascraft.combat.FNVActorEntity;
import dev.vegascraft.link.Proto;
import dev.vegascraft.link.VegasLink;
import dev.vegascraft.world.SkyClip;
import net.minecraft.util.Mth;
import net.minecraft.world.entity.LivingEntity;
import net.minecraft.world.entity.projectile.arrow.AbstractArrow;
import net.minecraft.world.entity.projectile.arrow.Arrow;
import net.minecraft.world.entity.projectile.arrow.SpectralArrow;
import net.minecraft.world.phys.EntityHitResult;
import net.minecraft.world.phys.Vec3;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import net.minecraft.world.level.ClipContext;
import net.minecraft.world.level.Level;
import net.minecraft.world.phys.BlockHitResult;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;

/**
 * Arrows and tridents hit Skyrim's exact surfaces. They then stick where they hit: the block state
 * there is air, the same as what they recorded on impact, so vanilla never makes them fall out.
 */
@Mixin(AbstractArrow.class)
public abstract class AbstractArrowMixin {
	@WrapOperation(
		method = "tick",
		at = @At(value = "INVOKE", target = "Lnet/minecraft/world/level/Level;clipIncludingBorder(Lnet/minecraft/world/level/ClipContext;)Lnet/minecraft/world/phys/BlockHitResult;")
	)
	private BlockHitResult vegascraft$hitSkyrim(Level level, ClipContext context, Operation<BlockHitResult> original) {
		return SkyClip.refine(context.getFrom(), context.getTo(), original.call(level, context), SkyClip.Use.PROJECTILE);
	}

	@Unique
	private Vec3 vegascraft$hitAt;

	@Inject(method = "onHitEntity", at = @At("HEAD"))
	private void vegascraft$rememberHit(EntityHitResult hitResult, CallbackInfo ci) {
		this.vegascraft$hitAt = hitResult.getLocation();
	}

	/**
	 * Where Minecraft counts an arrow as stuck in a creature (it hurt it and didn't pierce): if that
	 * creature is a Skyrim NPC's stand-in, Skyrim pins the arrow to the NPC's skeleton.
	 */
	@WrapOperation(method = "onHitEntity", at = @At(value = "INVOKE", target = "Lnet/minecraft/world/entity/LivingEntity;setArrowCount(I)V"))
	private void vegascraft$stickInSkyrimActor(LivingEntity mob, int count, Operation<Void> original) {
		original.call(mob, count);
		if (!(mob instanceof FNVActorEntity actor) || this.vegascraft$hitAt == null || !VegasLink.active()) {
			return;
		}
		AbstractArrow self = (AbstractArrow) (Object) this;
		Vec3 v = self.getDeltaMovement();
		float yaw = (float) (Mth.atan2(v.x, v.z) * Mth.RAD_TO_DEG);
		float pitch = (float) (Mth.atan2(v.y, v.horizontalDistance()) * Mth.RAD_TO_DEG);
		int texture = self instanceof SpectralArrow ? 2 : self instanceof Arrow tippable && tippable.getColor() > 0 ? 1 : 0;
		Vec3 at = this.vegascraft$hitAt;
		VegasLink.pushEvent(Proto.EV_ARROW_STUCK, actor.formId(), (float) at.x, (float) at.y, (float) at.z, yaw, Float.floatToRawIntBits(pitch), texture);
	}
}
