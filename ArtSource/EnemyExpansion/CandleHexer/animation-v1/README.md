# Candle Hexer — hover, spellcasting and wax-comet source pack

Fourth Forgotten Keep enemy concept. A floating burgundy-robed candle ghost with skeletal hands, ivory wax candles and violet flames. The concept reference is the fourth character on the Forgotten Keep concept board. This is generated source artwork for review, not a finished UE5 enemy.

## Included first pass

- Eight directional hovering-movement sheets, 16 source poses each: 128 cells.
- Eight directional spellcasting sequences, each split into two 12-pose sheets: 192 cells. Attack-a charges/releases; attack-b recovers.
- Wax-comet travel: 12 looping east-facing frames.
- Player-hit effect: 12 one-shot frames, effects only, no player baked in.
- Environment-hit effect: 16 source frames. The preview uses frames 3–16 (14 frames), excluding the first two incoming-projectile poses so impact starts on collision.
- Reference art, generation and correction prompts, measured crop data, preview, checks and home-PC task.

There are 320 character and 40 FX source cells. Counts describe source cells, not a guarantee of unique polished animation. Idle, hurt and death states remain unfinished. The turnaround is a design reference with a background; do not import it as a transparent runtime sheet.

Open preview.html to review hover, cast, projectile and both impacts. The embedded-image local preview can run offline. Pause, step and scrub all views, including the transition between casting halves. The browser is a visual review only; damage, homing, collision and UE5 integration are not implemented by it.

## Sprite extraction and import

Use atlas-manifest.json `entries` for actual playback order, and `sourceFile` / `sourcePose` for the source image and pose. A frame `cell` is [x,y,width,height]. `clipRows`, when present, gives [localY,localX,width] spans relative to that cell; apply those as a mask to avoid neighboring-pose fragments. Character masks use measured connected silhouettes. FX uses measured gutters to retain detached wax chips and sparks. Review faint flame edges and any detached details before final export.

Normalize approved sprites to a fixed transparent canvas with stable body/hover anchors and consistent scale. Do not let candle-flame height or hand extension determine the character's size. The preview crop pivots are provisional. Preserve flame flicker and cloth motion while avoiding whole-body jitter. Match the current adventurer/pixel-art texture import settings and nearest filtering under /Game/Art/EnemyExpansion/CandleHexer/. Never mirror asymmetric equipment to supply a missing direction.

Design invariant: one candle staff in anatomical left hand; anatomical right hand casts. West attack-a uses attack-a-W-grip.png; the earlier attack-a-W.png is rejected. Inspect shoulder/arm connections and staff grip, not just which screen side a hand appears on. Rear-diagonal facings and casting-half seams still need art review; see QA.md.

## Provisional combat profile

Medium standard enemy, base HP 76, raw spell damage 15 before armor. Apply the current game health/room scaling once. Do not apply the legacy extra 1.8 damage multiplier to this new profile. No passive contact damage. Spawn exactly one projectile per accepted cast and cancel an unreleased cast on death/stagger. Proposed windup 0.9 seconds and recovery 1.2 seconds; align the release event to the final accepted poses rather than assuming the catalog's original frame number.

Wax comet speed: 230 logical pixels/second; maximum lifetime 2.6 seconds. Initial tracking lasts at most 0.65 seconds, turns at most 60 degrees/second and stops within 100 logical pixels of the target. A successful dodge permanently breaks tracking for that projectile; it continues straight and never reacquires. Convert logical units to the existing game's units consistently. These values require playtesting.

Rotate the east-facing travel sprite to actual velocity. Place its pivot near the bright wax core, not the tail; its art extent is not its collision radius. Use swept collision against the player hurt shape and blocking environment. Resolve the earliest accepted hit exactly once. Player hit plays the player-impact effect at the hit point; environment hit plays the 14 selected environment frames. Remove the traveling projectile immediately on resolution. Lifetime expiry dissipates harmlessly without damage. No double direct/splash hit, poison, slow, lingering aura or damaging wax puddle is added by this profile. Impact effects are anchored in world space and do not follow the player.

As a standard enemy, Candle Hexer is affected by FREEDOM: execute if already below 25% maximum health; otherwise remove 75% current health. Preserve boss immunity.

Before normal spawning, complete idle/hurt/death, inspect current roster IDs and renderer dispatch, choose an unused stable ID without renumbering anything, and finish in-engine visual/gameplay checks. Keep all existing saves, hero art, boss behavior and release builds intact.


NW perspective correction: use walk-NW-occlusion.png, attack-a-NW-hidden.png and attack-b-NW-occlusion.png as selected by the manifest. The left hand grips the candle wand; the far-side right hand is fully occluded by the robe in this view. Do not restore the original rear-protruding hand or rear palm spark. Cast release still uses a separate projectile/event; the hand itself is hidden at this angle. Correction prompts are in nw-correction-prompts.json.

