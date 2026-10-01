# Review status — first animation draft

The selected inventory contains 16 transparent PNG sheets with 320 non-empty pose cells: 128 walk and 192 attack. This counts cells, not independently approved unique animation frames. No UE5 or browser visual playback test has been completed on this machine.

All eight selected walk sheets have no measured edge-risk cells. Attack crop warnings remain (one-based pose numbers): E 11, 12, 14, 15; SE 15; S 5, 8; SW 5, 6, 7, 8; NW 6, 8, 10. These require checking actual blade bounds and separation from neighboring cells before extraction. W, N and NE attacks have no measured edge warnings. Alpha-based checks cannot certify animation quality.

Visual review of source sheets found inconsistent sword length, repeated or near-repeated poses, and hand/grip changes, especially during SW and NW attacks. Several sequences have an additional raised-sword pose during recovery. Correct these before production. Walking requires playback review for alternating legs and a clean loop. Additional frames alone do not guarantee smoother movement.

Revised N/NE walking sheets improve sword margins; revised SW walking faces left. Revised attacks retain their intended directional starting/ending stance more consistently than the first drafts. These revisions remain drafts.

The preview displays selected source frames directly, including imperfections, and offers pause, frame stepping, action selection, speed and optional alignment. The package includes source PNGs, measured crop metadata, original/correction prompts and UE5 handoff instructions. Earlier rejected sheets remain in the local work folder but are excluded from the review ZIP.

Still pending: final pixel/anatomy cleanup, consistent pivots, idle/hurt/death, and animations plus applicable projectile/impact effects for the other 34 enemy concepts. This is a source-art upload; game modification and UE5 validation remain pending.

Playback fix: preview.html and preview-playback-fix.html now embed all 16 PNG sheets, show image loading/error status, and use timer-driven pose updates. A minimal-DOM execution check verified advancing poses, pause/resume, frame stepping, scrubbing and action selection. This is not a browser visual test. Rebuild using build_preview.py; inspect_atlases.py now calls it automatically.

Right-hand correction: replaced SW/NW walk and attack sheets. Visual inspection of selected sheets shows the sword in anatomical right hand and left hand empty throughout. Attacks use a simpler one-handed overhead motion to avoid the prior hand swaps. Revised frame crops account for uneven row spacing; remaining crop flags are recorded in atlas-manifest.json. Existing broader animation-polish limitations remain. Built-in image generation was used; prompts saved in right-hand-correction-prompts.json and right-hand-followup-prompts.json.

SW crop correction: measured the 16 walking and 24 attack pose outlines independently; replaced equal-column rectangles with padded individual pose bounds. Verified every SW crop excludes the bounding rectangles of all other poses. Preserved column anchors to avoid crop-induced horizontal shifts. Artwork PNGs were not modified. Browser visual review is still pending.
