# Graveglass Slinger review notes

This is the first walk/attack/effects draft, not a completed gameplay enemy. Read the current atlas manifest for actual inventory and technical flags. The preview is an offline source-frame player; UE5 and browser visual playback have not been tested here.

Source-sheet inspection: N and S walks face the intended direction. NE/E and W/NW separation needs refinement; some diagonal views are too close to profile. Torso, hood, urn size and backpack positioning vary across sheets. Several walk poses repeat the same leading leg; do not equate 16 cells with a polished gait.

The original N attack moved the sling between hands. attack-N-v2.png corrects its opening grip. E/SE attack loading and SE followthrough still need close hand review; SE can show an extra sling during the strike. SW has a mid-windup pose that returns to ready too early and a followthrough that changes the visible arm. Resolve these before integrating. Timing, loaded urn visibility and release consistency need cleanup in all attacks.

Frame rectangles are measured from alpha. For character sheets with the expected connected-pose count, the preview uses individual padded pose bounds rather than a fixed grid. This avoids adjacent-pose fragments, but does not prove all detached small effects or sling details were captured. Otherwise the preview uses measured gutters and flags risky boundaries. Review every source crop before importing. Projectile effect cells may contain multiple independent shards and require a different extraction method.

Idle, hurt and death states are not produced in this pass. Character health and damage are provisional. This is the source-art repository upload; UE5 implementation remains pending.

Additional review: attack-SE-v2 removes the duplicated sling but grip/release timing remains imperfect. The NW correction attempt still swaps hands and is retained only as an unused alternate; original NW is not production-approved either. W attack also requires grip review. Projectile-v2 restores the intact urn to the last travel cell. Impact effects are separate hero/environment sequences; they have no damaging residual zone.

North throw cleanup: uses attack-N-clean-v4.png,20 authored poses and4 explicit final recovery holds for24 playback frames. Clean transparent source replaces brown-halo variant. Per-row silhouette crop metadata (clipRows) excludes neighboring-pose fragments where bounding rectangles overlap; preview applies these clips. HOME-PC EXTRACTION MUST honor clipRows as a sprite mask, not simply rectangular crops. Row-span overlap check passed for all20 poses. PNG remains an atlas; clipping instructions are part of the required extraction metadata.
