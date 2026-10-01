# Home-PC task: Chainbound Bailiff

Integrate this reviewed source artwork into The Beard and Blade on the UE5 home PC. Read README.md, QA.md, atlas-manifest.json and crop-check.json first. Preserve the existing game and the Bailiff's original concept identity: iron cage helmet, burgundy cloth, torso chains and one chain mace in the anatomical right hand.

This pack is a first walking/attack draft. Do not assume it is approved for shipping. Step through every direction and correct visible grip, chain, silhouette, scale and timing inconsistencies before importing. Idle, hurt and death art still needs completion. North's original attack-a-N.png was rejected because of a smoky checkerboard strip; use the clean filename selected by the manifest.

Extract measured frame cells with their clipRows masks; simple uniform slicing can include neighboring pixels. Normalize fixed canvases and body/ground pivots, avoiding weapon-driven scaling. Join attack-a and attack-b as one attack with one damage event, and check the transition and loop seam. The browser's review playback speed is not a combat timing specification.

Implement a Medium standard enemy with provisional base health 110 and raw melee damage 20, using the current game's scaling exactly once. Give a readable 0.9-second windup, short chain sweep, and 1.2-second exposed recovery. Lock aim when winding up; use swept melee collision and once-per-swing hit tracking, obey blocking walls, cancel on death/stagger, and do not add passive contact damage. No projectile is required.

Inspect current enemy IDs, spawn rules and renderer dispatch before adding a stable unused ID. Keep all existing IDs and save compatibility. Use all eight authored facings without mirroring weapon handedness. Preserve the current FREEDOM rule for normal enemies and boss immunity.

Verify animation, movement and attacks at 30/60/120 FPS; test dodge, player melee reach, walls, interruption, damage scaling and existing bosses/hero. Only enable normal spawning and package a new build after the missing states and visual corrections are complete. Report exactly what was changed and what remains unfinished.

Use the manifest playback order, including the N/NW exclusions and recovery holds, rather than all source cells. Re-author the five excluded NW poses if a full uninterrupted 24-pose attack is required; do not restore them with the hand swap.
