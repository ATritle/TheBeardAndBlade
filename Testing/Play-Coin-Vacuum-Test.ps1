param([string]$ArchivePath='', [string]$InstallRoot='', [switch]$NoLaunch)
$ErrorActionPreference='Stop'
$version='coin-vacuum-2026-10-05'
$expected='CF8FC6C6465F4245A08619DC25D184B8240A84634C73D38CCB02677A0546025D'
$url='https://github.com/ATritle/TheBeardAndBlade/releases/download/testing-coin-vacuum-2026-10-05/TheBeardAndBlade-CoinVacuum-Windows-2026-10-05.zip'
if(!$InstallRoot){$InstallRoot=Join-Path $env:LOCALAPPDATA "TheBeardAndBladeTests/$version"}
$InstallRoot=[IO.Path]::GetFullPath($InstallRoot)
New-Item -ItemType Directory -Force -Path $InstallRoot | Out-Null
$exe=Join-Path $InstallRoot 'Windows/TheBeardAndBlade.exe'
$marker=Join-Path $InstallRoot 'verified-build.txt'
if(!(Test-Path -LiteralPath $exe) -or !(Test-Path -LiteralPath $marker) -or (Get-Content -LiteralPath $marker -Raw).Trim() -ne $expected){
    if(!$ArchivePath){
        $ArchivePath=Join-Path $InstallRoot 'game.zip'
        if(!(Test-Path -LiteralPath $ArchivePath) -or (Get-FileHash -LiteralPath $ArchivePath -Algorithm SHA256).Hash -ne $expected){
            Write-Host 'Downloading the complete Windows game (about 1.13 GB). Unreal Editor is NOT needed.'
            $partial=Join-Path $InstallRoot 'game.download'
            & curl.exe --fail --location --retry 3 --progress-bar --output $partial $url
            if($LASTEXITCODE -ne 0){throw 'Download failed. Run the launcher again to retry.'}
            if((Get-FileHash -LiteralPath $partial -Algorithm SHA256).Hash -ne $expected){throw 'Download checksum mismatch. Game was not launched.'}
            Move-Item -LiteralPath $partial -Destination $ArchivePath -Force
        }
    }
    if((Get-FileHash -LiteralPath $ArchivePath -Algorithm SHA256).Hash -ne $expected){throw 'Archive checksum mismatch. Game was not launched.'}
    Write-Host 'Verified download. Extracting the game...'
    Expand-Archive -LiteralPath $ArchivePath -DestinationPath $InstallRoot -Force
    foreach($required in @('Windows/TheBeardAndBlade.exe','Windows/TheBeardAndBlade/Binaries/Win64/TheBeardAndBlade-Win64-Shipping.exe','Windows/TheBeardAndBlade/Content/Paks/TheBeardAndBlade-Windows.ucas')){
        if(!(Test-Path -LiteralPath (Join-Path $InstallRoot $required))){throw "Missing game file: $required"}
    }
    Set-Content -LiteralPath $marker -Value $expected -Encoding Ascii
}
Write-Host "Game ready: $InstallRoot"
if(!$NoLaunch){Start-Process -FilePath $exe -WorkingDirectory (Join-Path $InstallRoot 'Windows')}
