# v0.4.4 playtest

Current focus: test full-body melee and bow movement in all eight directions;
check one-handed grip, smaller weapon size and blade orientation; tap bow attack
and hold to the 1.5-second automatic power shot, then release/rearm. Inspect
independently fading arrow wakes, skill/enemy effects and doorway chains. Check
trader batch sales and potion purchases. Finance's descent must enter the bunker
as floor two, with no Grease encounter. Boss-effect refinements are deferred.

Use the Windows release archive for normal gameplay. One-off CMD test launchers have been retired.

## Death/revival refresh

- Death freezes the encounter, shows a rising blue spirit above the fallen adventurer, then offers revival or a new descent after 2.4 seconds.
- Revive costs all held gold for balances 1–2,499; balances of 2,500 or more pay half, rounded up. Zero gold disables revival. Enter selects revival, never silently starts a new run.
- Check return to the previous cleared room, full health/stamina, retained equipment, bag, skill charge/cooldowns, explored map and trader stock. First-room deaths return to the floor entrance.
- Re-entering the failed room retries the encounter; bosses regain full health. Broken props and uncollected drops are preserved, not rerolled. Cleared rooms remain cleared.
- Run `-RevivalVerify` headlessly, or `-RevivalReview` with rendering for an automatic nine-second capture in `Saved/RevivalReview`. Native cost checks: `Tools/revival_probe.cpp`.

## Release playtest

- Explore and backtrack through all eight floors; check map persistence and trader return travel.
- Check later-room enemies, projectile alignment, elite scale and all directional animations.
- Check inventory, equipment, selling, floor loot and portrait rotation.
- Check RMB facing, block reserve reuse/recovery and partial protection from bosses.
- Charge FREEDOM with 15 kills; bosses remain immune.
- Press 2 (or your remapped key) for Tea Spirit: check the right-handed raise/sip/lower sequence, pulsing glow and outward waves, ten-second immunity (including bosses), +50% movement speed after drinking, and 30-second recharge. MMB remains the separate throwing attack.
- Compare combat across early/late rooms and floors, including armor, healing affixes, attack-speed caps, coordinated attacks and trader choices.
- Throw tea with MMB: hand-sized cup, smaller splashes, no extra rotation or lingering fade. Ground impact lasts 0.6 seconds; enemy impact 0.4 seconds.
- Fight Iron Matriarch: escalating flames, meteor splash, powerful slam and final victory.
- Check audio after menus and room transitions.

## Developer checks

Run UnrealEditor-Cmd.exe with the project path, -game -unattended -nosplash -nullrhi and one verification flag at a time: -TeaSpiritVerify, -BalanceVerify, -TeaV4Verify, -HotbarVerify, -IronVerify, -AtlasVerify, -ExpansionVerify, -TraderVerify or -EconomyVerify. Add -nosound except for -IronVerify, which checks the intro audio component. Inspect completion/error logs. Use a writable project-local TEMP/TMP when the host's default temporary directory blocks SDK validation.

The legacy -DungeonVerify routine assumes the retired linear-room campaign and is not an Atlas acceptance test; use -AtlasVerify for the current eight-floor campaign. Historical notes in Docs/Archive describe intermediate builds, not current player instructions.

October 1 refresh validation: Tea Spirit (12), balance runtime (2,978), TEA effects (83), HUD (291), Iron Matriarch including intro audio (110), and expansion animations/combat (35,982) checks passed. Atlas, trader and economy verifiers completed without errors. Native Tea Spirit timing tests and 84,799 balance assertions passed. These checks supplement the approved animation preview; they do not replace a full human campaign playthrough or hosted StreamPixel testing.
# Carried health potions (local test)

## Full Settings test

- Audio: master, music, sound effects, voices, interface sliders, plus existing music/SFX mute controls. Preferences saved on release/back. Music retains the quiet dungeon mix.
- Display: supported resolutions, fullscreen/borderless/windowed, VSync, frame limit and engine graphics presets. Borderless uses desktop resolution. Apply previews for 15 seconds; Keep persists, Revert/timeout/Back restores previous settings.
- Controls: 16 keyboard/mouse actions, conflict rejection, Escape-to-cancel capture, reset defaults. Escape and Enter remain fixed menu controls. Mouse selection remains left-click even after rebinding Attack. Keyboard/mouse only; no controller remapping UI in this pass.
- HUD: potion, FREEDOM, Golden Tea, attack, throw, block and map show the live binding; mouse-button graphics change appropriately. How to Play reads the same binding table.
- Keybinds/audio are stored in GameUserSettings.ini under DungeonKeys/DungeonAudio; older installs default to existing bindings. Disk run Save/Load is still not implemented.
- `-SettingsVerify` exercises conflicts, action dispatch, release behavior, audio routing, config reload in an isolated test file, and display timeout rollback. `-DungeonCapture -DungeonMenuPreview -DungeonSettingsPreview -SettingsPage=0/1/2` captures each page.
- Launcher: `Builds/FullSettingsTest-2026-10-01-v2/Launch-Settings-Test.cmd`.

## Title / pause menu refresh

- Original TeaTitle background unchanged. Live sharp text, translucent gold-ruled buttons and emerald hover accents replace baked-label buttons.
- Initial menu: New Game, Settings, How to Play, Quit. Active run: Resume, Settings, How to Play, Quit; no accidental New Run action.
- Settings exposes existing persisted music/SFX toggles and master volume. Escape/P returns from subpages before resuming. Enter only starts/resumes at the root menu.
- Quit requires explicit click confirmation and warns that the run is not saved. Save/load is not implemented in this update.
- How to Play now includes Q/potion capacity. `-MenuVerify` checks title, settings, help, pause/resume and quit-cancel state transitions.
- Launcher: `Builds/TitleMenuTest-2026-10-01/Launch-Title-Menu-Test.cmd`.

- Golden Tea (2): protection and +50% movement speed now last 10 gameplay seconds from activation. Sip timing and 30-second recharge after the buff expires remain unchanged. HUD/aura and help text use the new duration. Launcher: `Builds/GoldenTea10sTest-2026-10-01/Launch-Golden-Tea-Test.cmd`.

- Q now uses its own eight-pose red health-potion bottle sheet (`ArtSource/HeroPotion/Potion_DrinkSheet.png`); tea on 2 retains its original cup. Timing and healing are unchanged. Updated launcher: `Builds/PotionBottleTest-2026-10-01/Launch-Potion-Test.cmd`.

- HUD refinement: potion arc now shares a continuous textured gold chassis rail, sampled from the existing HUD artwork, with ends tucked under the dragon and lightning mount. `-PotionReview` captures full, partial and empty states in `Saved/PotionBorderReview`.
- Updated standalone launcher: `Builds/PotionBorderTest-2026-10-01/Launch-Potion-Test.cmd`.

- Walk over a potion to store it, including at full health. Carry up to four; excess remains in the room and persists on backtracking.
- Q consumes one charge and restores 25% of current maximum health, capped at maximum. No consumption at full health, while dead, in menus, or during conflicting actions.
- Four red gemstone segments above the blue stamina arc show carried charges. New runs start empty; revival retains unused charges.
- Quick 0.65-second raise/sip/lower animation reuses the authored tea-drinking poses. Feet stay planted and other hand actions are locked during the sip. This does not grant tea's invulnerability or speed buff.
- `-PotionVerify`: 15 pickup, capacity, use, animation lock, gear-health scaling, revival and reset checks.
- Earlier standalone test paths above are local-only. For the complete refresh, use the replacement v0.4.2 release download.
- Key capture regression: remap Golden Tea from 2 to G. Verify capture accepts G, the HUD shows G, and the binding survives a restart. `-SettingsVerify` includes this case (25 checks).
