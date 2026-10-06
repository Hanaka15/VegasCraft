@echo off
setlocal EnableExtensions
cd /d "%~dp0"

rem One Proton/wineserver session: Minecraft first, then Fallout (keeps FNV focus).
rem Do not use a second Linux "proton run" - that blocks on wineserver -w.
rem
rem NOTE: timeout is unreliable under Wine/Proton (often returns immediately).
rem Use ping -n delays instead (~1s per echo). Labels must stay outside IF blocks.

set "PRISM=%LOCALAPPDATA%\VegasCraft\Prism\prismlauncher.exe"
if not exist "%PRISM%" goto no_prism

echo VegasCraft_boot: starting Prism
start "" "%PRISM%" --launch VegasCraft

rem Wait until javaw appears (Minecraft JVM), up to ~90s.
set TRIES=0

:wait_javaw
ping -n 2 127.0.0.1 >nul
set /a TRIES+=1
tasklist /FI "IMAGENAME eq javaw.exe" 2>NUL | find /I "javaw.exe" >NUL
if not errorlevel 1 goto javaw_up
if %TRIES% GEQ 45 goto javaw_giveup
goto wait_javaw

:javaw_giveup
echo VegasCraft_boot: javaw not seen after wait - starting Fallout anyway
goto after_wait

:javaw_up
echo VegasCraft_boot: javaw running - settle for mirror world load
rem ~20s more for Fabric + mirror world after JVM start
ping -n 21 127.0.0.1 >nul
goto after_wait

:no_prism
echo VegasCraft_boot: Prism missing at %PRISM%

:after_wait
echo VegasCraft_boot: starting FalloutNV.exe
start "" /wait "FalloutNV.exe"
set RC=%ERRORLEVEL%
echo VegasCraft_boot: Fallout exited %RC%

rem Best-effort: close Prism/Minecraft when FNV quits.
taskkill /F /IM javaw.exe >nul 2>&1
taskkill /F /IM prismlauncher.exe >nul 2>&1
exit /b %RC%
