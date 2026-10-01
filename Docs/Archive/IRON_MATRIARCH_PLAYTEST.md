# Iron Matriarch / Iron Aerie — local integration review (historical development notes)

No GitHub push, release replacement or StreamPixel package was requested or made.

## Latest damage/clarity revision

Front-art follow-up: the soft v3 artwork is superseded by front-slam-v4.
Sixteen poses are registered from higher-detail small groups, with user-approved
translucent-edge cleanup and nearest filtering on the front slam only.
See ArtSource/Bosses/IronMatriarch/front-slam-v4/README.md.

- Slam now deals 85 raw damage once on impact (previously 42).
- Flame ticks are 12, 18, 24, 30, 36 during uninterrupted exposure; leaving
  the cone or dodging resets escalation. All five heads share one damage cadence.
- Meteor radius is now 110, with 55 raw damage within radius 35, falling to
  13.75 at the outer edge. Warnings use the same projected ellipse as splash.
- Blocking any boss attack removes 25% of otherwise received damage, after
  armor/reduction; rear hits remain unblocked. Regular enemies still fully block.
  Flash guard prevents blindness/ambush while passing reduced boss damage.
- Front-facing slam received a detailed 16-pose art correction, registered to
  the previous silhouette bounds. Source/provenance and crop measurements:
  ArtSource/Bosses/IronMatriarch/front-slam-v3. Other animations unchanged.

This section supersedes the initial tuning/source-detail notes below.
Current tests: IronDamageVerify.log (110 checks, zero errors) and
BossBlockFinal.log (291 checks, zero errors), Development Editor build and
asset import. Use the same Play_Iron_Matriarch_Test.cmd launcher.

## Play

- `Play_Iron_Matriarch_Test.cmd`: boss encounter with the six-second intro. R restarts it. The test equips level-scaled sword/armor as existing boss tests do. Normal damage applies.
- `Play_Iron_Aerie_Floor.cmd`: eighth-floor entrance, fourteen-room exploration map, trader/reward branches and late mechanical guards. R restarts the floor.
- Normal campaign: existing seven bosses retain their order; Twister now opens the eighth floor. Iron Matriarch defeat triggers the updated eight-floor victory story.
- LMB sword; MMB tea; RMB directional block; 1 FREEDOM (boss immune); SPACE dodge; E interact; M map; I inventory.

## Art and provenance

Source files were fetched non-destructively from GitHub commit `5cb880568a3ae80c9d6bcc1f0f3c67f8d858522d`, only `ArtSource/Bosses/IronMatriarch`. Existing local game changes were not merged away. Original 21 sheets and handoffs are unchanged.

`Tools/prepare_iron_matriarch.py` uses all 216 manifest rectangles, not a uniform source grid. It produces 144 character and 72 effect poses in padded 512-pixel cells, and checks alpha boundaries on every exported pose. One fixed scale per sequence preserves crouches and changing wing silhouettes. Slam neutral poses are crouched, so their scale uses a smaller target height than upright idle; scaling each pose independently would make the body pulse. Front-slam source resolution remains that of the supplied pack—no claim of newly generated high-resolution source detail.

Per-frame feet are registered independently from wing/tail bounding-box centers. Runtime flight lift is applied once after foot registration; shadow and impact remain on the floor. Runtime imports are lossless UI textures, no mipmaps, never streaming, with bilinear sampling at gameplay size. Registered review sheets and exact transforms are in `ArtSource/Bosses/IronMatriarch/runtime-v1/registration.json`.

Front, left three-quarter and right three-quarter use their supplied art, never mirroring. Five-head breath uses reviewed per-pose mouth coordinates transformed by the same registration and exported to `IronMatriarchSockets.h`. White, blue, crowned red, green and violet retain their identities. Flame emitters spread without crossing, aim at fixed floor endpoints, and do not imply extra status ailments.

The new Iron Aerie background is `ArtSource/Exploration/AtlasChamber7.png`, generated with the built-in image tool using the existing Cinder room as a layout/style reference. Prompt and saved origin are in `ArtSource/Bosses/IronMatriarch/runtime-v1/dungeon-art-prompt.json`. Current floor population reuses approved mechanical/forge enemies and foundry props, not new enemy designs.

Missing animation sheets are handled deliberately for this test: the boss is planted during idle and repositions by flying slam, not by sliding a fake walk cycle. Hurt is a brief tint/impact response, without cancelling her armored attack. Death cancels events and performs a 1.8-second compressed mechanical collapse/fade with the supplied impact effect. These are runtime presentations, not newly authored locomotion/hurt/death sprite sheets.

## First-pass balance (not final tuning)

All tuning lives in `Source/TheBeardAndBlade/IronMatriarch.h`.

- Base HP 1800; existing boss scaling at room 32 yields 4315.95 HP. No extra elite multiplier.
- Slam: 2s telegraph/flight to impact pose 13, radius 115, damage 42 once, recovery to 3.1s. Destination locks at windup and stays within boss-safe room bounds. No special airborne invulnerability.
- Breath: 1.2s windup, 2s active, 0.8s recovery; 230 ground-unit reach. Five shared 9-damage ticks at 0.4s cadence, not five simultaneous head hits. Player block/dodge and armor rules apply normally.
- Meteors: summon pose 8 at 1.4s; each target gets 1.15s warning plus 0.45s falling visual. Five staggered impacts, radius 58, damage 32 each once. Fixed player snapshot plus four spaced arena sites leaves escape room; no persistent damage pools.
- Attack cycle is slam → breath → meteors. Recovery between attacks is 1.7s, shortening to 1.1s below 40% health.
- Death, room teardown and explicit state cancellation remove pending attack events. Menus/inventory pause the state machine with normal gameplay.

## Audio and intro

`intro-v1/intro-breviceps-roar.wav` is imported unchanged as IronIntro. Its built-in 2.10s silence is preserved: playback starts at intro time zero with no added delay. Existing left-entry/settle/nameplate timeline lasts six seconds; skip/pause/teardown use the existing controller. The user's previous removal of reduced-motion behavior remains in effect.

Gameplay cuts use the repository's unchanged Breviceps high-quality MP3 source (CC0 per supplied source notes), not the padded intro file and not the rejected synthetic roar. The separately mentioned Downloads path was unavailable locally; the same named supplied repository MP3 was available. WAV exports are PCM conversions, not restored original-quality recordings. Cuts, gain/fades and provenance are in `runtime-v1/audio-manifest.json`; original pitch is retained. The original Freesound WAV remains an optional production-source upgrade.

## Verification

Development Editor build and 33-asset import succeeded. IronVerify checks event counts at 15/30/60/120 FPS, flame overlap cadence, fixed meteor targets, floor bounds, directional block without stamina, cancellation/death, FREEDOM immunity, six-second audio asset, new final victory and Twister descent. AtlasVerify checks all eight floors, 128 seeds per floor, every encounter, backtracking and boss progression. Rendered review captures exercise the intro and three attacks; the recorded preview is scripted and silent, not a human balance playtest.

Final results: `IronVerifyFinal.log` 104 checks / 0 errors; `IronAtlasVerify.log`, `IronIntroVerify.log`, and legacy `IronEndingVerify.log` each report 0 errors. The six-second intro PCM was checked directly: first audible sample above threshold is 2.138s (source begins at 2.10s), with no runtime offset added. Slam/meteor collision matches the same .65 Y-projected ellipse as the warning ring. `Iron_Matriarch_Review.mp4` is a 35-second silent engine capture in this folder's `ArtSource/Bosses/IronMatriarch/runtime-v1/` directory. No Unreal process was left running at handoff.

For repro: run `Tools/prepare_iron_matriarch.py`, `Tools/prepare_iron_audio.py <ffmpeg>`, then UE's Python commandlet with the absolute path to `Tools/import_iron_matriarch.py`. Verify with `-game -IronVerify` (sound enabled) and `-game -AtlasVerify -nullrhi`. Render with `-game -IronReview -IronCapture=Saved/IronReviewFinal -RenderOffscreen`.
