@echo off
setlocal EnableExtensions
cd /d "%~dp0"

rem Windowed mode. Fallout first, then Prism/MC in the same wineserver.

set "PRISM=%LOCALAPPDATA%\VegasCraft\Prism\prismlauncher.exe"

echo VegasCraft_boot: starting FalloutNV windowed
start "" "FalloutNV.exe"

set TRIES=0
:wait_fnv
ping -n 2 127.0.0.1 >nul
set /a TRIES+=1
tasklist /FI "IMAGENAME eq FalloutNV.exe" 2>NUL | find /I "FalloutNV.exe" >NUL
if not errorlevel 1 goto fnv_ready
if %TRIES% GEQ 60 goto fnv_ready
goto wait_fnv

:fnv_ready
if not exist "%PRISM%" goto no_prism
echo VegasCraft_boot: Fallout up - starting Prism minimized
start "" /min "%PRISM%" --launch VegasCraft
goto wait_exit

:no_prism
echo VegasCraft_boot: Prism missing at %PRISM%

:wait_exit
ping -n 3 127.0.0.1 >nul
tasklist /FI "IMAGENAME eq FalloutNV.exe" 2>NUL | find /I "FalloutNV.exe" >NUL
if not errorlevel 1 goto wait_exit

echo VegasCraft_boot: Fallout exited
taskkill /F /IM javaw.exe >nul 2>&1
taskkill /F /IM prismlauncher.exe >nul 2>&1
exit /b 0
