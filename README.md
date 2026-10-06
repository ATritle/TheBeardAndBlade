# The Beard and Blade

A pixel-art dungeon crawler starring a bearded adventurer armed with steel, stubbornness, and an explosive cup of tea.

Explore seven active themed dungeon floors with branching rooms, four-way doors, persistent backtracking, hidden bosses, trader branches and the Emerald Atlas map. Grease is temporarily excluded for rework; its assets are retained. Tougher enemies appear deeper into each floor. Build your character with eight equipment slots, rare loot, coin drops and trader buying/selling.

## Play v0.4.4

Download `TheBeardAndBlade-Windows-Vagon-v0.4.4.zip` from [Releases](https://github.com/ATritle/TheBeardAndBlade/releases/tag/v0.4.4). Extract the entire archive into a fresh folder and launch `TheBeardAndBlade.exe` at its root. Unreal Editor is not required. GitHub's automatic source ZIP is not playable. This release includes full-body adventurer animations, enchanted bows, Niagara effects, integrated doorway chains and refined one-handed weapon grips.

For Vagon Streams, upload the complete ZIP and select `TheBeardAndBlade/Binaries/Win64/TheBeardAndBlade-Win64-Shipping.exe`. Preserve all folders and dependencies. Hosted upload and activation remain separate steps.

## Controls

Default bindings: WASD move; Shift sprint; Space dodge; mouse aim; LMB attack; MMB throw tea; RMB block; 1 FREEDOM after 15 kills; 2 Tea Spirit; Q health potion; E interact; I inventory; P/Escape pause; M Emerald Atlas. Settings supports keyboard/mouse remapping, with matching HUD and help labels.

Hold LMB (or the rebound attack key) for repeated swings. The third consecutive landed melee swing is a heavy finisher with 50% bonus damage and an illustrated COMBO popup. Misses, interruptions, or a 1.4-second pause between attacks reset the chain. A cleave counts as one swing, regardless of how many enemies it hits.

Tea Spirit: drink a cup of tea for ten seconds of invulnerability and 50% increased movement speed. The protected window includes the 1.2-second raise/sip/lower animation; movement resumes after drinking. A pulsing gold glow and outward-radiating aura mark the effect. Recharge takes 30 seconds after protection ends. The skill has its own HUD icon and does not replace the MMB tea attack.

Carry up to four health potions. Q drinks one to restore 25% of maximum health, with a dedicated bottle animation and four charges integrated above the stamina meter. Excess potions remain on the floor.

Death offers a ghost-animation revival in the previous safe room: below 2,500 gold it costs all gold; at or above 2,500 it costs half, rounded up. With zero gold, revival is unavailable. Gear, unused potions and exploration are retained; the failed encounter resets.

Settings includes master, music, effects, voices and interface volume; fullscreen, borderless and windowed modes; resolution, VSync, frame-rate limits and quality presets. Display changes have a 15-second confirmation timeout. Preferences persist between launches; campaign saves are not implemented.

Block uses a five-second reserve that refills when released. Remaining reserve can be reused immediately. Face the incoming attack: regular attacks are blocked fully; boss attacks retain 75% damage.

Inspect gear in inventory. Drag or double-click to equip/unequip. Drag horizontally over the adventurer to rotate the portrait. Discarded items return to the floor; sell at the trader for coins.

## Campaign

Boss order: Finance Guy, Flash Bang Guy, Webroot, Rime, Cinder, Twister, then Iron Matriarch in the Iron Aerie. Grease / Big Mack is parked for future rework. Floor numbers and balance tiers remain consecutive.

## Develop

Open TheBeardAndBlade.uproject in UE 5.8 with the Windows C++ toolchain. Imported runtime assets are included. See [testing](PLAYTEST.md), [packaging](DISTRIBUTION.md), [StreamPixel](STREAMPixel_SETUP.md), [audio](AUDIO.md), and [release notes](RELEASE_NOTES.md). Historical development notes are in Docs/Archive; artwork handoffs remain beside their sources.

## Limitations

Unsigned Windows x64 playtest. No campaign save, multiplayer or automatic updater. Progress persists within a run, not after restarting. Bright flash effects remain. StreamPixel needs separate upload and browser validation.
