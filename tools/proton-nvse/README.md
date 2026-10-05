# Proton NVSE helpers

Proton does not always inject `nvse_steam_loader.dll` the way Windows Steam does.

`dinput8.dll` is [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) (x86).
The launch script copies it next to `FalloutNV.exe` and installs
`nvse_steam_loader.asi` (a copy of `nvse_steam_loader.dll`) so NVSE loads when
the game imports `DINPUT8.dll`.
