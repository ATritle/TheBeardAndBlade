# Full pose batch — working QA notes

All 25 turnaround references have been visually inspected. These boards are design references only, not final animation sheets. Greaseworks is excluded.

Known issues to resolve during source generation:
- Breach Hound N recovery and NE movement/attacks have selected corrected versions. Lifecycle clean-v2 sheets are selected, but hurt-SW still has brown haze and idle-E poses 4/8 touch the right source edge. Resolve before final handoff. Other lifecycle sheets still require visual review.
- Briar Spitter N/NE/NW movement now uses selected rear-v2 corrections with hidden mouth. Review scale consistency before animation.
- Trench Shivver NW and E, and Rivet Gunner NE and E use selected corrected walk sheets and updated dependent action references.
- Sporebell Witch SW/W grip-v2 correction candidates are saved but NOT selected: SW still appears to hold the staff with its far arm. W is improved, but needs a paired direction check. Do not propagate the original SW/W as approved anatomy.
- Silkfang Skitter movement has been viewed in all directions. Edge fringe/noise and leg count/contact continuity require cleanup; existence of 16 poses does not establish a smooth gait.
- Icicle Flinger must throw from anatomical right; reference S incorrectly suggests left.
- Permafrost Templar keeps anatomical right hand toward hammer head, left toward shaft butt.
- Sporebell Witch, Ashen Cantor and Thunderhead Adept keep staff in anatomical left hand. Some turnaround views switch sides.
- Cinder Pitcher keeps bucket in anatomical left, throws with right. Reference N incorrectly switches bucket side.
- Bellows Brute has the large anvil fist on anatomical right.
- Bunker Bulwark shield remains left, launcher over right shoulder.

Non-Breach action/lifecycle sheets reference the matching walk sheet; recovery sheets reference their attack windup sheet. Inspect those parents before using them. More source frames alone do not establish smooth animation. Check actual gait, feet contact, hand continuity, body scale, alpha edges, cropping and timing. Final UE5 import has not been tested here.

## Transparency inspection correction

The raw image inspection display can expose RGB color from transparent pixels. Sampled brown background pixels in Breach Hound hurt-SW-clean-v2 and hurt-SW-alpha-v3 have alpha=0. Bright red specks sampled in Thornweave Sentinel NW have alpha=1 or 2 of 255. These observations supersede earlier claims that the displayed brown/red backgrounds necessarily appear in-game. Do not redraw or delete character pixels solely based on raw RGB previews. Inspect proper alpha-composited output and edge behavior during final review. Keep clean-v2 selected unless a replacement has verified anatomical/crop improvements; new v3 candidates were not selected.


## Additional review checkpoint
- All eight Coalgnash walks viewed. NE turns its face toward camera during cycle; replacement rear-facing candidate is being generated before dependent action poses.
- All eight Icicle Flinger walks viewed. E, SE and NW have incorrect throwing-hand perspective; replacement grip candidates in production. W is acceptable (far right throwing hand, near left empty).
- All eight Permafrost Templar walks viewed. E, SE and NW need hammer grip correction; right hand must stay near hammer head, left at shaft butt.
- All eight Snowveil Oracle and Glacierback Ram walks viewed; references acceptable for phase-one source generation, with final motion QA pending.
- Alpha-zero hidden RGB is not a visible background defect. Do not regenerate sheets solely for appearance in the raw image viewer.

