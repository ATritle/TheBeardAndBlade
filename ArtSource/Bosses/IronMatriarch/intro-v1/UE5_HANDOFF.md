# Iron Matriarch intro v1

Use character.png and title.png as independent transparent layers in the existing boss-intro controller. Match Twister intro-v5 / Big Mack intro-v1. This is an art and browser concept pack; no runtime implementation is included.

Play on entry into Iron Matriarch's dungeon after the destination room and camera are ready. Overlay the actual dungeon; the preview floor is illustrative only. Boss enters from left at 0.30 seconds, overshoots at 1.50, settles by 1.68, rests 0.42 seconds, then the nameplate enters at 2.10 and hits center at 2.40. Hold until 5.10; fade out by 6.00 seconds. Keep wings, crown and tail within the UI safe area. Use separate size/aspect rules for character and title.

Recreate preview embers and metallic sparks as suitable UI effects. intro-breviceps-roar.wav now replaces the rejected synthetic sound. It is a 6-second PCM timeline cue with Breviceps' complete roar starting at 2.10s, alongside the name entrance. Play this file at intro time zero; do not add another offset. breviceps-dragon-466830-hq.mp3 is the unchanged high-quality public preview source, NOT Freesound's original WAV. See AUDIO_SOURCE.md for license and original-download details. Respect volume, skip and teardown. The old iron-matriarch-roar.wav is superseded and must not be imported. Avoid covering the five faces with particles.

Use existing input/AI gating and skip handling. Trigger once per intended encounter, cancel particles/audio on skip or teardown, clear held inputs and restore control exactly once. Keep normal gameplay damage and boss abilities outside this UI sequence. Support reduced motion without travel or screen flash.

Review the cutout against the approved five-head design: viewer-left white and blue, crowned red center, green and violet on viewer-right. Validate transparency, edge padding, aspect ratios, timing and gameplay transition on the home PC. No UE5 or rendered-browser verification has been performed here.
