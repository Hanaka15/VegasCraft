@echo off
setlocal EnableExtensions
cd /d "%~dp0"

rem Windowed. Do NOT use tasklist (broken under Wine) — it never saw FalloutNV
rem and Prism never launched. Start Prism, then Fallout with /wait.

set "PRISM=%LOCALAPPDATA%\VegasCraft\Prism\prismlauncher.exe"

if not exist "%PRISM%" goto no_prism
echo VegasCraft_boot: starting Prism minimized
start "" /min "%PRISM%" --launch VegasCraft
goto start_fnv

:no_prism
echo VegasCraft_boot: Prism missing at %PRISM%

:start_fnv
echo VegasCraft_boot: starting FalloutNV windowed
start "" /wait "FalloutNV.exe"
set RC=%ERRORLEVEL%
echo VegasCraft_boot: Fallout exited %RC%

taskkill /F /IM javaw.exe >nul 2>&1
taskkill /F /IM prismlauncher.exe >nul 2>&1
exit /b %RC%
