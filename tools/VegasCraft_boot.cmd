@echo off
setlocal EnableExtensions
cd /d "%~dp0"

rem One Proton/wineserver session: Prism then Fallout.
rem Do not use a second Linux "proton run" - that blocks on wineserver -w.
rem Use borderless windowed (bFull Screen=0) so FNV stays visible with Prism/Java.

set "PRISM=%LOCALAPPDATA%\VegasCraft\Prism\prismlauncher.exe"
if exist "%PRISM%" (
	echo VegasCraft_boot: starting Prism
	start "" "%PRISM%" --launch VegasCraft
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
