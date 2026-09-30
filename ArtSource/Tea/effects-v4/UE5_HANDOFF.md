# TEA v4 — wider splash artwork

New ground and enemy splash artwork replaces v3 compact splashes. Ground uses six frames (3x2); enemy uses four frames (2x2). Cup flight artwork is unchanged. The new impact geometry has longer lateral jets, detached droplets and scattered shards; it is not simply the earlier image enlarged.

Use measured manifest rectangles. Ground preview size302 compensates for the small cup inside the larger source cell, matching the flight cup at108; enemy310 makes its newly drawn broad radial spray readable. These are cell render dimensions, not hitbox radii. Exact sprite anchor/world-scale refinement remains UE5 work.

Keep 10fps: ground0.6s, enemy0.4s, then immediate removal. No aftermath pool, sizzling, steam hold or fade. Wider visuals do not change damage or area-of-effect gameplay radius without user approval. Ensure visual radius and gameplay radius are reconciled during integration. This upload contains approved source artwork and preview files; UE5 implementation is pending.

## Home-PC implementation checklist

Pull the latest main branch and use ArtSource/Tea/effects-v4 as the approved visual reference. Earlier local v1-v3 aftermath experiments are superseded and not part of this upload.

1. Inspect current Source/TheBeardAndBlade/DungeonTea.cpp (PowerMove, LaunchTea, ResolveProjectile) and DungeonActors.cpp (projectile/splash rendering). In the inspected version the cast lasts .48s, releases after .22s, and flight lasts .35-.75s. The current cup is TeaFX_0; the friendly ground splash is TeaFX_8. The old impact scales/fades a single image rather than advancing frames.
2. Import the three PNG sheets using manifest.json rects [x,y,width,height]. There are 8 cup poses, 6 ground-hit frames and 4 enemy-hit frames. Register hand/cup release, floor contact, and enemy hit anchors. Use one consistent scale per sequence, not per-frame auto-fit. The original TeaFX atlas is shared with other effects; add dedicated TEA assets instead of globally replacing shared textures/materials.
3. Replace the rotating single cup with the tumbling cup animation. Avoid applying full sprite rotation on top of baked tumbling poses. Preserve the existing trajectory and targeting unless an intentional approved adjustment is needed to align the visual.
4. Play the wider ground burst at landing and the enemy overlay on confirmed hits, following the existing area-hit rules. Avoid duplicating damage or spawning repeated bursts every render frame. Preserve current health, damage, cooldown and input bindings. No new damage-over-time behavior is requested.
5. End ground visuals at .6s and enemy-hit visuals at .4s. No residual puddle, steam, sizzling loop or fade-out. Support cancellation/cleanup on death, room change and level teardown.
6. Test all aim directions, near/far throws, multiple enemies, bosses, walls, repeated casts, viewport sizes and performance. Confirm cup size remains consistent through impact, droplets/shards stay within crops, and wider visuals communicate the actual gameplay area. Show an in-game preview before packaging.

Verification here: preview JavaScript syntax and frame references checked; measured crop boundary scan found no warnings. This is not a rendered-browser or UE5 runtime test. Final alpha appearance, pivot alignment, timing and transition polish require home-PC verification. No new sound is included; retain current TEA sound unless separately revised.
