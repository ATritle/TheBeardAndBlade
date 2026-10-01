# Enemy expansion: animation handoff requirements

## Scope and current status

This is the local source-art production batch for 25 enemies in Blackout Bunker, Webroot Hollows, Glacial Reliquary, Cinder Foundry and Stormbreach Citadel. Read `full-pose-status.json` for the actual saved and missing files. A generated-sheet count is not art approval or a successful UE5 import. Do not call the handoff complete while selected files are missing.

All five Greaseworks enemies are ON HOLD: Nugget Knuckler, Ketchup Cadet, Skillet Scrapper, Fondue Conjurer and Deep-Fry Juggernaut. Do not generate, integrate or upload them with this batch. Their old drafts are retained only as history.

Forgotten Keep was integrated on the home PC in v0.4.1. Preserve Rustblade Squire, Graveglass Slinger, Chainbound Bailiff, Candle Hexer and Sepulcher Lancer and their working gameplay integration. Preserve the inventory turntable changes and the existing adventurer sprite.

## Source selection

Use the selected filenames in each `animation-v2/generation-prompts.json`. Older PNGs retained alongside corrected files are not automatically selected for import. Do not glob every PNG into the game. Turnaround boards are identity references, not animation frames.

Planned character directions are N, NE, E, SE, S, SW, W and NW. N means away from the camera; S means toward the camera. East and west must be distinct side views. Diagonal rear views must not reuse frontal poses. Never mirror artwork containing asymmetric weapons or equipment to invent the opposite direction.

Each direction has 16 movement, 24 primary attack, 8 idle, 6 hurt and 12 death source poses. Hybrid enemies also have 24 ranged attack poses per direction. Primary/ranged attacks are supplied as 12-pose A and B sheets in that order. Separate effects have their own sheet layouts; read each job's rows, columns and frame count. These are source-pose counts, not a requirement to retain duplicate poses in runtime flipbooks.

## Cleanup and registration

Review anatomy, weapon hands, equipment side, facing, alpha fringes and pose order before assembly. See `FULL_POSE_QA_NOTES.md` and each enemy's crop report. Corrected rear-view and grip sheets must replace their superseded versions. Remove background haze and accidental colored edge pixels without removing intentional emissive details, detached spell particles or body parts in a death pose.

Do not blindly slice generated sheets into equal cells. Spacing can be nonuniform. The preview's frame data and component masks are provisional measurements. For effects and death poses, preserve intended disconnected pieces. Check the first/last columns and bottom row for clipping.

Keep a constant character scale across states and directions. Register standing feet to one ground pivot; preserve the body trajectory for jumps, recoil and collapse. Do not center every death frame independently or scale a crouch to standing height. Give every extracted frame enough padding for weapons, capes, horns and effects. Verify readability at the actual dungeon camera scale with the adventurer alongside it.

## UE5 integration

Start from the latest home-PC repository checkout. Inspect the current Forgotten Keep pipeline, including `Tools/prepare_expansion.py`, `Tools/import_expansion.py` and `Tools/verify_expansion_art.py`, before adapting it. Do not overwrite newer home-PC scripts with older work-PC assumptions.

Import cleaned frames with the project's established pixel-art texture and Paper2D settings. Reuse the existing enemy animation conventions, world scale, pivots and material setup. Build looping movement and idle states, one-shot attacks/hurt/death, and explicit interruption rules. Death holds its final pose; it must not wrap back to standing. Movement playback speed should match world travel to avoid foot sliding. Preview speed is only for reviewing artwork, not final combat timing.

Choose actual windup, release/contact and recovery poses by visual review. Place damage or projectile release on a single event and prevent repeated damage from multiple displayed frames. Do not assume the old provisional frame 10 remains correct after cleanup or pose curation. A hybrid enemy has separate melee and ranged attacks.

## Projectiles and impacts

Keep projectile travel art separate from enemy and impact art. Travel loops while moving; an accepted collision spawns exactly one appropriate one-shot impact at the collision point and removes the projectile. Use the adventurer-impact effect for a player hit and the environment effect for a wall or floor hit. An impact must not continue following the player.

Read the enemy's proposed projectile settings before implementing curved flight, arcs or area damage. When tracking is enabled, keep it readable, limit turning, and ensure a successful dodge defeats the shot as intended. Do not rotate strongly asymmetric art unless the result remains correct; author additional travel facings where needed. Prevent direct-hit and splash damage from double-counting the same target. Expiry should use the specified harmless dissipation behavior.

## Balance and gameplay checks

Health, damage and Basic/Medium/Elite classifications in `enemy-spec.json` are proposals. Compare them with the current game before applying values. Keep balance data editable in the project's established configuration rather than hiding it in sprite assets. Ensure attack reach and hurt shapes match the intended combat, not the transparent padding around an image.

Preserve the requested FREEDOM rule: for standard enemies, check pre-hit health; execute only if it is below 25% of maximum health, otherwise deal 75% of current health. Bosses are immune. Do not accidentally turn these standard enemies into bosses through reused immunity settings.

Test every direction, animation interruption, hurt/death transition, projectile collision, dodge, status application and damage event in the dungeon. Confirm no attacks continue after death and no damage is duplicated. Run the current repository's verification/build checks before packaging a new gameplay release. Work-PC source previews are not evidence of a UE5 build passing.

## Checkpoint

This is the authorized 2026-09-29 source checkpoint. Continue missing jobs from this folder; never resume the obsolete HomePCHandoff-v1 queue. Resolve all prompt paths from the repository root.
