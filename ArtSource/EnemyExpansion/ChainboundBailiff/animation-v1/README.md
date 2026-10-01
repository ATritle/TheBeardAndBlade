# Chainbound Bailiff — walking and chain-sweep source draft

Third Forgotten Keep enemy concept. A broad undead jailer in a vertical-bar iron cage helmet, dark iron armor, burgundy cloth and torso chains, carrying one short-chain spiked mace in the anatomical right hand. Original concept: third figure on the Forgotten Keep concept board. Built-in image generation creates the source PNGs; prompts are preserved alongside them.

## Inventory and preview

Generated first pass: eight directions, 16 walking poses per direction and 24 attack poses per direction. Each attack is authored in two 12-pose sheets: attack-a anticipates and swings into the strike; attack-b follows through and returns to ready. The preview joins those halves and plays source cells directly. Consult atlas-manifest.json for actual files and flags; planned counts are not proof of successful unique poses. No projectile is required for this melee enemy. Idle/hurt/death states are still pending.

Transparent PNGs and frame metadata are source artwork, not UE5 runtime assets. The preview checks animation without changing the existing game. Individual crops, stable pivots, grip continuity, cage proportions, mace size/chain length and the seam between attack halves require review. Brown halos, cropped neighboring poses and weapon hand swaps are defects to resolve, not intentional effects.

## Home-PC UE5 implementation

1. Preserve this design and use these source sheets; do not recreate the enemy in another style. Inspect the full sheets and step through all directions. Fix inconsistent anatomy, right-hand grip, chain connectivity, leading-leg alternation and silhouette scale. Review measured rectangles before extraction and remove any neighboring-pose pixels. Where supplied, clipRows is per-row silhouette crop metadata relative to cell and must be applied as a mask. Avoid uniform-grid slicing unless verified.
2. Normalize approved sprites to fixed transparent canvases and consistent body/ground anchors. Keep weapon extent from resizing the body between frames. Import under /Game/Art/EnemyExpansion/ChainboundBailiff/ using the existing adventurer/pixel-art import conventions, nearest filtering and the project's established mip/compression settings.
3. Inspect current roster IDs, renderer dispatch and spawn progression before allocating a new unused stable ID. Do not renumber existing enemies or rely on broad numeric ranges. Drive walking from distance traveled; halt gait when blocked. Use eight authored facings, not mirrored handedness.
4. Provisional Medium tier: base HP 110, raw melee damage 20 before armor. Apply the current health/room scaling once. No passive contact damage. Tune alongside the current game's updated enemy health and player reach.
5. Attack: plant feet, readable 0.9-second windup, short forward chain sweep, then exposed 1.2-second recovery. Lock aim at windup. Calibrate strike timing against the final frames instead of blindly using the original catalog's frame 10. Join attack-a and attack-b at a compatible strike pose; remove duplicate impact events at the seam. Blend timings by holding approved poses if needed, not by changing grip.
6. Mace is attached melee equipment, NOT a projectile. Use a swept arc/segment or the existing robust melee shape between previous/current strike positions, bounded by configured reach. Damage the player at most once per attack. Clear hit tracking at each new attack, suppress damage during recovery, and cancel pending strike on death/stagger. Sprite size does not set collider size. Leave a clear dodge opportunity and do not allow the chain to damage through blocking walls.
7. This is a standard enemy, not a boss: FREEDOM executes only if already below 25% maximum HP, otherwise removes 75% current HP. Preserve the existing bosses' immunity.
8. Finish idle, hurt, death and interruption cleanup before enabling normal dungeon spawns. Test all facings, walls, dodge, player weapon reach, attack cancellation, variable frame rates 30/60/120, and existing save/hero/boss behavior. Package after visual and gameplay review; report remaining issues honestly.

The first pass remains a review draft until art and in-engine behavior are validated. No generated-source sheet alone proves smooth animation.

## Exact reviewed playback selection

The 24 selected sheets contain 128 walking and 192 attack source cells. Playback uses 128 walking and 183 selected attack cells, with nine repeated recovery holds. These counts do not imply 320 unique polished animation poses. North uses attack-a poses 1–8, attack-b poses 1–12, then four recovery holds. Northwest uses attack-a poses 1–11, attack-b poses 5–12, then five recovery holds. Other directions use all 24 attack cells. Copy the ordered manifest entries rather than blindly concatenating the full sheets.

North attack-a poses 9–12 would replay recovery before the second half; they are excluded. Northwest attack-a pose 12 and attack-b poses 1–4 have incorrect weapon handedness and are excluded, although retained in the source sheets for correction. Shorter curated sequences still need timing and transition polish. The original N, W and NW attack-a sheets are rejected revisions and excluded from the ZIP.
