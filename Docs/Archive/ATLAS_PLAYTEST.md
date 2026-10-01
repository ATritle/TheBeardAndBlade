# Emerald Atlas — full-campaign UE playtest (historical development notes)

Local prototype only; no release or GitHub push requested.

Run `Play_Atlas_UE.cmd` with UE 5.8 installed, or open TheBeardAndBlade.uproject and Play. Choose Begin Descent / New Run for the full campaign. Existing boss/enemy shortcuts also use the new navigation unless launched with `-LegacyProgression`.

## Controls

- M opens/closes the Emerald Atlas. It pauses combat and shows only entered rooms and known doorway stubs, not hidden destinations.
- E near an open doorway walks/fades through it; arrival uses the opposite side of the adjacent room.
- Entering the trader doorway opens the shop directly, without a playable trader room. Leave Shop returns through the same doorway into the room you came from, with an arrival fade. Stock remains sold when revisiting.
- Rooms now contain 2–3 props instead of 4–6; the chance of a prop dropping nothing is unchanged.
- I manages inventory. Discarding drops gear on the floor; sell gear at the trader for coins.
- F8 toggles music; M no longer changes audio. The main-menu audio controls remain available.

## Campaign scope

Each of the seven floors has fourteen unique rooms: a safe entrance, eight combat rooms along the boss route, two optional combat rooms on a dead-end branch, one boss endpoint, one trader branch and one reward dead end. The layout is independently rotated/reflected on each floor; it is an authored, non-overlapping tree rather than a large procedural maze. The boss is nine connections from the entrance. Room directions are real and reciprocal. A closed gate indicates either a sealed wall or a combat-locked exit.

### Expanded encounters

Encounter composition uses shortest-path room depth, not visit order. Each combat room retains two waves of 4–6 enemies; new enemies replace slots rather than piling on extra enemies. Room-based health, damage and loot scaling are unchanged, so extra rooms do not inadvertently multiply chapter scaling. Depth 2 introduces lighter new enemies, depth 4 introduces casters, and depths 6–8 include one heavy/elite in the first wave only.

- Keep: Rustblade Squire, Graveglass Slinger, Chainbound Bailiff, Candle Hexer and Sepulcher Lancer; later rooms mix established specialists.
- Greaseworks: existing food-themed roster with later brute/cook encounters; no unrelated expansion artwork is assigned here.
- Blackout: Trench Shivver, Rivet Gunner, Coil Saboteur, Bunker Bulwark.
- Webroot: Silkfang Skitter, Briar Spitter, Sporebell Witch, Thornweave Sentinel.
- Rime: Icicle Flinger, Hailshot Gargoyle, Snowveil Oracle, Permafrost Templar.
- Cinder: Coalgnash, Cinder Pitcher, Ashen Cantor, Bellows Brute, Crucible Colossus.
- Storm: Gale Talon, Hailshot Gargoyle, Thunderhead Adept, Tempest Duelist. Duelist's existing enlarged scale and 1.5x speed remain unchanged.

All 20 enabled expansion species plus the five Keep additions appear in campaign encounters. Breach Hound, Stormcoil Behemoth, Glacierback Ram, Mossmaw Stalker and Rimeclaw Cub remain disabled as previously requested. Boss order, shop stock persistence, discovered-only map, doorway fading and saved room loot are unchanged.

Validation: Development Editor compiled successfully. Expanded `-AtlasVerify` passed with zero errors: 128 seeds across all seven floors, depth/adjacency/connectivity, every combat room and both waves, all enabled expansion species, elite gating, health scaling, persistent clears, trader/reward persistence, cardinal doorway fades and full boss progression.

Boss entrances have no portrait or special red glow: they look like ordinary combat doorways. Boss portraits on the map appear only for rooms already explored; hidden destinations are never drawn. The inventory and trader wallet displays use the rugged gold B coin instead of the COINS label (see CURRENCY_ART.md).

Boss/theme order: Finance Guy / Forgotten Keep → Big Mack / Greaseworks → Flash Bang Guy / Blackout Bunker → Webroot / Webroot Hollows → Rime / Glacial Reliquary → Cinder / Cinder Foundry → Twister / Stormbreach Citadel. Enemy and loot scaling retain each theme's existing four-room level range. Trader stock and reward-end loot now scale to the current floor rather than staying at first-floor levels.

Combat doors open after the room is cleared; chest loot is optional. The reward branch is safe and offers a choice of rare/epic loot. Cleared rooms do not respawn enemies, chests, props or stock. Uncollected items and potions remain in their room. These snapshots persist within the current run, not across application restarts.

Each of the first six boss defeats opens one free doorway as the descent. Collect any wanted loot, then use E at the gold stair marker. The next themed floor starts with a fresh explored map and independent room/shop state, while gear, health, stamina and coins carry forward. Backtracking is within the current floor; descent is one-way. Defeating Twister still triggers the existing victory screen; no eighth floor is created.

The south exit remains usable with the compact HUD. Ordinary doorway travel takes 2.2 seconds with a gradual character/weapon fade and a 0.75-second arrival. Boss descent takes four seconds.

The top floor/map labels are removed. The compact HUD uses mouse icons for LMB attack, MMB tea and RMB block, with FREEDOM on 1 and M for the map. Health and stamina have no numeric labels. The open map repeats the parchment icon beside an embossed gold-and-emerald EMERALD ATLAS title graphic. Artwork paths and prompts are in [ATLAS_UI_ART.md](ATLAS_UI_ART.md).

## Validation

Expanded-floor visual review: all fourteen discovered rooms fit inside the Emerald Atlas border at gameplay resolution. Review-only full discovery uses `-AtlasReview -AtlasExpandedMapReview` and writes `Saved/AtlasExpandedMap.png`; normal gameplay still reveals only visited rooms.

2026-09-27 local validation: UE 5.8 Editor Development build succeeded. AtlasVerify, legacy ProgressionVerify, EndingVerify and TraderVerify passed. Rendered UE room captures were reviewed for all seven themes at 1280×800, plus the first and final floor map screens. Room/map/doorway captures were saved for all seven themes. The Atlas test exercises all four door directions on every floor. Test processes exit automatically; no playtest is left running at handoff.

- `-game -AtlasVerify -nullrhi`: 128 seeds per floor (896 floor layouts), unique cells, connected tree, reciprocal exits, exactly one of each special room, minimum boss distance, fog/discovery, all four transition directions and opacity, map pause, reward/prop/potion persistence, no duplicate pickup, full seven-floor roster/loot/shop progression, boss non-respawn, six descents preserving player state, and Twister victory.
- `-game -AtlasReview -AtlasReviewChapter=0`: captures room, map, doorway fade and arrival to ArtSource, then exits. Chapter 0–6 selects the floor; `-AtlasReviewDoor=0` through `3` selects a cardinal exit.
- Existing legacy verification flags should include `-LegacyProgression` to isolate the unchanged sequential campaign and old fixtures.

Review manually: clear the first combat room, open M, explore one branch, return through the opposite door, buy/discard gear, leave and retrieve a chest drop, locate Finance Guy, and descend. Check the doorway approach and fading from every side.

Repeat navigation/shop/backtracking on each later floor. Check each boss matches its theme and the final boss ends the run.

## Artwork provenance

Built-in image generation created `ArtSource/Exploration/AtlasChamber.png`; imported by `Tools/import_atlas.py` to `/Game/Art/V2/AtlasChamber` with nearest filtering, no mipmaps and no texture streaming. The map uses the existing gold/emerald InventoryFrame, original clean font rendering, deterministic room/link graphics, and existing loot/merchant icons. No generated diagram is used as map logic.

Six matching four-way themed backgrounds were generated with the built-in image-generation tool and saved as `ArtSource/Exploration/AtlasChamber1.png` through `AtlasChamber6.png`; the importer creates the corresponding UE textures. Full prompts and asset paths are documented in [ATLAS_ART_PROMPTS.md](ATLAS_ART_PROMPTS.md).

Prompt: Production game background for The Beard and Blade, landscape 1536x1024, high-quality crisp detailed pixel-art medieval stone treasury dungeon. Slight top-down, screen-aligned rectangular walls, dark blue-grey stone with faint moss, amber torches and antique brass. Exactly four dark doorway passages centered on the north, south, east and west walls. South is a cutaway passage. Huge empty walkable tiled center. No characters, enemies, loose props, loot, text, HUD, border, logo or ordinary-exit stairs. Sharp pixel clusters, consistent lighting and scale.
