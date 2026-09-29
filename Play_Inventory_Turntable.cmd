@echo off
cd /d "%~dp0"
echo Begin Descent, press I, then drag left or right over the adventurer.
start "" "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "%~dp0TheBeardAndBlade.uproject" -game -windowed -ResX=1280 -ResY=800 -DDC=InstalledNoZenLocalFallback -DDC-ForceMemoryCache
