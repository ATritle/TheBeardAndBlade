# Equipment expansion — local review (historical development notes)

Twelve new items (catalog 60–71) activate head, hands, legs and feet. Existing catalog IDs remain unchanged. All four categories roll from chests, breakables, reward rooms and trader stock. Helmets/gloves/boots occupy one bag cell; legs occupy 1x2. Drag/drop and double-click equip/unequip work like other equipment. The adventurer portrait and combat costume are unchanged.

- Head: max stamina; three base strengths 10/12/14%.
- Hands: attack speed; three base strengths 7/9/11%.
- Legs: armor; base strengths 5/7/9, using existing level/rarity scaling.
- Feet: movement; base strengths 5/7/9%. Walking/sprinting gain speed, not dodge or gait cadence. Total movement bonus capped at 35%.

Percentage primaries use (1 + .012 x (level - 1)) x (1 + .18 x rarity) x a .9–1.1 roll. Rarity also grants the existing bonus affixes. Removing/swapping gear rebuilds stats and does not heal or refill stamina. New runs clear all eight slots.

Trader: 3–5 unique items, Rare minimum, first offer guaranteed Epic or Legendary; other offers 45% Rare / 45% Epic / 10% Legendary. Stock is two item levels above its previous floor level and remains persistent on return visits. Purchase price remains ceil(value x 1.2), so discarding cannot produce profit. New gear appears in the same pool as existing weapons/armor/amulets/rings.

## Art

Generated with the built-in image-generation tool, four generate-mode calls. Original outputs copied intact into ArtSource/Equipment/GearSheet_0.png through GearSheet_3.png. Each 2172x724 transparent sheet has three equal cells. Runtime UVs select each cell; no destructive cropping or external image editing. Import with Tools/import_equipment.py into /Game/Art/V2. UI compression, no mipmaps, never-stream, bilinear sampling match the existing clear inventory art. Packaged builds already cook this directory.

## Prompt set

### GearSheet_0

Use case: stylized-concept. Production pixel-art equipment sprite sheet for fantasy dungeon RPG The Beard and Blade. HEAD GEAR, exactly three different helmets: LEFT brown leather ranger hood with bronze brow band; CENTER steel open-face adventurer helmet with a small emerald crest; RIGHT ornate gold-and-silver winged endurance helm with a blue gem. No human heads or faces. Layout STRICT one horizontal row of THREE equal square cells on a wide 1536x512 canvas, cell centers at 1/6, 1/2, 5/6 of width. Each item centered within its cell, fills 78% of cell height, generous transparent separation so cells can be sliced cleanly. High quality detailed crisp pixel clusters, readable silhouettes at 64 pixels, rich material shading, medieval gold-and-emerald aesthetic. Genuine transparent alpha background across canvas. No cell frames, no labels, no text, no watermark, no characters, no scene. All three items fully visible, none overlaps another cell.

### GearSheet_1

Use case: stylized-concept. Production pixel-art equipment sprite sheet for fantasy dungeon RPG The Beard and Blade. HAND GEAR, exactly three pairs of gloves: LEFT supple dark leather duelist gloves with copper buckles; CENTER articulated silver steel gauntlets with green cloth cuffs; RIGHT black-and-gold swiftstrike gloves with amber knuckle studs. Each pair entirely within its own cell, no arms. Layout STRICT one horizontal row of THREE equal square cells on a wide 1536x512 canvas, cell centers at 1/6, 1/2, 5/6 of width. Each item centered within its cell, fills 78% of cell height, generous transparent separation so cells can be sliced cleanly. High quality detailed crisp pixel clusters, readable silhouettes at 64 pixels, rich material shading, medieval gold-and-emerald aesthetic. Genuine transparent alpha background across canvas. No cell frames, no labels, no text, no watermark, no characters, no scene. All three items fully visible, none overlaps another cell.

### GearSheet_2

Use case: stylized-concept. Production pixel-art equipment sprite sheet for fantasy dungeon RPG The Beard and Blade. LEG GEAR, exactly three leg armor pieces: LEFT brown reinforced leather trousers; CENTER silver plate leg armor over dark cloth with green belt; RIGHT ornate blackened gold-trimmed heavy armored trousers. Waist to ankles only, no feet or bodies. Layout STRICT one horizontal row of THREE equal square cells on a wide 1536x512 canvas, cell centers at 1/6, 1/2, 5/6 of width. Each item centered within its cell, fills 78% of cell height, generous transparent separation so cells can be sliced cleanly. High quality detailed crisp pixel clusters, readable silhouettes at 64 pixels, rich material shading, medieval gold-and-emerald aesthetic. Genuine transparent alpha background across canvas. No cell frames, no labels, no text, no watermark, no characters, no scene. All three items fully visible, none overlaps another cell.

### GearSheet_3

Use case: stylized-concept. Production pixel-art equipment sprite sheet for fantasy dungeon RPG The Beard and Blade. FOOT GEAR, exactly three pairs of boots: LEFT brown leather trailrunner boots with green ankle wraps; CENTER slim silver armored windstep boots with blue accents; RIGHT black-and-gold ornate stormstrider boots with small emerald ankle gems. Each pair entirely within its own cell, no legs beyond boot tops. Layout STRICT one horizontal row of THREE equal square cells on a wide 1536x512 canvas, cell centers at 1/6, 1/2, 5/6 of width. Each item centered within its cell, fills 78% of cell height, generous transparent separation so cells can be sliced cleanly. High quality detailed crisp pixel clusters, readable silhouettes at 64 pixels, rich material shading, medieval gold-and-emerald aesthetic. Genuine transparent alpha background across canvas. No cell frames, no labels, no text, no watermark, no characters, no scene. All three items fully visible, none overlaps another cell.

## Review

Use Play_Atlas_UE.cmd for normal play. Automated -GearReview captures all four item-card categories and trader stock, tests gear gestures, and exits. -DungeonVerify covers catalog identities, stat removal/reset, movement application, and 100 premium stock rolls; -AtlasVerify checks seven-floor shop persistence/levels; -TraderVerify covers purchases/full bags/discards.

Validated September 27, 2026: editor Development build succeeded; equipment import completed; DungeonVerify, AtlasVerify, TraderVerify and rendered GearReview completed with zero errors. All four card screenshots and premium trader list inspected at 1280x800. No game/editor process left running. This is a local UE review build, not a packaged release or GitHub push.
