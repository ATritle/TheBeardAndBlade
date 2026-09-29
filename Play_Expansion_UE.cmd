@echo off
cd /d "%~dp0"
echo 1 Graveglass Slinger
echo 2 Chainbound Bailiff
echo 3 Candle Hexer
echo 4 Sepulcher Lancer
echo 5 Mixed group
choice /c 12345 /m "Choose combat test"
set /a species=49+%errorlevel%
if %species%==54 set species=0
start "" "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "%~dp0TheBeardAndBlade.uproject" -game -ExpansionTest -ExpansionSpecies=%species% -windowed -ResX=1280 -ResY=800 -DDC=InstalledNoZenLocalFallback -DDC-ForceMemoryCache
