# Chainbound Bailiff review status

This is a generated source-art draft, not a completed UE5 enemy. The original Forgotten Keep concept and directional reference define the identity; sheet grids do not prove correct poses. Source PNGs are produced with the built-in image tool. No engine or packaged-game changes are made here.

The preview uses measured pose crops when a connected-component count matches the requested count. Chain/mace silhouettes can contain detached pieces or overlap; these require visual checking. When component counts do not match, gutter crops are used and crop risks are reported. Do not assume every non-empty cell is a complete, correct or unique frame.

Attack-a and attack-b are separate authored halves joined for review. Their seam, body scale, mace-ball size, chain length, right-hand grip and facing must match before integration. No attack is considered production-approved simply because two halves play sequentially. Preserve the same anatomical right hand, not the same screen side when the character turns.

Idle, hurt and death remain pending. Stats and attack timings are provisional. UE5 validation and browser visual playback testing have not been performed on this work PC. Refer to atlas-manifest.json for actual inventory and all crop flags.

## Review results

All 24 selected source sheets were visually inspected. Alpha ranges include 0 and 255. Each walking sheet has 16 measured connected silhouettes and each attack half has 12. Read-only crop checks found no empty cells, intersecting per-row pose masks, or silhouette pixels on the outer image boundary. These checks cannot certify art consistency.

North smoke/checkerboard contamination was removed using image generation. West late-strike handedness was corrected. Northwest still has five rejected hand-swap poses, excluded from playback as documented in README. North uses four recovery holds and northwest five; holds are not additional authored artwork.

Remaining art review: some walks have subtle/repetitive strides, front/diagonal facings can look similar, cage proportions and chain length vary slightly, and transitions between attack halves and walk/attack scales need final polish. North and northwest use curated shorter attack sequences with held recovery frames. Do not ship raw full-sheet sequences.

Preview logic is tested with a minimal DOM for image loading, action selection, advancing frames, pause, stepping, scrubbing and attack-half source mapping. This is not visual browser testing or UE5 verification.
