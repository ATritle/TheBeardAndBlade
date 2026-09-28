# Home-PC UE5 implementation task — Graveglass Slinger

Pull the latest ATritle/TheBeardAndBlade repository. Work from ArtSource/EnemyExpansion/GraveglassSlinger/animation-v1. Read README.md, QA.md and atlas-manifest.json before importing.

Preserve the existing character design and source PNG artwork: burgundy hood, iron mask, skeletal grave robber, teal cracked urns and leather sling. This is a source-art handoff; no runtime assets or packaged game were changed on the work PC.

CRITICAL NORTH THROW CORRECTION: use attack-N-clean-v4.png, not earlier brown-background variants. There are20 authored N poses plus4 explicitly repeated final recovery holds for24 playback frames. Honor each frame's cell, bounds, pivotX and clipRows. clipRows contains [rowOffset, xOffset, width] relative to cell; only those row spans belong to that pose. Apply as a transparency mask when extracting. Rectangular extraction alone reintroduces neighboring-pose fragments. Preserve the cleaned transparency; do not add an outer brown glow. All other directions have24 attack cells. Total authored character cells:128walk+188attack, plus40FX; do not claim320unique character poses.

Finalize artwork: correct remaining grip/hand continuity, including W/NW attacks, directional perspectives, backpack consistency, repeated stride poses, pivots and release timing. Keep the sling in anatomical right hand. Complete missing idle/hurt/death states. Current source counts do not certify smoothness. Use frame stepping and source-sheet review; import success is not visual approval.

Use existing adventurer texture/import and movement animation conventions. Add runtime sprites under /Game/Art/EnemyExpansion/GraveglassSlinger/. Inspect current roster IDs and dispatch before assigning a new unused ID; do not renumber existing enemies. Drive walking from displacement and lock throw direction during windup. Choose a release marker from the final artwork; spawn one projectile when elapsed time crosses it, including low frame rates.

Provisional Basic stats: baseHP48, rawdamage11 before armor. Apply current room scaling once. No passive contact damage. Physical urn aims at player at release and does not home after launch. Provisional speed280logicalpixels/second, lifetime2.6seconds, windup.65seconds, recovery.9seconds; tune in playtest. Swept projectile collision resolves earliest world/player hit exactly once. Player impact applies11damage; ground impact is visual only. No splash double hit, residual damage or status effect. Dodging can avoid it. Spawn appropriate separate hero/environment impact art at hit point, play once, then destroy effects. Expiry dissipates harmlessly.

This is a standard enemy, not a boss: FREEDOM removes75percent current HP unless already below25percent maxHP, when it executes. Preserve boss immunity.

Test every facing, moving/wall-blocked animation, launch/death interruptions, projectile collisions, dodge, slow and30/60/120FPS behavior. Verify existing hero/bosses and saves still work. Build/package after cleanup and tests, and report exactly what was implemented, verified and left unfinished.

