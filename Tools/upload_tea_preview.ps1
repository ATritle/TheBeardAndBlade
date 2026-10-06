$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$work = Join-Path (Split-Path (Split-Path $root -Parent) -Parent) 'work'
$gh = Join-Path $work 'github-cli/bin/bin/gh.exe'
$env:GH_CONFIG_DIR = Join-Path $work 'github-auth'
$repo = 'ATritle/TheBeardAndBlade'
$branch = 'previews/tea-spirit-2026-10-01'
$base = '1ec7f88f202e0a2c04fa3dd86babb883ebd4e6cb'

# Publish preview media only; do not touch the checkout, main, tags or releases.
@{ref="refs/heads/$branch";sha=$base} | ConvertTo-Json -Compress |
    & $gh api "repos/$repo/git/refs" --method POST --input - --jq '.ref'
if ($LASTEXITCODE -ne 0) { throw 'Preview branch creation failed. Inspect before retrying.' }

function Publish-PreviewFile([string]$name, [byte[]]$bytes) {
    @{message="Add tea skill preview: $name";branch=$branch;content=[Convert]::ToBase64String($bytes)} |
        ConvertTo-Json -Compress |
        & $gh api "repos/$repo/contents/Previews/TeaSpirit/$name" --method PUT --input - --jq '.content.html_url'
    if ($LASTEXITCODE -ne 0) { throw "Failed to publish $name" }
}
Publish-PreviewFile 'tea-spirit.gif' ([IO.File]::ReadAllBytes((Join-Path $root 'Saved/TeaSpiritPreview/tea-spirit-raise-sip-lower.gif')))
Publish-PreviewFile 'tea-spirit.mp4' ([IO.File]::ReadAllBytes((Join-Path $root 'Saved/TeaSpiritPreview/tea-spirit-raise-sip-lower.mp4')))
$readme = @'
# Tea Spirit — skill 2 preview

Actual in-game capture: raise the cup, sip, lower it, then move with a pulsing gold glow and soft outward-radiating waves.

![Tea Spirit animation](tea-spirit.gif)

[Open/download the MP4](tea-spirit.mp4?raw=true)

The action uses eight dedicated drinking poses. Protection lasts five seconds from activation, including the 1.2-second drinking animation. Movement is boosted by 50%; recharge takes 30 seconds after the effect ends.

This branch contains preview media only. The main branch and published game release have not been updated.
'@
Publish-PreviewFile 'README.md' ([Text.Encoding]::UTF8.GetBytes($readme))
