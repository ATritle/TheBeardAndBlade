@echo off
set "atlasEngine=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
if not exist "%atlasEngine%" (
  echo Unreal Engine 5.8 is required for this local playtest.
  pause
  exit /b 1
)
start "The Beard and Blade - Emerald Atlas playtest" "%atlasEngine%" "%~dp0TheBeardAndBlade.uproject" -game -windowed -ResX=1280 -ResY=800 -DDC=InstalledNoZenLocalFallback -DDC-ForceMemoryCache
