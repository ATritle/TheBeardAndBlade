#pragma once
#include <cstdint>

namespace DungeonRevival
{
constexpr float ChoiceDelay=2.4f;
// Below the threshold, death consumes the whole purse. Above it, round the
// half-gold toll up without overflowing a signed 64-bit wallet.
constexpr std::int64_t Cost(std::int64_t Gold)
{
    return Gold<=0?0:Gold<2500?Gold:Gold/2+Gold%2;
}
}
