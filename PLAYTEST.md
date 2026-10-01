# v0.4.2 playtest

Use the Windows release archive for normal gameplay. One-off CMD test launchers have been retired.

- Explore and backtrack through all eight floors; check map persistence and trader return travel.
- Check later-room enemies, projectile alignment, elite scale and all directional animations.
- Check inventory, equipment, selling, floor loot and portrait rotation.
- Check RMB facing, block reserve reuse/recovery and partial protection from bosses.
- Charge FREEDOM with 15 kills; bosses remain immune.
- Press 2 for Tea Spirit: check the right-handed raise/sip/lower sequence, pulsing glow and outward waves, five-second immunity (including bosses), +50% movement speed after drinking, and 30-second recharge. MMB remains the separate throwing attack.
- Compare combat across early/late rooms and floors, including armor, healing affixes, attack-speed caps, coordinated attacks and trader choices.
- Throw tea with MMB: hand-sized cup, smaller splashes, no extra rotation or lingering fade. Ground impact lasts 0.6 seconds; enemy impact 0.4 seconds.
- Fight Iron Matriarch: escalating flames, meteor splash, powerful slam and final victory.
- Check audio after menus and room transitions.

## Developer checks

Run UnrealEditor-Cmd.exe with the project path, -game -unattended -nosplash -nullrhi and one verification flag at a time: -TeaSpiritVerify, -BalanceVerify, -TeaV4Verify, -HotbarVerify, -IronVerify, -AtlasVerify, -ExpansionVerify, -TraderVerify or -EconomyVerify. Add -nosound except for -IronVerify, which checks the intro audio component. Inspect completion/error logs. Use a writable project-local TEMP/TMP when the host's default temporary directory blocks SDK validation.

The legacy -DungeonVerify routine assumes the retired linear-room campaign and is not an Atlas acceptance test; use -AtlasVerify for the current eight-floor campaign. Historical notes in Docs/Archive describe intermediate builds, not current player instructions.

October 1 refresh validation: Tea Spirit (12), balance runtime (2,978), TEA effects (83), HUD (291), Iron Matriarch including intro audio (110), and expansion animations/combat (35,982) checks passed. Atlas, trader and economy verifiers completed without errors. Native Tea Spirit timing tests and 84,799 balance assertions passed. These checks supplement the approved animation preview; they do not replace a full human campaign playthrough or hosted StreamPixel testing.
