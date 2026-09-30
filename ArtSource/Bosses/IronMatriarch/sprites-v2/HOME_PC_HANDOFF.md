# Continue Iron Matriarch on the home PC

Pull the latest repository changes, then review ArtSource/Bosses/IronMatriarch/sprites-v2/preview.html and UE5_HANDOFF.md before implementation. This is the revised source-art pack approved for upload, not a finished runtime boss.

1. Preserve the existing game and other bosses. Follow the current project's sprite, material, input and damage conventions.
2. Import the 21 selected sheets listed in manifest.json. Use each frame's rects entry [x,y,width,height]; do not slice by equal grid. There are 144 character pose slots across front/left/right variants plus 72 effect slots. design-reference.png is a reference, not an animation atlas.
3. Align feet/ground pivots and normalize world scale across sequences. The front flying-slam sheet has additional transparent spacing and smaller source poses. Preserve intentional flight/crouch motion; do not normalize every pose to a different scale. Inspect head, wing and tail continuity during playback. Side views may need refinement to strengthen their three-quarter facing.
4. Implement flying slam with a readable windup, flight, one landing hit, separate ground shockwave and recovery. Implement close-range five-head breath using mouth anchors and matching white/blue/red/green/violet effects. Implement meteor summon with ground warnings, falling meteor visuals and single impact damage at valid floor targets. See UE5_HANDOFF.md for event and interruption details.
5. Keep boss FREEDOM immunity. Health, damage, ranges, timings and cooldowns are not approved values in this pack; expose tuning variables and resolve them using the current design. Additional locomotion/hurt/death animations and audio are not supplied.
6. Test animation transitions, pivots, all three views, flame sockets, telegraphs, attack cancellation/death, one-hit impact behavior and performance before packaging. Report remaining cleanup and show gameplay previews before calling integration complete.

Verification performed here: all 21 PNG sheets have transparency; all 216 measured frame rectangles pass the documented alpha boundary check; preview playback controls pass JavaScript logic tests. No rendered-browser or UE5 test was performed. The separate QA.json equal-grid warnings do not apply to the measured preview rectangles and demonstrate why uniform slicing should not be used.
