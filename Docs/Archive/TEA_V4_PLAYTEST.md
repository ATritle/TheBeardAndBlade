# TEA v4 wider impacts — local review (historical development notes)

Approved ArtSource/Tea/effects-v4 artwork, measured manifest crops, no new
generation. Eighteen dedicated textures under /Game/Art/TeaV4; shared TeaFX
textures and materials are not replaced.

- Flight: eight baked tumbles, 10 FPS; no additional rotation. Existing arc,
  target clamp and .35–.75-second flight retained. Short throws naturally display
  fewer frames rather than altering travel time to force a full animation loop.
- Ground: six frames, 0.6 seconds. Enemy: four frames, 0.4 seconds.
- One ground burst at landing; one overlay per confirmed enemy hit. Overlays
  follow surviving targets and remain at the last contact when targets die.
- No fade, held aftermath, puddle, sizzling loop, extra damage, or damage over time.
- Unchanged damage: 55 + attack power * 0.8. Radius 135, with existing
  +12 regular / +24 boss hit tolerance and .65 ground projection.
- Unchanged cooldown 10 seconds, .48-second cast and .22-second release.
- Revised gameplay cell widths: ground 112; flight 38; enemy 124. Held cup is
  attached at its handle rather than its center. These
  are visual dimensions, not gameplay radii. The existing targeting ring retains
  the real damage radius; detached decorative droplets may extend beyond it.
- Per-sequence scale with measured cup, floor-contact and radial impact anchors.
  No per-frame auto-fit. Native-alpha, lossless, no mipmaps, resident textures.

Play_TEA_V4_Test.cmd opens a stationary-target room. MMB throws normally, R resets.
The automated silent preview uses direct timed launches to show several throws
quickly; this harness does not change the gameplay cooldown.

Validation: Development Editor build; 18-asset import; TeaV4Verify reports
83 checks / 0 errors covering visual scale, eight directions, near/far throws, target clamping,
unchanged damage/cooldown, multiple targets, normal/boss radius boundaries, exact
0.4/0.6-second removal, no repeat damage, 15/30/60/120 FPS lifetime, asset loading,
and death teardown. No packaging, release, or GitHub push.

Reproduce: Tools/prepare_tea_v4.py, UE commandlet with absolute path to
Tools/import_tea_v4.py, -game -TeaV4Verify. Capture with -game -TeaV4Review;
frames go to Saved/TeaV4Review.
