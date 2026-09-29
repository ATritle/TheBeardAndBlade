# Enemy expansion — home PC continuation checkpoint

796 of 1345 selected sprite sheets are included for 25 active enemies. **549 sheets remain ungenerated.** This is unfinished source art, not a gameplay release or an approved animation pack.

Open `preview.html` locally after pulling the repo to browse each enemy. GitHub itself does not play the HTML previews.

## Continue here

1. Pull the latest repository and preserve all newer home-PC gameplay work. Forgotten Keep's five enemies and the inventory turntable were integrated in v0.4.1; do not replace them.
2. Read `inventory.json`, `missing-jobs.json`, `REVIEW_CHECKPOINT-2026-09-29.md`, and `HOME_PC_INTEGRATION-v2.md`.
3. Each enemy has its selected art in `../EnemyName/animation-v2`. `selected-files.json` is authoritative for animation sources; other PNGs are reference dependencies. Prompt paths resolve from the repository root, not this folder. A missing job whose reference is also missing must wait for that reference job first.
4. Preserve the existing character design and generate remaining directional poses using the saved references/prompts. Prioritize complete walking/attack sequences before polishing optional lifecycle states. A/B sheets are the first/second halves of one attack, not two different attacks.
5. Inspect and repair facing, hands, anatomical consistency, clipping, transparency and ground pivots. Generated pose counts do not guarantee distinct or smooth frames. Previews use provisional crops and do not prove final quality.
6. Adapt the current `Tools/prepare_expansion.py`, `Tools/import_expansion.py`, and `Tools/verify_expansion_art.py` pipeline. Finish UE5 timing, collision, projectile events and gameplay checks before packaging a release.

## Held scope

Do not generate or integrate Nugget Knuckler, Ketchup Cadet, Skillet Scrapper, Fondue Conjurer or Deep-Fry Juggernaut. Greaseworks is being redesigned. No held enemy artwork is included in this checkpoint.

Health/damage tiers in enemy-spec.json are proposals, not applied gameplay changes. This checkpoint modifies ArtSource only.
