# Iron Matriarch — revised spacing review

Open preview.html to review 21 sheets and 216 pose slots. Select a sequence, play/pause, change FPS, or scrub individual frames. Original sources remain in ../sprites-v1.

Thirteen crowded sheets were revised using image generation to provide more wing, tail, and effect clearance. The front slam received an additional reduction pass. Eight other sheets retain their original artwork. Measured frame rectangles in manifest.json replace equal-grid preview crops, which previously cut into neighboring poses.

All 216 measured rectangles pass the alpha boundary check described in UE5_HANDOFF.md. Preview playback, pause, scrub, restart and final-frame hold pass a JavaScript logic test across all 21 sheets. This is not a rendered browser or UE5 test. QA.json retains the separate equal-grid diagnostic; equal-grid warnings are the reason to use the measured rectangles.

Final foot registration, scale matching across sequences, anatomy/pose continuity, attack timing and UE5 import still require in-engine review. This ArtSource pack does not change the official gameplay build. Start with HOME_PC_HANDOFF.md on the home PC.
