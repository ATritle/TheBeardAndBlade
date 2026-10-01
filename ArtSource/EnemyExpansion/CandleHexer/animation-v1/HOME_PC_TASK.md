# Home-PC UE5 task: Candle Hexer

Integrate this source pack into The Beard and Blade on the UE5 home PC after visual approval. Read README.md, QA.md and the ordered entries in atlas-manifest.json. Use the supplied artwork and preserve the Forgotten Keep style rather than replacing it with new concepts.

Review all eight hover/cast facings, left-hand staff and right-hand casting anatomy, flame edges, body scale, loop seams and charge-to-recovery joins. The rear diagonals can read too similarly and require attention. Use attack-a-W-grip.png, not the rejected original west sheet. Apply measured clipRows masks where supplied, then normalize fixed canvases and stable hover/body pivots. Do not import the background-bearing turnaround as animation.

Complete idle, hurt and death states. Import approved pixel-art sprites using the established hero/enemy texture settings. Create hover and cast playback with one release event, aligned to the final art. Keep the old game roster and numeric IDs stable. Add Candle Hexer to appropriate Forgotten Keep spawn choices only after validation.

Use the provisional Medium profile: base HP 76, raw damage 15, 0.9-second windup, 1.2-second recovery, no contact damage. Apply current scaling exactly once. Add one wax comet per accepted cast, traveling at 230 logical pixels/second for at most 2.6 seconds. Limit initial tracking to 60 degrees/second for 0.65 seconds, ending within 100 logical pixels. Dodging permanently breaks tracking for that shot; no reacquisition.

Use swept earliest-hit collision, once-only resolution and immediate projectile removal. Play player-impact for an accepted player hit and environment-impact source frames 3–16 for walls/floor. Anchor impact effects to the collision point. No double splash damage, residual damage or arbitrary damage at lifetime expiry. Death/stagger cancels unreleased casts. Preserve standard-enemy FREEDOM rules and boss immunity.

Test the artwork against actual dungeon backgrounds, all facings, player dodges, walls, hit reach, scaling, death during cast, and 30/60/120 FPS. Check existing hero/boss/save behavior. Report remaining art and gameplay issues honestly; package only after missing states and integration checks are complete.


NW perspective correction: use walk-NW-occlusion.png, attack-a-NW-hidden.png and attack-b-NW-occlusion.png as selected by the manifest. The left hand grips the candle wand; the far-side right hand is fully occluded by the robe in this view. Do not restore the original rear-protruding hand or rear palm spark. Cast release still uses a separate projectile/event; the hand itself is hidden at this angle. Correction prompts are in nw-correction-prompts.json.

