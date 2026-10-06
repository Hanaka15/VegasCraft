@echo off
setlocal EnableExtensions
cd /d "%~dp0"

rem Wayland/XWayland cannot show D3D9 exclusive fullscreen (1x1 HWND, audio only).
rem Wine virtual desktop gives exclusive FS a visible 1920x1080 surface — not borderless.

echo VegasCraft_boot: Wine virtual desktop 1920x1080 (exclusive FS)
start "" /wait explorer.exe /desktop=VegasCraft,1920x1080 "%~dp0VegasCraft_inner.cmd"
set RC=%ERRORLEVEL%
exit /b %RC%
