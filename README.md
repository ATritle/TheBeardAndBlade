# The Beard and Blade

A Windows pixel-art dungeon crawler built with Unreal Engine 5.8.

## Play

Download `TheBeardAndBlade-Windows-v0.3.3.zip` from [Releases](https://github.com/ATritle/TheBeardAndBlade/releases). Extract the **entire ZIP** and launch `TheBeardAndBlade/TheBeardAndBlade.exe`. Unreal Editor is not required. The automatic GitHub source ZIP is not playable. A separate `TheBeardAndBlade-StreamPixel-v0.3.3.zip` contains the same Pixel Streaming-enabled runtime in the hosting upload layout.

Windows x64 with a compatible DirectX graphics driver is required. Keep the supporting folders beside the EXE. If prerequisites are missing, run the bundled installer under `Engine/Extras/Redist/en-us` inside the extracted game folder. This is an unsigned playtest, not a browser, macOS or Linux build.

## Controls

WASD move; Shift sprint; Space dodge; mouse aim; LMB attack; RMB tea splash; MMB FREEDOM after 15 enemy kills; E interact; I inventory; P menu; M music; N effects.

Hover over inventory items for stats. Drag to rearrange the bag or drop on the matching equipment slot. Double-click a bag item to equip; double-click equipped gear to return it to the bag, or drag it into a free bag footprint. Items cannot overlap; full bags and invalid drops leave gear equipped.

## Features (current local development build)

- Seven dungeon themes and seven bosses, appearing every fourth room: Finance Guy (4), Big Mack (8), Flash Bang Guy (12), Webroot (16), Rime (20), Cinder (24), Twister (28). See [trader and progression playtest](TRADER_PLAYTEST.md).
- Multiple boss attacks, illustrated projectiles, blood effects and fading floor pools.
- Three random reward chests per cleared room: choose one, watch it open and eject loot, then press E near the landed item to collect it. Full bags preserve the same floor item. Equip it from inventory, then enter a glowing background arch.
- 60 item designs with five rarity tiers, rolled stats, coin valuations and timed combat effects. Twelve rings add bleed/poison chance, leech, damage reduction, health or stamina capacity.
- Six-by-six inventory: weapons 1×2, armor 2×2, amulets/rings 1×1. Illustrated adventurer, four active equipment slots, future armor placeholders, and framed item cards. Discard gear for its coin value; visit the trader halfway through each theme for 3–5 offers priced 20% above loot value. Coins persist for the current run.
- Directional hero animations, hand-anchored equipment, individually sized weapons and a consistent starting outfit.
- Compact health/stamina HUD, potions, music, sound effects and an illustrated title menu.
- Seven illustrated boss entrance cinematics with skip, pause, themed effects and synchronized audio. Full entrance motion is always enabled; the former R shortcut and saved reduced-motion preference no longer apply.
- Illustrated death/restart screen and campaign victory with a closing story after Twister in room 28. No final chest or room 29.

## Develop

Open `TheBeardAndBlade.uproject` in Unreal Engine 5.8 with the C++ toolchain installed. Choose Play > Selected Viewport. Imported runtime assets are included; raw/private reference artwork, caches and packaged binaries are excluded from source control.

See [PLAYTEST.md](PLAYTEST.md) for testing, [DISTRIBUTION.md](DISTRIBUTION.md) for packaging, [AUDIO.md](AUDIO.md) for sound and [RELEASE_NOTES.md](RELEASE_NOTES.md) for this version.

## Playtest limitations

No saved campaign progress. The local development campaign ends after room 28; New Run resets gear and coins. Armor changes stats without changing the hero's outfit colors. FREEDOM uses an eagle screech and graphical callout, not a recorded spoken voice. Balance and subjective animation/audio quality still need player feedback. See [current progression playtest](PROGRESSION_PLAYTEST.md) for the latest room order and editor shortcuts; older integration notes describe earlier builds. Browser hosting requires a separately prepared Pixel Streaming build; this Windows release does not enable that plugin.
