# Visual review and remaining work

This is a generated source-art draft for review. No UE5 import or gameplay test has been run. The source sheets have been visually inspected; preview controls receive a minimal-DOM script check, not a rendered browser test.

## Selected corrections

- North walk uses `walk-N-back.png`: shield back and straps replace the incorrect decorative front.
- North attack A uses `attack-a-N-fixed.png`: removed an erroneous second spearhead. Its teal glow is weaker than neighboring sequences; unify this during final cleanup.
- NE attack A uses `attack-a-NE-fixed.png`: removed the duplicate shield.
- SW attack A uses `attack-a-SW-fixed.png`: restored the missing lance in source pose 8.
- W and NW attack A use the `-fixed` sheets: reduced incorrect near-side weapon arms. NW recovery uses `attack-b-NW-hidden.png` for the same occlusion correction.
- North attack B was generated ready-to-extended. The manifest plays its source poses in reverse order (12 through 1) so it serves as recovery. Do not import that half in raw reading order.

Rejected originals may remain in the working folder but are omitted from the selected ZIP/manifest. The opaque turnaround is reference only.

## Remaining visual cleanup

- Source cells are not all equally distinct. Several ready poses are very similar. More frames alone do not guarantee smoothness.
- N and NW silhouettes are close; N attack foreshortening and NW shield/weapon overlap still need final perspective review. Corrected far arms are deliberately largely hidden.
- Some transitions between the two attack sheets are abrupt, including body scale and spear angle changes. SE late recovery and S recovery return upright quickly. Use final pivot/scale alignment and redraw in-betweens where necessary.
- Several spear tips sit close to sheet edges. Initial source-border checks flagged S walk pose 4 and SW attack A/B pose 12. Do not interpret padding or crop masks as restoring missing pixels. Repair those edge details before runtime import.
- Faint gray/colored alpha fringes remain on some armor, cloth and spear-tip edges. Character row masks reduce neighbor contamination but may exclude detached faint pixels. Review original images on light and dark backgrounds and preserve intentional effects.
- The preview uses provisional direction-based scale and bottom alignment. UE5 needs stable shared root pivots, body scale and correctly padded cleaned sprites.
- Projectile and impact effects retain their own crop cells rather than character-component masks. Check impact collision-center alignment and fade timing separately. Last frames still contain flecks: fade/stop the one-shot effect in UE5. Preview repeats it for inspection only.

## Validation

`crop-check.json` records alpha range, nonempty cells, component counts, row-mask overlap and source-edge flags. These checks do not establish correct anatomy, seamless animation or gameplay behavior. Idle, hurt and death states remain unbuilt. Final combat tuning, collision, damage events and runtime performance require the home UE5 project.
