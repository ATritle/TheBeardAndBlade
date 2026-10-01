# Trader and 28-room campaign — local UE review (historical development notes)

Boss rooms: Finance Guy 4, Big Mack 8, Flash Bang Guy 12, Webroot 16, Rime 20, Cinder 24, Twister 28. Every chapter now has three standard rooms and one boss. Existing boss shortcuts follow the new room numbers.

Trader visits happen after taking an exit from rooms 2, 6, 10, 14, 18, 22 and 26, before the next dungeon spawns. No purchase is required to continue. Combat and movement pause during shopping. Stock has 3–5 unique catalog items with ordinary rarity odds and the upcoming room's level. Stock is fixed for that visit, including while opening/closing inventory. Sold items cannot be repurchased.

Discard a bag item to gain its full coin value. The wallet is shown in inventory and the shop, and persists between dungeon rooms. Like the existing inventory, it resets for a new run and is not saved across application restarts. Equipped items must be unequipped before discarding.

Shop prices are 120% of item value, rounded up to a whole coin. Purchase charges only after the item fits in the bag; insufficient coins or space leave both stock and wallet unchanged. Reselling purchased items returns their original value, not the marked-up shop price. This prevents a buy/discard profit loop.

Open inventory with I or the shop's inventory button to equip, rearrange or discard gear. Close it to return to the same stock. Hover/click an offer to inspect its card; click BUY to purchase. Continue deploys into the next room once.

## Review / validation

- `-game -TraderVerify -nullrhi`: wallet credits, invalid duplicate discard, all seven stops, frozen stock, markup, insufficient funds, full bags, exact purchase, sold-item guard, inventory access, wallet carryover and restart.
- `-game -DungeonVerify -nullrhi`: whole 28-room campaign, trader stops and final boss ending.
- Regression flags: SeptemberVerify, IntroVerify, FlashVerify, BreakablesVerify, ProgressionVerify, EndingVerify.
- `Tools/capture_review.ps1 -Preview Trader`: five-offer shop screenshot, 1200 preview coins, selected ring card. Preview funds never affect a normal run.

## Artwork

Current selection: Market Duelist, with a short tousled blonde bob, emerald cropped vest, brown shorts, burgundy sash and a sword resting beside her leg. Source: `ArtSource/Trader/MarketDuelist.png`, generated with the built-in image tool. The importer replaces the existing TraderPortrait runtime texture without changing shop behavior. Earlier portraits are retained locally.

Generated with the built-in image-generation tool, not CLI. Selected portrait: `ArtSource/Trader/TraderPortrait.png`; imported as `/Game/Art/V2/TraderPortrait` by `Tools/import_trader.py`. The selected portrait retains its warm dark backing within the framed portrait panel. No code-based background removal was used.

Generation prompt: Reference image is character design reference, not a background to preserve. Create a full-body standing adult female fantasy dungeon trader in high-quality detailed pixel art for The Beard and Blade inventory portrait, transparent background. Preserve reference physique, athletic curvy hourglass body proportions, golden blonde hair color, very voluminous high messy ponytail and long front braids with side undercut, green eyes. Preserve brown cropped jacket, dark fitted corset top, bronze goggles at neck, tan baggy cargo trousers, belts and green gloves. Subtle changes: add small merchant coin pouch and practical boots. Confident welcoming expression, one hand at hip and other holding small leather coin purse, relaxed trading stance. Entire body from ponytail to boots visible with padding, portrait aspect ratio, centered. Crisp controlled pixel clusters, shaded metal and cloth, dark fantasy RPG style, readable face, no blur, no text, signature, watermark, background scene or UI. Actual transparent alpha background. Do not exaggerate proportions beyond reference.

An additional background-extraction request did not remove the backing reliably; the first portrait was selected for its sharper pixel detail.

Included in v0.3.3 Windows and StreamPixel packages.
