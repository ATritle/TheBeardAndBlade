$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$work=Join-Path (Split-Path (Split-Path $root -Parent) -Parent) 'work'
$gh=Join-Path $work 'github-cli/bin/bin/gh.exe'
$env:GH_CONFIG_DIR=Join-Path $work 'github-auth'
$repo='ATritle/TheBeardAndBlade'
function Api([string]$Endpoint,$Payload=$null){
    if($null -eq $Payload){$result=& $gh api $Endpoint}
    else {$result=$Payload | ConvertTo-Json -Depth 20 -Compress | & $gh api $Endpoint --method POST --input -}
    if($LASTEXITCODE -ne 0){throw "GitHub API failed: $Endpoint"}
    return ($result | ConvertFrom-Json)
}
$head=Api "repos/$repo/git/ref/heads/main"
$parent=$head.object.sha
$commit=Api "repos/$repo/git/commits/$parent"
$entries=@()
foreach($path in @('Testing/Play-Coin-Vacuum-Test.ps1','Testing/Play-Coin-Vacuum-Test.cmd','Testing/README.md')){
    $bytes=[IO.File]::ReadAllBytes((Join-Path $root $path))
    $blob=Api "repos/$repo/git/blobs" @{content=[Convert]::ToBase64String($bytes);encoding='base64'}
    $entries+=@{path=$path;mode='100644';type='blob';sha=$blob.sha}
}
$tree=Api "repos/$repo/git/trees" @{base_tree=$commit.tree.sha;tree=$entries}
$new=Api "repos/$repo/git/commits" @{message='Add standalone coin-vacuum test downloader and instructions';tree=$tree.sha;parents=@($parent)}
@{sha=$new.sha;force=$false} | ConvertTo-Json | & $gh api "repos/$repo/git/refs/heads/main" --method PATCH --input - --jq '.object.sha'
if($LASTEXITCODE -ne 0){throw 'Main advanced; no force update attempted. Inspect before retrying.'}
$tag='testing-coin-vacuum-2026-10-05'
& $gh release create $tag --repo $repo --target $new.sha --draft --prerelease --title 'Windows test - coin vacuum, room scrolling and combat polish' --notes 'Standalone Windows test, October 5, 2026. Includes automatic gold attraction, pickup sparkle and recorded coin sound, directional room scrolling, and combat/atmosphere polish. No Unreal Editor required. Download and extract the ZIP, then run Launch-Coin-Vacuum-Test.cmd. Testing also contains a verified download-and-play launcher. Stable v0.4.2 is unchanged. This is a packaged local test; the tag identifies its download instructions, not matching gameplay source.'
if($LASTEXITCODE -ne 0){throw 'Could not create draft test prerelease.'}
$zip=Join-Path $root 'Builds/CoinVacuumTest-2026-10-05/TheBeardAndBlade-CoinVacuum-Windows-2026-10-05.zip'
& $gh release upload $tag $zip --repo $repo
if($LASTEXITCODE -ne 0){throw 'Asset upload failed; prerelease remains draft.'}
Write-Output "Draft test upload complete. Commit: $($new.sha)"
