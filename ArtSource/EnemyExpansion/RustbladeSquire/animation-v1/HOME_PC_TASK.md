# Home-PC implementation task

Pull the latest ATritle/TheBeardAndBlade repository. Start with ArtSource/EnemyExpansion/RustbladeSquire/animation-v1/README.md, QA.md, selected-atlases.json and atlas-manifest.json.

Use these existing PNGs as source artwork. Preserve the approved Rustblade Squire identity. The user specifically corrected SW/NW to keep the sword in the anatomical right hand and asked to remove neighboring-pose clipping in SW. Do not revert those corrections or regenerate the character design.

This is a source-art draft, not an integrated enemy. It contains 128 walking and 192 attack pose cells over eight directions. Idle, hurt and death are unfinished. A cell count does not establish unique poses or production readiness.

First validate crops and fix outstanding crop flags, sword proportions, grip continuity in other directions, duplicated/abrupt poses, gait, and looping. SW uses measured individual rectangles and pivotX offsets; do not re-slice as uniform cells. NW/SW attacks were simplified to a one-handed overhead strike. Re-evaluate strike markers on the final artwork instead of blindly using frame 10.

Inspect the current game code and current adventurer import/animation pipeline before implementing. Prior scan found DungeonActors.cpp rendering, DungeonCampaign.cpp animation and DungeonCombatBalance.h balance; confirm their current locations and behavior. Allocate a new stable enemy ID without changing existing IDs or broad numeric dispatch rules. Place approved runtime sprites under /Game/Art/EnemyExpansion/RustbladeSquire/. Keep source files in ArtSource.

Use eight independently authored facing directions (do not mirror handedness), displacement-based walking, fixed body scale/pivots, nearest filtering and existing pixel-art import conventions. Complete missing states. Lock attack facing; trigger one melee damage event on a crossed strike timestamp with swept/appropriate reach checks. Do not let sprite size determine hitbox size. Provisional Basic tier: base health65, raw primary damage14; apply current room scaling once and preserve existing armor rules. This standard enemy is vulnerable to FREEDOM: execute only when already below25% maximum HP; otherwise remove75% current HP. Boss immunity remains unchanged.

Test all directions, walls, hit reach, dodge, slow, interruptions, death, 30/60/120 FPS and current save/progression behavior. Keep the existing adventurer and bosses working. Build and package only after validation. Report completed work, remaining visual issues, test results, and exact modified files. Do not describe untested artwork as final.
