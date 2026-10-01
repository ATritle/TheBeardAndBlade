# v0.4.2 playtest

Use the Windows release archive for normal gameplay. One-off CMD test launchers have been retired.

- Explore and backtrack through all eight floors; check map persistence and trader return travel.
- Check later-room enemies, projectile alignment, elite scale and all directional animations.
- Check inventory, equipment, selling, floor loot and portrait rotation.
- Check RMB facing, block reserve reuse/recovery and partial protection from bosses.
- Charge FREEDOM with 15 kills; bosses remain immune.
- Throw tea with MMB: hand-sized cup, smaller splashes, no extra rotation or lingering fade. Ground impact lasts 0.6 seconds; enemy impact 0.4 seconds.
- Fight Iron Matriarch: escalating flames, meteor splash, powerful slam and final victory.
- Check audio after menus and room transitions.

## Developer checks

Run UnrealEditor-Cmd.exe with the project path, -game -unattended -nosplash -nosound -nullrhi and one verification flag at a time: -TeaV4Verify, -HotbarVerify, -IronVerify, -AtlasVerify or -ExpansionVerify. Inspect completion/error logs. Historical notes in Docs/Archive describe intermediate builds, not current player instructions.
