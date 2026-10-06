@echo off
setlocal EnableExtensions
cd /d "%~dp0"

rem Windowed. Do NOT use tasklist (broken under Wine).
rem Start Prism/Minecraft first, wait (Wine-safe ping), then run FalloutNV.exe
rem in THIS console so /wait is reliable. `start /wait` often returns immediately
rem under Proton and then we taskkill javaw — looks like "Minecraft not loading".

set "PRISM=%LOCALAPPDATA%\VegasCraft\Prism\prismlauncher.exe"

if not exist "%PRISM%" goto no_prism
echo VegasCraft_boot: starting Prism minimized
start "" /min "%PRISM%" --launch VegasCraft
echo VegasCraft_boot: waiting for Minecraft (javaw)...
rem ping -n N waits ~N-1 seconds under Wine
ping 127.0.0.1 -n 21 >nul
goto start_fnv

:no_prism
echo VegasCraft_boot: Prism missing at %PRISM%

:start_fnv
echo VegasCraft_boot: starting FalloutNV windowed (blocking)
FalloutNV.exe
set RC=%ERRORLEVEL%
echo VegasCraft_boot: Fallout exited %RC%

taskkill /F /IM javaw.exe >nul 2>&1
taskkill /F /IM prismlauncher.exe >nul 2>&1
exit /b %RC%
