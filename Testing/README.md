# Standalone Windows test — October 5, 2026

Includes automatic gold pickup, collection sparkle and coin sound, directional room scrolling, and the latest combat/atmosphere polish. **Unreal Editor, Git, and Blender are not required.**

## Download and play

1. [Download Play-Coin-Vacuum-Test.cmd](https://raw.githubusercontent.com/ATritle/TheBeardAndBlade/testing-coin-vacuum-2026-10-05/Testing/Play-Coin-Vacuum-Test.cmd). Save it as a `.cmd` file, not `.txt`.
2. Double-click it on a Windows 10/11 x64 PC. The launcher downloads and verifies the helper and complete game ZIP, extracts it, then starts the game.
3. Keep the launcher to play again without downloading the game again.

The download is about **1.13 GB**. Allow **4 GB of free space** for the archive and extracted game. Files are stored in `%LOCALAPPDATA%\TheBeardAndBladeTests\coin-vacuum-2026-10-05`. Existing game settings/save data may still be shared with your other builds.

## Manual ZIP option

[Download the complete Windows ZIP](https://github.com/ATritle/TheBeardAndBlade/releases/download/testing-coin-vacuum-2026-10-05/TheBeardAndBlade-CoinVacuum-Windows-2026-10-05.zip), extract **everything**, then run `Launch-Coin-Vacuum-Test.cmd` or `Windows\TheBeardAndBlade.exe`. Do not move the EXE away from its folders. If Windows reports a missing Visual C++ runtime, run the included `vc_redist.x64.exe` under `Windows\Engine\Extras\Redist\en-us`.

This is an unsigned test build; Windows may show a publisher/SmartScreen warning. Only run it if you trust this repository. Do not disable Windows security globally.

## What to test

- Kill enemies: coins should briefly appear, then fly into the adventurer from anywhere in the current room.
- Collection should show gold sparkles and a `+gold` popup, play a recorded coin sound, and add the amount exactly once.
- Check movement while coins fly, inventory pause, and room backtracking.
- Room scrolling and earlier combat polish are included.

[Test prerelease and full download](https://github.com/ATritle/TheBeardAndBlade/releases/tag/testing-coin-vacuum-2026-10-05). The stable **v0.4.2 release is unchanged**. Large game files are hosted as prerelease assets because they exceed GitHub's per-file repository limit; this folder contains the downloader and instructions. The prerelease packages the local test build; its tag does not claim the gameplay source on main matches this binary.

ZIP SHA-256: `CF8FC6C6465F4245A08619DC25D184B8240A84634C73D38CCB02677A0546025D`
