# v0.4.2 — Iron Aerie, expanded enemies and combat HUD

- Expanded dungeon floors with tougher late-room enemies and persistent exploration.
- Completed directional enemy animation packs, corrected anchors, revised elite scale and projectile presentation.
- New Iron Aerie floor and Iron Matriarch boss, with cinematic intro and recorded dragon audio.
- Iron Matriarch's flames escalate with exposure; meteors splash and slams deal heavy damage. Bosses remain immune to FREEDOM.
- Compact illustrated HUD, animated meters, mouse-button icons and graphical FREEDOM charge.
- Directional RMB blocking with a reusable five-second reserve and player block animation. Boss attacks retain 75% damage through block.
- Dedicated TEA flight, ground and enemy impact assets: hand-sized cup, compact splashes, 10 FPS playback, immediate endings, unchanged damage radius and cooldown.
- Removed obsolete floor-loot prompts. Updated current documentation, archived historical notes and retired one-off CMD launchers.

Download the Windows ZIP, extract everything, and run TheBeardAndBlade/TheBeardAndBlade.exe. The separate StreamPixel ZIP uses Windows/TheBeardAndBlade.exe. Both contain the same Pixel Streaming-enabled Windows x64 Shipping runtime. SHA-256 files are included.

Unsigned playtest; no campaign save. StreamPixel upload and hosted validation are separate steps.

## Earlier releases

# v0.4.1 — Forgotten Keep enemies and rotating inventory portrait

- Five approved enemies now populate the Forgotten Keep: Rustblade Squire, Graveglass Slinger, Chainbound Bailiff, Candle Hexer and Sepulcher Lancer.
- Directional idle, walking, attack, hurt and death animations, corrected crop masks, aligned scale/feet and attack-pose-synchronized damage/projectile release.
- Enemies are introduced by combat-room depth. Safe entrance, trader/reward branches, persistent backtracking and the existing boss progression are retained.
- A 32-view illustrated inventory adventurer: hold LMB over the portrait and drag horizontally through 360 degrees. Gameplay adventurer sprites and weapon anchors are unchanged.
- Inventory portrait rotation coexists with existing gear drag/drop, double-click equip/unequip and hover stats.

Download `TheBeardAndBlade-Windows-v0.4.1.zip`, extract everything, and run `TheBeardAndBlade/TheBeardAndBlade.exe`. UE is not required. The separate `TheBeardAndBlade-StreamPixel-v0.4.1.zip` uses the hosting-ready `Windows/` layout. Neither download is the GitHub source ZIP.

Windows x64 Shipping build, still unsigned. No campaign save system has been added. StreamPixel upload/activation remains a separate step.

## Earlier releases

# v0.4.0 — Emerald Atlas, expanded equipment and coin economy

- Explore all seven dungeon themes through four-way doors and the Emerald Atlas map. Backtrack through persistent rooms, discover a trader branch and reward dead end, and find each hidden boss encounter.
- Added head, hands, legs and feet equipment; premium trader stock and detailed upgrade/downgrade comparisons.
- Enemies can drop spinning, shining gold coins. Discarded equipment stays on the dungeon floor; sell inventory items at the trader for 50% of their value. New item values increased 50%.
- Trader portrait breathing and movement, polished inventory controls, currency and map artwork.
- Recorded footsteps, weapons, rifle shots, doors, props, inventory, enemy and explosion effects with included third-party credits. Human death voices removed; Finance Guy uses non-vocal effects.
- Quiet looping RPG Ambience - Dungeon by HitCtrl. Fixed room transitions raising music volume and removed the accidental N-key SFX mute shortcut.
- Reduced prop density and retained the existing boss order: Finance Guy, Big Mack, Flash Bang Guy, Webroot, Rime, Cinder, Twister.

Download the Windows ZIP to play locally without UE. Upload the separate StreamPixel ZIP (Windows folder layout) to your existing hosted project. Both use the same Pixel Streaming-enabled Windows Shipping runtime. No hosting credentials are embedded. Coins and exploration persist within a run, not across closing/restarting the game. Bright flash effects remain. StreamPixel browser validation is required after upload.

# v0.3.3 — loot, traders and dungeon progression

- Expanded campaign to 28 rooms: Finance Guy, Big Mack, Flash Bang Guy, Webroot, Rime, Cinder and Twister now guard every fourth room.
- Market Duelist trader visits halfway through each theme, with 3–5 offers priced at 120% of their loot value. Discarded gear credits a run-based coin wallet.
- Redesigned inventory with adventurer portrait, equipment icons, combined-stat hover panel, readable item cards, coin values and twelve new rings.
- Themed destructible crates, barrels and urns crumble and fade, with a 50% chance to leave loot.
- Opening a chest unlocks the exits immediately; uncollected loot can stay on the floor. Item cards remain inside inventory/shop.
- Boss entrance animations no longer use the reduced-motion option.

Download the complete Windows ZIP to play without Unreal Editor. The separate StreamPixel ZIP contains the same Shipping runtime in the hosting upload layout. Coins and inventory reset on a new run; no persistent save system is included.

# v0.3.2 — adventurer, combat and presentation update

- New athletic adventurer animation set, matching resting/movement proportions, breathing, right-hand weapon attachments and updated attack poses.
- Slower four-second stairway transitions with gentle shrinking and fading before the arch.
- Improved melee reach without increasing enemy contact-damage range; stronger regular enemies, lower sprint/dodge costs and limited projectile tracking that stops after a dodge.
- FREEDOM removes 75% of current health from regular enemies, executes only enemies already below 25% maximum health, and displays boss immunity.
- Sharper enemy/boss and tea-splash graphics, ornate menu hover states, sword cursor and clearer controls panel.
- Forgotten Keep main-menu music, music/SFX toggles and volume slider, including music resume fixes.
- Windows Shipping download and separate StreamPixel upload layout, both with Pixel Streaming enabled. No hosting credentials or signalling URL are embedded.

Download `TheBeardAndBlade-Windows-v0.3.2.zip`, extract everything and run `TheBeardAndBlade/TheBeardAndBlade.exe`. Upload `TheBeardAndBlade-StreamPixel-v0.3.2.zip` to StreamPixel; select `Windows/TheBeardAndBlade.exe` if prompted. UE5 is not needed to play the Windows download. Browser validation is required after hosting upload. SHA-256 files accompany both archives.

Unsigned Windows x64 playtest; keyboard/mouse controls. Existing bright flash effects remain. No saved campaign progress. Previous releases remain available.

# v0.3.1 — boss entrances and campaign endings

Download `TheBeardAndBlade-Windows-v0.3.1.zip`, extract the entire archive, and run `TheBeardAndBlade/TheBeardAndBlade.exe`. Windows x64; Unreal Editor is not required.

- Illustrated six-second entrance cinematics for all seven bosses, with separate character/title art, themed debris, impact timing and synchronized audio.
- Click, Space or Enter to skip an entrance; P pauses and R toggles reduced intro motion. Boss dialogue follows before combat resumes.
- New illustrated death screen, chamber reached, a closing saying, and New Run/Exit buttons.
- New illustrated victory screen and closing story after Twister in chamber 21. The campaign no longer loops into room 22. Earlier rooms retain their chest rewards.
- Safe ending cleanup, one-second restart input guard, and fresh-run resets.

Controls remain WASD, Shift sprint, Space dodge, LMB attack, RMB tea, MMB FREEDOM, E interact, I inventory, P pause, M music and N effects. Enter restarts from an ending. Editor-only boss shortcuts are disabled in Shipping.

Unsigned Windows build; no saved campaign progress. Flash grenades use bright fading effects and blur. This download is not yet configured for StreamPixel hosting; a separate Pixel Streaming-enabled package is needed.

## Earlier releases

# v0.3.0 — seven-chapter dungeon campaign

Download `TheBeardAndBlade-Windows-v0.3.0.zip`, extract the entire archive, and run `TheBeardAndBlade/TheBeardAndBlade.exe`. Windows x64; UE5 is not required. Keep the included folders alongside the executable. Visual C++ prerequisites are included under `Engine/Extras/Redist/en-us`.

- Twenty-one rooms: bosses every third room in order Finance Guy, Big Mack, Flash Bang Guy, Webroot, Rime, Cinder, Twister.
- New Greaseworks kitchen arena; matching bunker and storm chapters; 18 new animated enemies with melee, charge, rifle, grenade and elemental attacks.
- Sharper 256-pixel enemy frames, nearest-neighbor UI textures without mip streaming, frame preloading, and opaque spawning instead of washed-out sprite fades.
- Flash Bang Guy follows a successful stun from his own grenade with a teleport, one knife strike for exactly 25% of the player's current health at impact, and a teleport back. Looking away avoids both flash and combo. Ordinary flash troops cannot trigger this boss attack.
- Updated boss art, health-bar fit, weapon grip corrections, player idle breathing and Twister projectile/audio improvements from the local playtests.
- Animated chest opening, actual-item ejection, rarity glow, floor pickup and full-inventory protection.
- UE editor test shortcuts use the number row above QWERTY; test-room shortcuts are not enabled in the Shipping release.

Controls: WASD move; Shift sprint; Space dodge; LMB attack; RMB tea; MMB FREEDOM; E interact/pick up; I inventory; P menu; M music; N effects.

Unsigned Windows build. No saved campaign progress or ending; the seven-chapter rotation repeats after room 21. Flash grenades produce a bright fading flash and temporary blur. Balance and animation feel remain subject to player feedback.

## Earlier releases and development history

# v0.2.1 — combat balance and HUD cleanup

Extract the complete `TheBeardAndBlade-Windows-v0.2.1.zip` and launch `Windows/TheBeardAndBlade.exe`. Windows x64; Unreal Editor is not required.

- Removed the leftover upper-left attack/armor icons and room-state portal/chest/enemy icon.
- Ordinary enemy potion drop chance reduced from 35% to 12%. Bosses still guarantee a potion; healing amount is unchanged.
- Normal enemy melee, projectile and charge damage increased by about 36%. Boss damage is unchanged.
- Slowed walking to about 8 animation frames/sec and sprinting to about 10. Sprint movement remains 2× speed.
- Footsteps follow the slower gait cycle; reduced chest pulsing and vertical bob.
- Bleed and poison each have a 10% chance per successful weapon hit. Tooltips show the chance. Their existing durations and damage remain unchanged.
- Armor retains its stats but no longer changes the player's starting outfit colors, including during rolls.

Controls: WASD move; Shift sprint; Space dodge; LMB attack; RMB tea; MMB FREEDOM; E interact; I inventory; P menu; M music; N effects.

Unsigned feedback build. No saved campaign progress or ending; themes repeat after room 16. Windows only, not a browser build. The Visual C++ prerequisite installer is in Engine/Extras/Redist/en-us/vc_redist.x64.exe if needed.
# Local v0.3.0 playtest — September asset integration

- Big Mack in room 8; Twister in room 24; six-boss rotation preserves all previous bosses.
- Eight-direction boss animation, burger impacts/debris/landing dust, timed stun/slow, single-shot and full-auto attacks.
- Illustrated boss portraits/nameplates, dynamic ornate health bars and animated status indicators.
- Animated chest opening, actual-item ejection, rarity glow, floor pickup and full-bag protection.
- Original source sheets retained with reproducible cleaned exports. See SEPTEMBER_INTEGRATION.md for balance and art caveats.
- Local Windows package only; this task does not publish a new GitHub release.
