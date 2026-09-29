@echo off
start "Rustblade animation board" "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "%~dp0TheBeardAndBlade.uproject" -game -RustbladeReview -windowed -ResX=1600 -ResY=1000 -DDC=InstalledNoZenLocalFallback -DDC-ForceMemoryCache
