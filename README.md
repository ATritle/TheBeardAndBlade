# The Beard and Blade

A pixel-art dungeon crawler starring a bearded adventurer armed with steel, stubbornness, and an explosive cup of tea.
Explore seven themed dungeon floors through branching rooms and four-way doorways. Chart your discoveries with the Emerald Atlas, backtrack through cleared chambers, and choose which unexplored path to brave next. Somewhere ahead, a boss awaits—but the doorway won’t give away the surprise.
Break props for loot, collect fallen enemies’ coins, and trade for stronger equipment. Build your adventurer around powerful weapons, armor, rings, and amulets with bonuses such as bleed, poison, life leech, attack speed, and movement.
Features
- Seven distinct dungeon themes with persistent room exploration and hidden boss encounters.
- Seven unusual bosses: Finance Guy, Big Mack, Flash Bang Guy, Webroot, Rime, Cinder, and Twister.
- Action-focused combat with melee attacks, dodging, sprinting, throwable tea, and the charged FREEDOM ability.
- Eight equipment slots, multiple loot rarities, and detailed stat comparisons.
- A coin-and-trader economy: buy upgrades, sell unwanted gear, or leave items behind to recover later.
- Detailed pixel-art presentation, animated boss introductions, atmospheric music, and recorded sound effects.

## Play

Download `TheBeardAndBlade-Windows-v0.4.1.zip` from [Releases](https://github.com/ATritle/TheBeardAndBlade/releases). Extract the **entire ZIP** and launch `TheBeardAndBlade/TheBeardAndBlade.exe`. Unreal Editor is not required. The automatic GitHub source ZIP is not playable. A separate `TheBeardAndBlade-StreamPixel-v0.4.1.zip` contains the same Pixel Streaming-enabled runtime in the hosting upload layout.

v0.4.1 adds five animated Forgotten Keep enemies and a 32-view inventory adventurer. Open inventory with I, then hold LMB over the portrait and drag horizontally to rotate. Gameplay adventurer sprites are unchanged.

Windows x64 with a compatible DirectX graphics driver is required. Keep the supporting folders beside the EXE. If prerequisites are missing, run the bundled installer under `Engine/Extras/Redist/en-us` inside the extracted game folder. This is an unsigned playtest, not a browser, macOS or Linux build.

## Controls

WASD move; Shift sprint; Space dodge; mouse aim; LMB attack; RMB tea splash; MMB FREEDOM after 15 enemy kills; E interact; I inventory; P menu; M Emerald Atlas; F8 music; N effects.

v0.4.0: all seven floors now use eight persistent, directionally connected rooms each, an Emerald Atlas map, one direct-to-shop trader branch, one reward dead end and one boss endpoint. Boss order is unchanged; each defeated boss opens a descent to the next theme, and Twister ends the run. Start a local UE test using `Play_Atlas_UE.cmd`. See [Atlas playtest](ATLAS_PLAYTEST.md) and [coin economy](ECONOMY_PLAYTEST.md). Discard drops gear; trader sales and enemy coin pickups fund purchases.

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
