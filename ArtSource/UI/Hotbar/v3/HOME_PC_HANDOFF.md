# Home PC handoff: lower-center hotbar HUD
Date: 2026-09-30
Repository: ATritle/TheBeardAndBlade
Design authority: hotbar-concept-v3.png in this folder supersedes hotbar-v1/v2.
Status: approved-direction visual concept and implementation brief only. No UE code or assets changed here. Repository location: ArtSource/UI/Hotbar/v3/.

## Task to continue
Start from the latest home-PC checkout, preserve newer gameplay work, inspect the existing HUD/input/ability code and implement this replacement in the official UE5 game. Use the pictured design and existing pixel-art conventions. Locate actual classes/widgets before editing; do not assume filenames from an older build.

## Presentation and art preparation
- Anchor the complete HUD lower-center with safe margins and responsive DPI scaling. The preview is enlarged for review; size the runtime HUD to leave combat visible.
- Keep blackened iron, aged gold, emerald ornament and the six-slot layout.
- Left red health orb, BLUE stamina arc above and around its upper-left edge, gold lightning icon on dark-blue disk.
- Separate GREEN block ring and shield at the far right. Keep RMB and BLOCK labels. Do not include the removed static "5s HOLD / 3s COOLDOWN" caption.
- Preserve bald, glasses-wearing, red/ginger-bearded adventurer with green cloak and metal shoulder armor in the LMB sword-swing icon.
- This PNG is a flattened visual mockup including a dungeon background, baked full meters and sample 100/100. It is NOT a production-ready transparent HUD atlas. Create separate transparent frames, icons, fill masks and ring layers; exclude the pictured dungeon floor from the actual overlay. Render key labels and values dynamically. Do not overlay this whole screenshot in gameplay.
- Use existing texture filtering/material conventions for sharp pixel art. Keep decorative frame layers separate from fills, countdowns and highlights.

## Live bindings: left to right
| Control | Function | HUD behavior |
|---|---|---|
| 1 | FREEDOM | Eagle icon; show real charge/readiness and any actual cooldown |
| 2 | Reserved | Empty, inactive socket; no invented action or timer |
| 3 | Reserved | Empty, inactive socket; no invented action or timer |
| 4 | Reserved | Empty, inactive socket; no invented action or timer |
| LMB | Normal sword attack | Correct adventurer icon; real attack recovery/availability |
| MMB | TEA | Teacup icon; real cooldown and ready pulse |
| RMB | Block | Separate shield indicator with duration/cooldown ring |

Replace conflicting old mappings: MMB must no longer activate FREEDOM, RMB must no longer activate TEA. Update gameplay bindings and associated input hints/tutorials; preserve movement, dodge, sprint, inventory and menu controls. UI should not steal combat input or allow mouse-wheel scrolling to trigger MMB.

## Meter and cooldown behavior
- Health orb reads current/max health. Replace baked 100/100 with real values; clamp fill safely, including zero/changed max. Animate actual health changes without delaying gameplay state.
- Blue stamina arc reads current/max stamina and follows existing spending/regeneration rules. Do not create a separate HUD stamina pool.
- Each usable action displays a countdown meter during its real cooldown/recovery, based on gameplay state, and becomes ready when gameplay says so. Use a radial sweep/dim overlay and remaining seconds where legible. Do not invent durations for TEA, FREEDOM or LMB.
- FREEDOM may be kill-charge based: show its actual progress (e.g. current kills / required kills) rather than presenting charges as elapsed seconds. Preserve the configured threshold and existing damage rules. If there is also a timed cooldown, display that separately from charge state.
- Reserved slots have no cooldown display. A normal attack should show its real recovery/lockout if present, not gain an arbitrary new cooldown.
- Readiness must account for resource requirements, death, stun, menus and other existing gameplay restrictions, not just a zero timer.
- Bind to authoritative player/ability state and events (or project-equivalent polling); avoid independent cosmetic timers that drift from actual availability. Clean up/rebind on respawn, level transition and player replacement.

## Block: confirmed requirements
- RMB holds block for at most FIVE seconds.
- At the limit, end block and disable it for THREE seconds.
- Ready: full green ring, clear shield.
- Blocking: remaining-duration ring drains from full to empty over five seconds.
- Cooldown: dim shield, show remaining seconds from 3 to 0 and a distinct refill/sweep. Return to ready at the actual gameplay end time.
- Ring segments in the artwork are decorative, not an instruction to change the duration.
- Before finalizing behavior, resolve what happens on early release: whether every release triggers the full cooldown, and whether unused duration persists. Also resolve continuous held-input behavior when cooldown ends (new press versus automatic restart). Do not silently implement a bypass through repeated short taps.
- Block mitigation amount, direction/coverage, stamina interaction, movement restrictions and interruption behavior are not specified by this artwork request. Preserve any established implementation or ask the user only for genuinely missing gameplay decisions.

## Replace existing HUD safely
Retire the old health/stamina/action HUD rendering once the new version works, including duplicate plain-text input hints. Preserve unrelated HUD functions such as boss health, dialogue, objectives, loot, inventory and damage/status feedback unless they are explicitly replaced. Do not delete art/code still referenced by other screens. Keep changes scoped and reviewable.

## Verification on home PC
1. Compile and run the actual UE5 build.
2. Confirm precisely one player hotbar; check supported resolutions and window sizes for clipping/overlap.
3. Verify health loss/healing/max changes, stamina spending/regeneration, and death/respawn.
4. Test all new bindings, reserved-key inactivity, and removal of conflicting old bindings.
5. Verify ability charge, cooldown countdown, ready pulses and inability to activate unavailable abilities.
6. Measure block ending at five seconds and reavailability after three seconds; test early release, re-press/held input and interrupted states according to the resolved rules.
7. Check pause, dungeon transitions and input focus; preserve established time-dilation/pause semantics.
8. Review in-game screenshots and timing behavior. Do not call this implemented based only on the mockup.


