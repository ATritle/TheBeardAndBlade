# Approved TEA visual rework — effects v4

Approved for GitHub upload on 2026-09-30. Open preview.html locally with adjacent files present; choose Throw → ground impact or Throw → enemy impact. Pause and scrub to review individual frames.

Includes 18 frames across three transparent PNG sheets: 8 tumbling cup poses, 6 newly drawn wide ground-splash poses, and 4 wide enemy-impact poses. Wider coverage comes from longer tea jets, detached droplets and ceramic fragments. Cup size is matched through the transition.

The accepted effect ends immediately after impact. No lingering puddle, sizzling, steam, held aftermath or fade. Prior local alternatives are superseded.

Start with UE5_HANDOFF.md for implementation. manifest.json contains measured crop rectangles and playback limits; manifest.js supplies the same data to the standalone preview. generation-prompts.json records the wide-impact generation requests. The enemy sheet was subsequently rearranged into a 2x2 grid for safe spacing; use manifest.json as the final layout authority.

This is an ArtSource upload only. Official gameplay code and packaged builds have not been changed.
