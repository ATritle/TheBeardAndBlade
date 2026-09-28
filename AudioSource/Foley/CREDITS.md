# Recorded footstep and weapon foley

Downloaded September 27, 2026. All three source pages explicitly license these assets CC0 1.0: https://creativecommons.org/publicdomain/zero/1.0/

- Fantozzi, single stone footsteps; cut/distributed by qubodup. https://opengameart.org/content/fantozzis-footsteps-grasssand-stone — Fantozzi-footsteps.7z. Six Stone L/R FLAC samples.
- artisticdude, Swishes Sound Pack. https://opengameart.org/content/swishes-sound-pack — swishes.zip. Swish 5, 7, 8 and 9. Recorded object swings, used as blade air movement (not claimed to be recordings of a sharpened sword).
- Vehicle / Jan Schupke, Fantasy Weapons and Apparel SFX Library. https://opengameart.org/content/fantasy-weapons-and-apparel-sfx-library — weapons-apparel.zip. Seax unsheathe 01 and 03 for equipment handling. Original readme retained with archive.

Source archives and extracted originals are retained locally; Prepared contains game-ready mono 16-bit WAV files. See manifest.json for exact per-asset source mapping, peak and duration. Processing: remove DC offset, trim silence with margins, 4ms edge fades, conservative peak normalization. No generated oscillators or added synthetic clangs.

Initial pass: six stone footstep variants, four weapon air-swish variants, two gear-handling variants. No immediate repeats within a multi-sample family. Footstep cadence unchanged; pitch variation narrowed to 0.97–1.03. Master volume and SFX mute still gate these samples.

Rebuild source WAVs using Tools/prepare_real_foley.py (numpy + soundfile); import using Tools/import_real_foley.py in Unreal. /Game/Audio is already always-cooked. Original procedural Step/Sword/Equip assets remain available for rollback, but runtime routes those events to the new Foley assets.

## Expansion

See Content/ThirdParty/SOUND_CREDITS.txt for full attribution and source links, including the CC-BY 3.0 inventory and monster recordings. That directory is staged as loose NonUFS content so credits accompany future desktop/streaming packages. The inventory pack is used under its CC-BY 3.0 option, not its alternative share-alike/GPL options.

Added a seventh stone step from TinyWorlds; all steps use a short smoothing filter and .50 peak (previously .65), with walking volume .28 (previously .55). Equipment swap files and playback settings remain unchanged.

New event families: door passage, inventory open/close, prop breaking, monster spawn/pain/death, explosion, tea blast, flashbang, paper, UI, roll, throw, magic, teleport, chest and impact. Door events are separate from Flash Bang Guy's teleport. Enemy death is separate from the adventurer's death. Spawn/pain/death cues have family cooldowns to avoid many voices stacking in one frame.

The original player hurt/death and eagle cues remain. No music changes. Old source assets remain for rollback but replaced event families route only to Foley-prefixed assets.

## Bunker / human audio pass

Flash Bang Guy (30) and bunker soldiers (37–39, 41–42) use HumanSpawn/Pain/Death. Finance Guy (24) uses paper on arrival and generic Hit impacts on damage/death, with no human voice cues. Scout Drone (40) uses DroneFlight/Pain/Death; the flight cue is short, quiet, throttled, and only retriggered in active gameplay, never an orphaned audio loop. Monster enemies retain their original monster recordings.

Rifle Trooper, Scout Drone and Twister use single-shot recordings from the Free Firearm Sound Library (CC0), not prerecorded automatic bursts. Human/robot cues use Little Robot Sound Factory's Voices library (CC-BY 3.0); flight uses jwiese's mechanic recording (CC0 option). Full credits ship in Content/ThirdParty/SOUND_CREDITS.txt.

Flash Cadet and Flash Bang Guy use the short FlashBang recorded blast; Mortar Engineer uses a dedicated longer Grenade blast, derived from the credited EZduzziteh explosion source. Equipment swap and softened footsteps are unchanged.

Human death voices are now disabled in gameplay: human enemies use a generic Hit impact on death. Source recordings are retained, but only pain/spawn human families are routed by enemies.

Validation: FoleyVerify covers imported families, variation, mute and human/drone/monster routing. Subjective voice selection and mix still need a listening playtest; automated checks cannot judge timbre or realism. Gameplay music now uses HitCtrl's RPG Ambience - Dungeon; see shipping credits.
