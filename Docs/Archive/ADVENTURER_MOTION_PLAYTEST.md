# Adventurer locomotion review — September 27, 2026 (historical development notes)

## Scope and status

Originally a local UE review build. The September 28 handoff packages this tested
pass for StreamPixel and GitHub main at the user's request; no new release tag.
The original Athletic_* textures and attack/roll artwork remain untouched.

128 additive textures provide eight walking and eight running frames in each of
eight directions. Source sheets, registration reports, socket overrides, looping
previews and the built-in image-generation prompt set are in
`ArtSource/HeroLocomotionV2/`. Imported assets use the `Locomotion_*` prefix under
`Content/Art/V2/`.

**Visual polish is not final.** The generated diagonal sequences still contain
uneven opposite-foot progression. Several corrective generations repeated the
same leading leg or introduced a background and were rejected. Do not describe
this pass as production-approved natural gait simply because it compiles or its
frame-selection tests pass. Further pose-level work is needed, especially NE, SW
and NW. Runtime tests cannot validate anatomy or perceived smoothness.

## Runtime changes

- Walk/run visuals face actual movement. Attacks and tea remain cursor-aimed.
- Stationary facing follows the cursor. Flashbang exposure uses visible facing.
- Eight-frame movement playback is independent of six-frame attacks.
- Actual displacement drives gait, including speed gear, slow effects and walls.
- Walk/run share phase, with a longer sprint stride. Movement speed is unchanged.
- Steps occur on half-cycle contacts, not a separate free-running sound timer.
- New poses have per-frame anatomical right-hand anchors. Far-side weapons render
  behind the body; the left hand is never used as a replacement grip.
- Idle uses a matching close-foot walk pose to avoid swapping to differently sized
  idle artwork. Existing breathing remains, without additional locomotion warping.
- Door transitions explicitly select walking, even after sprinting into a door.

## Review

Launch `Play_Atlas_UE.cmd` for normal gameplay. Check all eight directions, sprint
start/stop, idle-to-walk proportions, attacks opposite the travel direction, tea,
roll recovery, right-hand grips and all four doorway fades.

`-LocomotionPreview` creates a silent, auto-closing UE animation board, capturing
16 frames under `Saved/Screenshots/LocomotionV2/`. `-DungeonHeroReviewPreview`
with `-DungeonBiome=2` or `3` shows eight static walk frames per direction;
add `-SprintReview` for running. Existing groups 0/1 remain attack review boards.

`-LocomotionVerify` checks all 128 textures, filtering/streaming settings,
eight movement-facing directions, frame bounds, distance-based phase at 60/120Hz,
speed modifiers, idle, wall blocking, doorway walking and cursor-aimed attacks.

## Reproduce artwork import

1. `Tools/prepare_hero_locomotion_v2.py`: mechanically extracts/aligns authored
   sprites, keeps source alpha and emits measured hand anchors and review sheets.
2. `Tools/import_hero_locomotion_v2.py`: UE Python import with nearest filtering,
   no mipmaps and no texture streaming.
3. Rebuild the Editor target and run the review/test flags above.

Source art is ignored by the existing repository rules; preserve the source folder
alongside any future release handoff. Built-in image generation was used, not an
external API/CLI. The prompt set is `ArtSource/HeroLocomotionV2/prompts.json`.

## Recorded validation

- Editor target compiled successfully.
- 128 textures imported successfully.
- LocomotionVerify: 0 errors (including movement/attack-facing separation).
- FlashVerify: 0 errors.
- DungeonVerify with LegacyProgression: 0 errors.
- Silent rendered UE contact sheets captured; no release packaging performed.

These checks establish runtime correctness, not final artistic approval.
