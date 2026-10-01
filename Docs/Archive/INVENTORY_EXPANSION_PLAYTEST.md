# Inventory and ring expansion — local review (historical development notes)

Open TheBeardAndBlade.uproject in UE 5.8 and Play. Press I to open the inventory.
No GitHub push, release packaging, or StreamPixel replacement has been performed.

## Included

- Full-body approved-style adventurer portrait, equipment on the left, 6×6 bag in the middle/right, item card at the far right.
- One active ring slot; head/hands/legs/feet are disabled placeholders.
- Hover or select to inspect. Drag to move/equip; double-click a bag item to equip. Double-click an equipped item to automatically stow it, or drag it into an empty bag footprint. Invalid and overlapping drops are rejected without losing equipment.
- Satchel graphic replaces the heading; helmet/gloves/pants/boots silhouettes mark disabled future slots. No status message or unequip buttons. Equipped comparisons are inside the bottom of the item card.
- Item cards now use smaller, antialiased runtime typography with measured wrapping and inset margins. The empty card is intentionally blank. Decorative button lettering remains unchanged.
- Hover over the central adventurer portrait to show combined live equipment stats in the right-hand card. Moving off restores the selected/hovered item card; dragging suppresses the portrait panel. Health/stamina, recovery, hit range, attack rate, pre-critical DPS, critical chance/multiplier, armor, damage reduction, bleed/poison chances and leech use the hero's capped combat values. Unique conditional signatures are listed separately, without double-counting duplicate effects.
- UI capture: add `-InventoryStatsPreview` to the existing `-DungeonCapture -DungeonInventoryPreview` launch to equip sample gear and place the cursor over the adventurer.
- Illustrated cards appear only inside inventory for bag/equipped items: rarity/name/level, icon, primary stats, purple modifiers, green special effects, gold value. Opening/collecting a chest never overlays an item card on gameplay.
- Opening a chest immediately unlocks the exits. Its reward stays on the floor until explicitly collected with E; a full bag does not block progression. Leaving the room abandons uncollected loot. Verified all three exits with uncollected loot, including leaving during the opening animation, plus the full-bag ice-room case.
- Sixty catalog items. IDs 48–59 are twelve illustrated ring designs; all five rarity tiers can occur. Every chest can roll a ring (12 of 60 catalog definitions). Boss rewards remain legendary.
- Values are stable on the rolled item instance. Discarding now credits the run's coin wallet, and trader purchases use that balance; see TRADER_PLAYTEST.md.

## Ring tuning

| Ring pair | Primary modifier, before rarity multiplier |
|---|---|
| Thornblood / Crimson Briar | 4% bleed chance |
| Serpent's Coil / Venomwell | 4% poison chance |
| Skyrunner / Gale Knot | 12% maximum stamina |
| Vampire's Vow / Nightfeast | 2% damage leech |
| Iron Oath / Black Bastion | 5% damage reduction |
| Amberheart / Dawnheart | 10% maximum health |

Primary roll = base × random 0.9–1.1 × (1 + 0.22 × rarity index). Higher rarities also roll up to three distinct bonus affixes.
Bleed lasts 4 seconds; poison lasts 6 seconds. Rings can enable those procs without a matching weapon and add to matching weapon proc chance (35% cap).
Ring leech applies to actual damage, including tea/ailments, cannot heal from overkill or revive a dead hero. Existing weapon leech remains its separate direct-hit effect.
Health/stamina rings raise capacity but never refill it on equip. Mitigation multiplies damage after the existing armor calculation; scripted percentage-health boss moves are unchanged.

## Local checks

- Unreal Editor Development target rebuilt successfully.
- DungeonVerify passed (0 errors): expanded catalog, ring stat caps/removal, actual-damage leech, chest pool coverage, full campaign flow.
- DungeonLootSmoke passed (0 errors): ring drag/drop and double-click, overlap rejection, all 60 item textures, hovered cards, temporary ailment rendering/expiry.
- WeekendVerify and SeptemberVerify passed (0 errors each): combat-balance and chest/boss-flow regression checks.
- Visual captures: ArtSource/InventoryExpansion/Review-9.png (ring), Review-4.png (weapon), Review-5.png (armor).

Art pipeline and source prompts: Tools/INVENTORY_ART_PROMPTS.md. Generated sources stay under ArtSource/InventoryExpansion; imported runtime assets are in Content/Art/V2.
