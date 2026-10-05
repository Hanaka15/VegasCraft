# Starts a second VegasCraft Minecraft client on this PC, as a multiplayer test guest.
#
# It runs from fabric/run-guest (its config/vegascraft.properties says join=localhost:25599) as the
# offline user "Guest", on its own link (Local\VegasCraft_guest) served by tools/fake_guest.py. The
# host must be running with VEGASCRAFT_LAN_PORT=25599 and VEGASCRAFT_LAN_OFFLINE=1 and be in its world.
# Uses the host's dev launch files (build the mod and run runClient once first).
param([string]$Name = "Guest", [string]$Link = "Local\VegasCraft_guest", [string]$RunDir = "run-guest", [string]$Log = "$env:TEMP\vegascraft_guest.log")

$root = Split-Path -Parent $PSScriptRoot
$fabric = Join-Path $root "fabric"
$java = Join-Path $env:JAVA_HOME "bin\java.exe"
if (-not (Test-Path $java)) { throw "Set JAVA_HOME to a JDK 25 first." }
$runDir = Join-Path $fabric $RunDir
New-Item -ItemType Directory -Force (Join-Path $runDir "config") | Out-Null
$config = Join-Path $runDir "config\vegascraft.properties"
if (-not (Test-Path $config)) { Set-Content -Path $config -Value "join=localhost:25599" -Encoding ascii }

$javaArgs = @(
    "-Dfabric.dli.config=$fabric\.gradle\loom-cache\launch.cfg",
    "-Dfabric.dli.env=client",
    "-Dfabric.dli.main=net.fabricmc.loader.impl.launch.knot.KnotClient",
    "@$fabric\build\loom-cache\argFiles\runClient",
    "--enable-native-access=ALL-UNNAMED",
    "-XX:StackShadowPages=32",
    "--sun-misc-unsafe-memory-access=allow",
    "-Dvegascraft.link=$Link",
    "net.fabricmc.devlaunchinjector.Main",
    "--username", $Name
)
$p = Start-Process -FilePath $java -ArgumentList $javaArgs -WorkingDirectory $runDir -RedirectStandardOutput $Log -RedirectStandardError "$Log.err" -PassThru -WindowStyle Minimized
"guest Minecraft started (pid $($p.Id)); log: $Log"
