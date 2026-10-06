@echo off
setlocal EnableExtensions
cd /d "%~dp0"

rem Exclusive fullscreen: start Prism/MC first (hidden), wait for javaw, THEN Fallout
rem so FNV takes exclusive mode last. No borderless.
rem Labels must stay outside IF blocks (Wine cmd).

set "PRISM=%LOCALAPPDATA%\VegasCraft\Prism\prismlauncher.exe"
if not exist "%PRISM%" goto no_prism

echo VegasCraft_boot: starting Prism minimized
start "" /min "%PRISM%" --launch VegasCraft

set TRIES=0
:wait_javaw
ping -n 2 127.0.0.1 >nul
set /a TRIES+=1
tasklist /FI "IMAGENAME eq javaw.exe" 2>NUL | find /I "javaw.exe" >NUL
if not errorlevel 1 goto javaw_ready
if %TRIES% GEQ 40 goto javaw_ready
goto wait_javaw

:javaw_ready
echo VegasCraft_boot: Minecraft JVM up - starting FalloutNV exclusive
goto start_fnv

:no_prism
echo VegasCraft_boot: Prism missing at %PRISM%

:start_fnv
echo VegasCraft_boot: starting FalloutNV.exe
start "" /wait "FalloutNV.exe"
set RC=%ERRORLEVEL%
echo VegasCraft_boot: Fallout exited %RC%

taskkill /F /IM javaw.exe >nul 2>&1
taskkill /F /IM prismlauncher.exe >nul 2>&1
exit /b %RC%
