# Rustblade Squire — movement and attack draft

This is the first actual animation asset pass for the 35-enemy expansion, created on the work PC from the existing Forgotten Keep concept. It contains generated poses, not merely concept illustrations or a camera animation. No UE5 installation was used. This folder is the source-art handoff. No runtime game code or packaged build is changed by this upload.

## Contents and limits

- Target: eight direction-specific walk atlases, 16 poses each (128 walking poses), and eight attack atlases, 24 poses each (192 attack poses).
- Directions: N, NE, E, SE, S, SW, W, NW. Walking grid is 4x4; attack grid is 4x6.
- Preview plays those source poses directly, with pause, speed, action selection and frame stepping. No image crossfades or synthetic limb interpolation.
- `atlas-manifest.json` records measured source rectangles, alpha extrema and technical crop warnings for the files actually present. It is the inventory authority; planned filenames in prompts do not establish completion.
- `generation-prompts.json` and `correction-prompts.json` preserve the built-in image-generation instructions.
- Static concept boards for the other 34 enemies are in the sibling design-v1 pack. Their animation sheets have NOT been produced in this pass.
- Idle, hurt and death animations are still pending, along with projectiles/impacts for other enemy types. Rustblade Squire is melee and requires no projectile.

## Artistic review status

These are draft animation sources, not production-approved frames. A full sheet and non-empty cells do not prove correct alternating steps or consistent anatomy. Review playback and frame steps for repeated lead legs, weapon-grip changes, altered sword length, torso drift, perspective consistency, and seams at cycle wrap. Some weapon/shoulder visibility in rear and left views requires anatomical review. Do not claim a pose is approved simply because it has transparency or an import succeeds.

First N/NE walking generations brought swords to the right edge; revised v2 sheets are intended to address margins. The first SW walking generation retained SE facing and must not be treated as an approved southwest reference. Retain originals as rejected/history sources. Use selected filenames in atlas-manifest.json for the current preview. See crop flags and actual revised artwork before import.

The preview's optional foot alignment adjusts display placement only. It does not rewrite the artwork or certify final pivots. The moving-floor option is only a visual aid, not gameplay or a claim that stride has been calibrated.

## Home-PC UE5 integration

1. Pull the latest repository and copy this whole folder. Open preview.html in Edge/Chrome; run build_preview.py with Python and Pillow available to produce a self-contained preview if needed. Review preview.html and source PNGs before replacing or importing assets. Do NOT regenerate character art just to import it.
2. Use the measured frame rectangles from atlas-manifest.json, then manually validate each crop and neighboring-cell separation. Actual source dimensions can differ from requested dimensions; never assume fixed 256/384/512 cells without checking.
3. Extract approved poses to a uniform transparent canvas, align body/ground pivots, and keep sword arcs inside the frame. Preserve natural anatomical movement; do not normalize each pose to a different scale. Fix any clipping and grip inconsistencies first.
4. Import additively under /Game/Art/EnemyExpansion/RustbladeSquire/ using the existing pixel-art settings: nearest filtering, no mipmaps, appropriate UI/icon compression and no texture streaming. Follow the current repository import conventions.
5. Give the new enemy an explicit profile and stable new ID. At the last inspected revision existing IDs were 0–48; verify the current roster and allocate an unused ID without shifting existing IDs. Avoid relying on current modulo-six roster selection or species>=31 assumptions for this new family.
6. Drive the 16-frame walk phase from actual displacement, using species stride length and movement facing. Stop the walk phase at a wall. Attack facing locks to the chosen attack aim.
7. Play the 24-frame attack as a single non-looping action, then return to movement or idle. Proposed staging: anticipation 0–9, active strike 10–12, recovery 13–23. The final hit time must be calibrated against the actual blade pose, not assumed solely from the prompt.
8. Apply one damage event when elapsed time crosses the strike marker, including at low FPS; do not require landing on exactly one frame. Preserve existing player weapon hitbox improvements. Enemy contact boxes must not grow when artwork grows.
9. The expansion catalog proposes base HP 65 and raw primary damage 14 for this enemy. These are provisional. Use existing room-based health scaling once; avoid unintentionally applying legacy damage multipliers twice. It is a normal enemy and remains vulnerable to FREEDOM.
10. Test 30/60/120 FPS, all directions, slow effects, walls, attacks, hurt interruptions, death and transition cleanup. Finish missing idle/hurt/death states before enabling it in normal gameplay. Package only after visual and combat review.

## Tools

Artwork was generated with the built-in image tool. inspect_atlases.py reads images to measure alpha and crop bounds; it does not paint or modify PNGs. The preview draws source frames using those rectangles. Keeping prompts, sources and corrections together lets the same artwork be used at home.
