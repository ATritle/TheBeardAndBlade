@echo off
start "Rustblade Squire review" "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "%~dp0TheBeardAndBlade.uproject" -game -RustbladeTest -windowed -ResX=1280 -ResY=800 -DDC=InstalledNoZenLocalFallback -DDC-ForceMemoryCache
