@echo off
cd /d "%~dp0"
echo 1 Graveglass Slinger
echo 2 Chainbound Bailiff
echo 3 Candle Hexer
echo 4 Sepulcher Lancer
choice /c 1234 /m "Choose eight-direction animation review"
set /a species=49+%errorlevel%
start "" "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "%~dp0TheBeardAndBlade.uproject" -game -ExpansionReview -ExpansionSpecies=%species% -windowed -ResX=1600 -ResY=1000 -DDC=InstalledNoZenLocalFallback -DDC-ForceMemoryCache
