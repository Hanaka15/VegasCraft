package dev.vegascraft;

/**
 * Wine/Proton (Wine &lt; 9.3, including Proton 9.0) crashes inside
 * {@code java.net.NetworkInterface.getAll()} with {@code 0xc06d007f}.
 * Netty hits that when the integrated server binds its memory channel.
 * A fixed machine/process id makes Netty skip the native NIC enumeration.
 */
public final class ProtonWorkarounds {
	private static boolean applied;

	private ProtonWorkarounds() {}

	public static void apply() {
		if (applied) {
			return;
		}
		applied = true;
		setIfAbsent("io.netty.machineId", "02:00:00:00:00:01");
		setIfAbsent("io.netty.processId", "1");
	}

	private static void setIfAbsent(String key, String value) {
		if (System.getProperty(key) == null || System.getProperty(key).isBlank()) {
			System.setProperty(key, value);
		}
	}
}
