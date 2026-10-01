# Candle Hexer review notes

Status: first generated hover/cast/FX source draft. UE5 is not installed here; runtime integration, combat behavior and packaged-game testing remain pending. No game code or existing release was changed.

All generated directional and FX sheets were visually inspected. West attack-a initially lost the staff grip in late casting poses; a targeted revision restores a grasping hand and is selected by the manifest. Full anatomical consistency is still a review task, especially the far-arm connection in profile views.

Known polish work: NE/NW hood views can read too similarly; preserve the intended movement direction when finalizing these. E/W profiles lean toward three-quarter views. Candle count/placement, robe proportions and palm-spark intensity vary slightly. Some SE cast-half transitions retract the arm abruptly. S returns toward its ready posture early. Some movement/recovery poses differ only subtly, so cell count is not proof of equally smooth motion. Do not ship the sequences without timing and pivot review.

Some faint violet or gray pixels surround flames and hands in the source. The preview uses measured pose masks, but masks do not replace an art pass. Inspect on both light and dark dungeon backgrounds and retain intentional small flames while removing unwanted fringe. Background-bearing turnaround.png is reference only.

Ground-impact source poses 1–2 show an incoming comet and are excluded from impact playback; use poses 3–16. Travel loops, but both impacts should play once and disappear. The preview loops effects only to aid review.

Read crop-check.json for measured source counts and crop warnings. Non-empty cells, alpha transparency and non-intersecting masks do not establish correct anatomy, timing or style. Preview script checks use a minimal DOM, not visual browser validation. Final quality must be checked in UE5.


NW perspective correction: use walk-NW-occlusion.png, attack-a-NW-hidden.png and attack-b-NW-occlusion.png as selected by the manifest. The left hand grips the candle wand; the far-side right hand is fully occluded by the robe in this view. Do not restore the original rear-protruding hand or rear palm spark. Cast release still uses a separate projectile/event; the hand itself is hidden at this angle. Correction prompts are in nw-correction-prompts.json.

