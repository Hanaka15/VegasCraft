@echo off
rem Launches Skyrim through MO2 + SKSE using the currently selected MO2 profile.
rem Set VEGASCRAFT_MO2 to your Mod Organizer folder (the one with ModOrganizer.exe).
if "%VEGASCRAFT_MO2%"=="" (
    echo Set VEGASCRAFT_MO2 to your Mod Organizer folder first.
    exit /b 1
)
start "" "%VEGASCRAFT_MO2%\ModOrganizer.exe" "moshortcut://Skyrim Special Edition:SKSE"
