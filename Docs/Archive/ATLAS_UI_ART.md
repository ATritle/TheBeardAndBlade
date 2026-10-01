# Emerald Atlas UI artwork (historical development notes)

Created using the built-in image-generation tool, inspired by the user's parchment-map reference. Source PNGs preserve transparent alpha. UE uses bilinear UI sampling to keep the large source art readable at 48/70-pixel icon sizes and a compact title. No controls or navigation logic changed.

Sources: `ArtSource/Exploration/AtlasMapIcon.png`, `ArtSource/Exploration/AtlasTitle.png`.
Imported: `Content/Art/V2/AtlasMapIcon.uasset`, `Content/Art/V2/AtlasTitle.uasset`.
Import script: `Tools/import_atlas_ui.py`.

## Icon prompt

Create one original polished pixel-art fantasy RPG inventory map icon: unrolled golden parchment treasure map, tall curled left edge and rolled lower corners, dark brown grid markings, bold crimson dashed winding route ending at a red X in upper right. Tiny emerald green clasp accent. Readable silhouette at 48 pixels. Inspired by a classic rolled treasure map, not a flat folded map. Warm gold highlights, dark brown outline, crisp detailed pixel clusters. Isolated centered icon fills 90% of square canvas. Genuine transparent alpha background, no backdrop, no text, no watermark, no drop shadow outside silhouette.

## Title prompt

Create a production fantasy RPG UI title graphic. Exact text: "EMERALD ATLAS" in a SINGLE horizontal line, elegantly crafted embossed antique gold serif capitals with dark brown bevel/shadow, restrained emerald jewel accents and fine golden end flourishes. Clean readable lettering for a dungeon exploration map in The Beard and Blade; polished pixel-art compatible detailing, no chunky illegible pixels. Text must dominate, minimal ornament, no enclosing panel/banner. Very wide composition 3:1 canvas, title centered occupies middle horizontal band about 45 percent of height and 92 percent width. Genuine transparent alpha background around letters and ornaments, no checkerboard painted background, no extra words, no watermark.
