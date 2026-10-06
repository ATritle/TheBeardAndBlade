param([string]$Engine='C:/Program Files/Epic Games/UE_5.8',[string]$Destination='')
$projectRoot=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if(!$Destination) { $Destination=Join-Path $projectRoot 'Builds/v0.4.4' }
if(Test-Path (Join-Path $Destination 'Windows')) { throw 'Choose a fresh archive destination to avoid shipping leftover files from older builds.' }
$env:uebp_EngineSavedFolder=Join-Path $projectRoot 'Saved/Automation'
# Use a writable temporary directory for UBT response files, SDK validation and
# build-script locks. Default Windows TEMP was blocking unattended builds.
$packageTemp=Join-Path $projectRoot 'Intermediate/PackagingTemp'
New-Item -ItemType Directory -Force $packageTemp | Out-Null
$env:TEMP=$packageTemp
$env:TMP=$packageTemp
$sdkTools='C:/Program Files (x86)/Windows Kits/10/bin/10.0.22621.0/x64'
if(Test-Path (Join-Path $sdkTools 'mt.exe')) { $env:PATH="$sdkTools;$env:PATH" }
# Cook and UnrealPak must use the same Zen store, including child processes.
${env:UE-LocalDataCachePath}=Join-Path $projectRoot 'Intermediate/DDC'
# Build explicitly so UBA uses a writable project-local cache rather than ProgramData.
foreach($target in @(@('TheBeardAndBladeEditor','Development'),@('TheBeardAndBlade','Shipping'))) {
    & "$Engine/Engine/Binaries/ThirdParty/DotNet/10.0/win-x64/dotnet.exe" "$Engine/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.dll" $target[0] Win64 $target[1] "-Project=$projectRoot/TheBeardAndBlade.uproject" -WaitMutex -NoHotReloadFromIDE "-UBARootDir=$projectRoot/Intermediate/UBA"
    if($LASTEXITCODE -ne 0) { throw "Unreal compilation failed: $LASTEXITCODE" }
}
& "$Engine/Engine/Build/BatchFiles/RunUAT.bat" BuildCookRun "-project=$projectRoot/TheBeardAndBlade.uproject" -noP4 -platform=Win64 -clientconfig=Shipping -skipbuildeditor -nocompileeditor -nocompile -cook -stage -pak -iostore -archive "-archivedirectory=$Destination" "-AdditionalCookerOptions=-DDC=InstalledNoZenLocalFallback -DDC-ForceMemoryCache -LocalDataCachePath=$projectRoot/Intermediate/DDC -ShaderWorkingDir=$projectRoot/Intermediate/Shaders" -prereqs -nodebuginfo -utf8output -unattended
if($LASTEXITCODE -ne 0) { throw "Unreal packaging failed: $LASTEXITCODE" }
# Enemy textures load by generated names, so the cooker cannot discover them
# through map references. Fail packaging if any imported animation is omitted.
$enemySourceRoot=Join-Path $projectRoot 'Content/Art/EnemyExpansion'
$enemyCookRoot=Join-Path $projectRoot 'Saved/Cooked/Windows/TheBeardAndBlade/Content/Art/EnemyExpansion'
$missingEnemyAssets=@(Get-ChildItem -LiteralPath $enemySourceRoot -Recurse -Filter '*.uasset' | Where-Object {
    $relativeEnemyAsset=$_.FullName.Substring($enemySourceRoot.Length+1)
    !(Test-Path -LiteralPath (Join-Path $enemyCookRoot $relativeEnemyAsset))
})
if($missingEnemyAssets.Count) {
    throw ("Cook omitted {0} enemy assets, including {1}" -f $missingEnemyAssets.Count,$missingEnemyAssets[0].FullName)
}
Write-Output 'Enemy animation cook coverage verified.'
# Also inspect the final IoStore container, not just loose cooker output.
$enemyContainer=Join-Path $Destination 'Windows/TheBeardAndBlade/Content/Paks/TheBeardAndBlade-Windows.utoc'
$enemyContainerList=Join-Path $Destination 'enemy-container-audit.csv'
& "$Engine/Engine/Binaries/Win64/UnrealPak.exe" "-ListContainer=$enemyContainer" "-Csv=$enemyContainerList"
if($LASTEXITCODE -ne 0) { throw 'Cannot inspect packaged enemy assets.' }
$enemyContainerContents=Get-Content -Raw -LiteralPath $enemyContainerList
$missingPackagedEnemies=@(Get-ChildItem -LiteralPath $enemySourceRoot -Recurse -Filter '*.uasset' | Where-Object {
    $relativeEnemyAsset=$_.FullName.Substring($enemySourceRoot.Length+1).Replace('\','/')
    !$enemyContainerContents.Contains('/Content/Art/EnemyExpansion/'+$relativeEnemyAsset)
})
if($missingPackagedEnemies.Count) {
    throw ("Final container omitted {0} enemy assets, including {1}" -f $missingPackagedEnemies.Count,$missingPackagedEnemies[0].FullName)
}
Write-Output 'Final packaged enemy animation coverage verified.'
$groundedNames=@(Get-ChildItem -LiteralPath (Join-Path $projectRoot 'Content/Art/V2') -Filter 'Grounded*.uasset' | ForEach-Object { $_.BaseName })
if($groundedNames.Count -ne 1072) { throw 'Grounded hero import is incomplete.' }
foreach($groundedName in $groundedNames) {
    if(!$enemyContainerContents.Contains('/Content/Art/V2/'+$groundedName+'.uasset')) { throw "Final container omitted $groundedName" }
}
Write-Output 'All 1072 grounded hero animation layers verified in the final container.'
$bowNames=Get-Content -Raw (Join-Path $projectRoot 'ArtSource/BowsV2/manifest.json') | ConvertFrom-Json
if($bowNames.Count -ne 126) { throw 'Bow artwork manifest is incomplete.' }
foreach($bowName in $bowNames) {
    if(!$enemyContainerContents.Contains('/Content/Art/V2/'+$bowName+'.uasset')) { throw "Final container omitted $bowName" }
}
Write-Output 'All 126 bow animation, loot and effect textures verified in the final container.'
$fullBodyNames=Get-Content -Raw (Join-Path $projectRoot 'ArtSource/HeroFullBodyV1/runtime-test/manifest.json') | ConvertFrom-Json
if($fullBodyNames.Count -ne 152) { throw 'Full-body adventurer manifest is incomplete.' }
foreach($heroName in @($fullBodyNames)+@('MeleeIdle_SW','MeleeBlock_SW')) {
    if(!$enemyContainerContents.Contains('/Content/Art/V2/'+$heroName+'.uasset')) { throw "Final container omitted hero animation $heroName" }
}
Write-Output 'All 152 full-body atlases and both one-handed corrections verified in the final container.'
foreach($arrowFX in @('NS_ArrowWisp','M_ArrowWisp')) {
    if(!$enemyContainerContents.Contains('/Content/Effects/Arrows/'+$arrowFX+'.uasset')) { throw "Final container omitted Niagara arrow effect $arrowFX" }
}
# These assets also load by name and must survive staging into the final container.
foreach($requiredArt in @('/Content/Art/V2/Combat_Combo.uasset','/Content/Audio/CoinPickup.uasset','/Content/Art/TeaSpirit/TeaSpirit_DrinkSheet.uasset','/Content/Art/V2/Potion_DrinkSheet.uasset','/Content/Art/UI/Hotbar/Hotbar_TeaSpirit.uasset','/Content/Art/V2/M_DeathSpirit.uasset')) {
    if(!$enemyContainerContents.Contains($requiredArt)) { throw "Final container omitted $requiredArt" }
}
Write-Output 'Tea Spirit, HUD and death-spirit material cook coverage verified.'
$brandOutput=Join-Path $Destination 'Windows/Branding'
New-Item -ItemType Directory -Force $brandOutput | Out-Null
Copy-Item -LiteralPath (Join-Path $projectRoot 'Branding/Icon.png'),(Join-Path $projectRoot 'Branding/Logo.png'),(Join-Path $projectRoot 'Branding/PROMPTS.md') -Destination $brandOutput
Write-Output "Package ready in $Destination. Distribute the whole Windows folder, not only the EXE."
