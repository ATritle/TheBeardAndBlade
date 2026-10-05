@echo off
setlocal
title The Beard and Blade - Download and Play Test
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$ErrorActionPreference='Stop'; try { $dir=Join-Path $env:LOCALAPPDATA 'TheBeardAndBladeTests/coin-vacuum-2026-10-05'; New-Item -ItemType Directory -Force -Path $dir | Out-Null; $helper=Join-Path $dir 'Play-Coin-Vacuum-Test.ps1'; $expected='C52CD9600454E87A6400BE71427314E166ADB7130DB3F8F6D7F424CEDFFB3675'; if(!(Test-Path -LiteralPath $helper) -or (Get-FileHash -LiteralPath $helper -Algorithm SHA256).Hash -ne $expected){ & curl.exe --fail --location --retry 3 --silent --show-error --output $helper 'https://raw.githubusercontent.com/ATritle/TheBeardAndBlade/testing-coin-vacuum-2026-10-05/Testing/Play-Coin-Vacuum-Test.ps1'; if($LASTEXITCODE -ne 0){throw 'Could not download the test helper.'} }; if((Get-FileHash -LiteralPath $helper -Algorithm SHA256).Hash -ne $expected){throw 'Helper checksum mismatch. Nothing was run.'}; & $helper } catch { Write-Host $_ -ForegroundColor Red; exit 1 }"
if errorlevel 1 (
  echo.
  echo Download or startup failed. Please report the message above.
  pause
  exit /b 1
)
exit /b 0
