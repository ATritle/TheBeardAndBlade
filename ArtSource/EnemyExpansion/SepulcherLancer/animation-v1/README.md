# Sepulcher Lancer — animation source draft

Fifth Forgotten Keep enemy: an Elite blackened knight with a coffin shield, burgundy cloth and a spectral bone lance. These are generated pixel-art sources and an offline review, not imported UE5 assets.

Open `preview.html` to review eight directions. Select Walk, Thrust, Spectral lance, Player hit or Ground / wall hit. Playback starts automatically; Pause, pose arrows, scrubber and half-speed controls support inspection.

## Content

- 16 walking source poses per direction, 128 total.
- 24 thrust/recovery source poses per direction, split across two 12-pose sheets, 192 total.
- Separate 12-pose projectile travel, 12-pose player impact and 16-pose environment impact effects.
- Generation prompts, crop metadata and a home-PC integration task.

Pose counts describe source cells, not a guarantee of equally distinct or production-polished frames. The character must retain the shield on the anatomical left arm and lance in the right hand. Never mirror a view to manufacture another direction. `walk-N-back.png` replaces the original north walk sheet to show the shield's back and straps.

`turnaround.png` is an opaque design reference only; do not import its background into gameplay. The preview uses original PNGs with canvas crop metadata. It does not create final cleaned individual sprites or Paper2D assets. `build_review.py` measures alpha and rebuilds the embedded preview using Python and Pillow; it does not repaint source pixels. Pixel cleanup, fixed pivots, final timing, idle/hurt/death states and UE5 verification remain necessary. Read `QA.md` and `HOME_PC_TASK.md` before integration.

## Proposed combat values

Elite; base health 175; raw attack damage 26. These values are editable tuning proposals. Apply dungeon scaling once. Melee and ranged actions share the thrust artwork but use mutually exclusive gameplay events. No contact damage, no untelegraphed double hit.

Ranged attack: readable 1.2-second windup, one straight spectral lance aimed at the player's position at release, 270 logical pixels/second, maximum lifetime 2.6 seconds, 1.6-second recovery. Convert logical pixels to the project's actual world units. No homing, arc or area damage in this first version. The physical lance remains in the knight's hand.

As a standard enemy, Lancer follows the requested FREEDOM rule: execute only if already below 25% of maximum health; otherwise deal 75% of current health and leave the enemy alive. Boss immunity is not applicable to this enemy.
