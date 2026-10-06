@echo off
setlocal EnableExtensions
cd /d "%~dp0"

rem One Proton/wineserver session.
rem Start Prism minimized (MC window is already hidden by the mod), then Fallout
rem with /wait so exclusive fullscreen can take the display last (low input lag).
rem Do not use a second Linux "proton run" - that blocks on wineserver -w.

set "PRISM=%LOCALAPPDATA%\VegasCraft\Prism\prismlauncher.exe"
if exist "%PRISM%" (
	echo VegasCraft_boot: starting Prism minimized
	start "" /min "%PRISM%" --launch VegasCraft
) else (
	echo VegasCraft_boot: Prism missing at %PRISM%
)

echo VegasCraft_boot: starting FalloutNV.exe
start "" /wait "FalloutNV.exe"
set RC=%ERRORLEVEL%
echo VegasCraft_boot: Fallout exited %RC%

taskkill /F /IM javaw.exe >nul 2>&1
taskkill /F /IM prismlauncher.exe >nul 2>&1
exit /b %RC%
