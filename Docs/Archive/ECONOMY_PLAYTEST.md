# Coin drops and trader selling (historical development notes)

- Standard enemies: 40% chance, 12–28 coins multiplied by theme number (1–7).
- Bosses: 85% chance, 80–140 coins multiplied by theme number.
- Coins use CurrencyCoin artwork, horizontal spin, glint, shadow, and amount label.
  Walk within 38 virtual pixels after a 0.6 second presentation delay to collect.
- Discard/Drop adds the exact gear to the floor. E picks it up; a full bag leaves it there.
  Discard never creates money. Floor gear and coins persist on backtracking within a floor.
- Trader: open INVENTORY / SELL, select a bag item, click SELL with the quoted price.
  Equipped items must first be moved into the bag. Selling pays 50% of item value,
  rounded down with a one-coin minimum. Trader purchase markup remains 20%.
- Newly generated item values are 50% higher. These are initial balance settings,
  not a claim of completed long-run economy balancing.
- Human death voices no longer play; human enemies use a generic impact on death.
  Human pain/spawn cues and robotic cues remain unchanged.

Checks: EconomyVerify covers discard/recovery, room persistence, pickup-once,
sale guards, discounts, overflow, and new-run reset. TraderVerify and FoleyVerify
cover shop and sound-routing regressions. EconomyReview captures floor and sale UI.
