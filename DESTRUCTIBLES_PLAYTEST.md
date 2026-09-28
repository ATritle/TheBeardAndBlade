# Dungeon destructibles — local review

Each playable room has 2–3 randomized breakables in spaced side-wall positions, outside gate/chest paths. Three silhouettes per theme: crate, barrel and urn. Themes: treasury oak/brass, Big Mack kitchen supplies, military olive containers, Webroot moss/webs, Rime frost, Cinder scorched embers, Twister storm-blue containers. The Atlas trader branch opens the shop directly and has no breakables.

Strike with melee to break a prop in one hit. Each prop rolls once: 50% random loot, 50% nothing (tunable via `DungeonCombatBalance::PropLootChance`). Empty props still crumble and fade, without a pickup prompt or invisible item. Sixteen matching textured fragments burst, fall, settle and fade over 1.35 seconds. Loot becomes available after 0.45 seconds. E collects the closest drop within 85 screen units; full bags leave the exact rolled item in place. Drops use standard chest rarity odds and current room level, including rings. No auto-equipping, stat card overlay, healing, kill charge or boss-guaranteed legendary reward. Props are non-blocking and don't affect room-clear objectives or exit locks.

Props/drops persist between waves, reset on new rooms/restarts, and cannot be rerolled by repeat attacks. Uncollected drops are abandoned on departure.

## Validation

- `-game -BreakablesVerify -nullrhi`: all seven chapters, placement, eight-direction melee reach, one-time rolls, full bags, exact pickup, no duplicate grants, wave preservation, room reset, exit independence.
- `Tools/capture_review.ps1 -Preview Breakables -Biome N`: review intact props on left, fragment animation and settled loot on right. N is campaign chapter index 0–6.
- In UE: strike all three types, wait for debris to vanish, collect with E, and inspect only inside inventory. Check each theme at gameplay scale.

## Art provenance / import

Built-in image generation (not CLI) created `ArtSource/Destructibles/DestructiblesAtlas.png`; `Tools/import_destructibles.py` imports `/Game/Art/V2/DestructiblesAtlas` with nearest filtering, no mipmaps, never-stream, and alpha-preserving icon compression. Atlas uses measured column UV windows; runtime fragments preserve the original alpha and artwork.

Prompt: Create one transparent production pixel-art sprite atlas, 3 columns (crate, barrel, ceramic urn) by 7 rows in campaign order: treasury oak/brass; burger kitchen food crate/pickle barrel/mustard crock; military olive supply/fuel containers; haunted roots/moss/spiderwebs; icy frost-blue; charred ember-red; electrical storm-blue. Same slightly top-down front view, crisp detailed pixel clusters and dark outlines, no text/grid/characters/floor/shadows, isolated intact props with padding. Intended canvas 1536×1792; delivered image 1161×1354. Crumbling uses sixteen animated texture fragments per prop rather than alternate painted frame sheets.

No release, GitHub push or streaming package requested for this review.
