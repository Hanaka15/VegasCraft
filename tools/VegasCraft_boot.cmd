@echo off
setlocal
cd /d "%~dp0"

rem One Proton/wineserver session: Minecraft first, then Fallout (keeps FNV focus).
rem Do not use a second Linux "proton run" — that blocks on wineserver -w.

set "PRISM=%LOCALAPPDATA%\VegasCraft\Prism\prismlauncher.exe"
if exist "%PRISM%" (
	echo VegasCraft_boot: starting Prism
	start "" "%PRISM%" --launch VegasCraft
	rem Give javaw time to come up before Fallout grabs the session.
	timeout /t 18 /nobreak >nul
) else (
	echo VegasCraft_boot: Prism missing at %PRISM%
)

echo VegasCraft_boot: starting FalloutNV.exe
start "" /wait "FalloutNV.exe"
set RC=%ERRORLEVEL%
echo VegasCraft_boot: Fallout exited %RC%

rem Best-effort: close Prism/Minecraft when FNV quits.
taskkill /F /IM javaw.exe >nul 2>&1
taskkill /F /IM prismlauncher.exe >nul 2>&1
exit /b %RC%
