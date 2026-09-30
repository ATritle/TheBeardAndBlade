# Iron Matriarch — three-view attack source pack

This is new source artwork, not a completed Unreal implementation. Use the approved mechanical dragon identity; preserve existing game assets. Front, left three-quarter and right three-quarter views were requested, not a full eight-direction set. Do not mirror the character to create missing views: the five head colors and anatomy must remain consistent.

## Supplied sequences

Each view has 8 idle, 16 flying-slam, 12 flamethrower and 12 meteor-summon source poses: 48 per view, 144 character poses total. Five separate color flame sheets and four additional effects each request 8 poses: 72 effects poses. These are authored layout counts, not proof every generated pose is distinct or ready for production.

Read manifest.json for frame order and each sheet's rects array: [x, y, width, height] in source pixels. This revision uses measured crop rectangles, not automatic equal-grid slicing. Thirteen sheets received an artwork spacing pass; the front slam received a second pass. All 216 measured rectangles pass the alpha boundary check (alpha > 64, two-pixel boundary, warning threshold > 5 pixels). This checks cropping, not anatomy or motion continuity. Do not treat the browser flipbook as a verified UE animation.

## Attack 1: flying slam

Use ready/crouch/takeoff/hover/dive/impact/recovery poses. Candidate contact pose is 13 of 16, subject to visual review and actual landing. Separate the boss's ground collision anchor and shadow from visual flight height. Do not double-apply vertical lift in both artwork and runtime transforms. Apply landing damage once at actual impact and spawn slam-shockwave at the ground contact point. Cancel pending attack events on death/interruption. Choose telegraph duration, radius, damage, flight height and invulnerability rules with the current game design; none are approved by the art request.

## Attack 2: five-head close-range flamethrower

Five head identities from the FRONT viewer's left to right: icy white, blue, central crowned red, green, violet. Each head emits its corresponding colored fire. Color does not imply an additional status effect unless specified later.

Flamethrower poses 1–5 anticipate/charge, 6–9 suggest a sustain loop, 10–12 recover. Attach a separate flame emitter to each moving mouth; author per-frame mouth sockets after registration for each view. Flame sheets run left-to-right from a fixed emitter origin; orient and scale them toward the actual mouth aim in engine. Effect poses 1–2 ignite, 3–6 sustain, 7–8 extinguish. Keep the breath close-range. Stop emission and damage when the attack ends. Resolve damage tick interval and whether overlapping heads stack damage explicitly; never let render frame rate control damage.

## Attack 3: dungeon meteor rain

The summon sequence raises the heads/arms and lights the reactor; pose 8 is a provisional release cue. Spawn meteor warnings across valid dungeon floor locations, then falling meteors and one-shot impacts. The falling sheet is a downward-moving visual with its tail pointing up; move its visual from above while keeping a chosen ground target fixed. Spawn meteor-impact once when it reaches that target. Do not bake the whole dungeon meteor field into the boss animation.

Keep targets within navigable floor and outside walls; choose a readable warning interval and preserve escape routes. Meteor count, spread, damage, radius, cadence and attack cooldown remain tuning decisions. Do not silently turn the illustrated flame/embers into a persistent damage pool. Meteor warnings are a supplied readability effect, not a required new combat mechanic if the game already has a telegraph convention.

## Cleanup and import

- Inspect all five heads, crown, wings, hands, feet, and tail for continuity across every sheet. Side variants must remain actual three-quarter views; refine any view drifting back toward front.
- Use the supplied measured rectangles to avoid reintroducing clipping. Give exported frames consistent transparent padding. Register feet and preserve intentional motion arcs instead of centering each pose independently. Normalize world scale between sheets, especially the front slam, whose art was reduced further for clearance.
- Inspect alpha rather than hidden RGB: transparent pixels may contain brown RGB without showing in game. Preserve intentional flame sparks and detached debris.
- Import using the project's existing pixel-art texture/material pipeline, consistent boss world scale, and proper transparency. No automated raster cleanup was performed on these sources.
- Use idle loops and one-shot attacks; implement a deliberate sustain loop for breath. Determine final timing and contact/release events by viewing the poses in game.
- Test the three views, interruptions, transitions, flame attachment, single-hit slam/meteors, effect cleanup, and performance. Add locomotion/hurt/death or extra directions if required by gameplay; those are outside this requested attack pack.

No health pool, resistance, damage value, attack cooldown, audio, or gameplay implementation is included. Keep existing boss immunity rules, including FREEDOM immunity. Do not overwrite the official build with preview code.
