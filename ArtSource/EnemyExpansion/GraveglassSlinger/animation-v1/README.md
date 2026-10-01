# Graveglass Slinger — animation source draft

Second enemy in the Forgotten Keep expansion. Reference: the existing Forgotten Keep concept board, second character. Identity: hunched undead grave robber, iron half-mask, burgundy hood/cape, skeletal limbs, leather harness and cracked teal-glowing urns. Artwork generated with the built-in image tool; prompts are retained. No UE5 installation or gameplay changes were used.

## Actual inventory

Consult atlas-manifest.json for files present; prompts describe intended outputs, not proof of completion. Inventory:128 walking poses,188 authored throwing poses plus4 explicit north recovery holds,12 urn flight poses,12 hero-impact poses and16 environment-impact poses. North uses attack-N-clean-v4.png and requires clipRows masking; see HOME_PC_TASK.md. Idle, hurt and death sets remain pending. Generated cells are not proof of unique poses, correct handedness or smooth motion.

preview.html loads the sibling PNG sheets; open it in Edge or Chrome after pulling the full folder. Run build_review.py with Python/Pillow to rebuild an embedded-image preview. It plays actual pose cells, not interpolated whole-image movement. build_review.py measures source rectangles and rebuilds the preview without editing image pixels. Use frame stepping to inspect hands, backpack urns, direction, crop edges and stance transitions. Marked crop boundaries need manual review; do not import blindly as a uniform grid.

## Home-PC UE5 work

1. Preserve this character design and use these PNGs. Review visual consistency, true transparency, cell counts, alternating gait, right-hand sling grip and source crop boundaries. Clean up incorrect poses and align stable body/ground pivots on uniform transparent canvases before import. Keep scale consistent; nearest-neighbor filtering and current game's pixel-art texture settings.
2. Import additively under /Game/Art/EnemyExpansion/GraveglassSlinger/. Verify current enemy IDs and render dispatch; allocate an unused stable ID without renumbering existing characters or relying on broad numerical ranges. Use the current adventurer animation pipeline where appropriate.
3. Provisional Basic tier, base HP 48, raw primary damage 11 before armor. Apply current room-health scaling only once. No contact damage. Standard enemy: vulnerable to FREEDOM; execute when already below 25% max HP, otherwise lose 75% current HP. Preserve boss immunity.
4. Drive 16-pose walk from displacement. Face movement while walking. Lock attack direction at throw windup; nominal windup .65 seconds, recovery .9 seconds. Calibrate the actual release pose visually; emit exactly one projectile when elapsed time crosses release, including low FPS. Do not spawn one per animation update.
5. Urn launches toward the player's position at release and follows a ballistic-looking arc at provisional logical speed 280 pixels/second. Physical thrown urns do not steer after release. Dodge permits evasion. Maximum lifetime 2.6 seconds; lifetime expiry dissipates harmlessly.
6. Sweep collision from previous to current projectile position against world geometry and player hurt shape; resolve earliest accepted collision exactly once. Apply damage 11 only on a valid player hit. This urn has no area damage or status effect. Do not double-hit with visual shards.
7. Play hero-impact or environment-impact asset at collision point, destroy projectile, and remove all effect instances after playback. Environment shards are visual only, fade out and never form a damaging puddle. Urn flight is a 12-pose loop; impacts are nonlooping. Rotate authored travel artwork with flight direction only if appearance remains correct.
8. Finish idle, hurt, death and interruption behavior before adding to normal spawn pools. Test all directions, terrain, projectile impacts, dodge, slow, death during windup, frame-rate independence at 30/60/120 FPS and preservation of existing gameplay. Package only after review.

The other expansion enemy designs are separate work; this folder only delivers Graveglass Slinger sources.
