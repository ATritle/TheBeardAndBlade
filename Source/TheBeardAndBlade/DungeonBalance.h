#pragma once
#include <algorithm>
#include <cmath>

// Balance pass 1: pure functions shared by the game and native regression probe.
// Floor is zero-based. Depth is graph distance (+2 on optional combat branches),
// never visit count or equipment. Tune here after reviewing Saved/Balance CSVs.
// Initial targets: regular 3-5 hits, heavies 6-9; elites about 15-25s and bosses
// about 90-150s at typical equipment / roughly 50% attack uptime.
// Tools/balance_probe.cpp checks formulas; -BalanceVerify checks live UE wiring.
namespace DungeonBalance {
template<class T> constexpr T Clamp(T V,T L,T H){return V<L?L:V>H?H:V;}
enum class Role { Nuisance, Ranged, Melee, Heavy, Elite, Boss };
constexpr int LootLevel(int Floor,int Depth) {
    return 1+Clamp(Floor,0,7)*3+Clamp(Depth,0,9)/3;
}
constexpr float ExpectedHit(int Floor,int Depth) {
    return 28.f+4.5f*Clamp(Floor,0,7)+.65f*Clamp(Depth,1,9);
}
constexpr float ExpectedDPS(int Floor,int Depth) {
    return ExpectedHit(Floor,Depth)*(1.10f+.035f*Clamp(Floor,0,7))/.48f*1.08f;
}
constexpr float EnemyHealth(Role Kind,int Floor,int Depth) {
    const float Hit=ExpectedHit(Floor,Depth);
    switch(Kind) {
        case Role::Nuisance:return Hit*1.65f;
        case Role::Ranged:return Hit*3.0f;
        case Role::Melee:return Hit*4.0f;
        case Role::Heavy:return Hit*7.0f;
        case Role::Elite:return ExpectedDPS(Floor,Depth)*11.0f;
        case Role::Boss:return ExpectedDPS(Floor,Depth)*60.f;
    }
    return Hit*4;
}
constexpr float IncomingScale(int Floor,int Depth,bool Boss) {
    return 1.f+(Boss?.08f:.12f)*Clamp(Floor,0,7)+.018f*(Clamp(Depth,1,9)-1);
}
constexpr float ArmorReduction(float Armor) {
    return Clamp(Armor/(std::max(0.f,Armor)+85.f),0.f,.60f);
}
constexpr float Mitigated(float Damage,float Armor,float Reduction,bool Aegis) {
    return std::max(0.f,Damage)*(1-ArmorReduction(Armor))*
        (1-Clamp(Reduction,0.f,.30f))*(Aegis?.92f:1.f);
}
constexpr float PrimaryScale(int Level,int Rarity) {
    return (1+.055f*(Clamp(Level,1,25)-1))*(1+.09f*Clamp(Rarity,0,4));
}
constexpr float PercentScale(int Level,int Rarity) {
    return (1+.006f*(Clamp(Level,1,25)-1))*(1+.08f*Clamp(Rarity,0,4));
}
constexpr int ChestRarity(float Roll,int Level,bool Boss=false) {
    const int Floor=Clamp((Level-1)/3,0,7);
    if(Boss)return Roll<.20f?4:3;
    const float Legendary=Floor<3?0.f:.01f;
    const float Epic=.01f+.018f*Floor;
    const float Rare=.09f+.035f*Floor;
    const float Uncommon=.35f;
    return Roll<Legendary?4:Roll<Legendary+Epic?3:
        Roll<Legendary+Epic+Rare?2:Roll<Legendary+Epic+Rare+Uncommon?1:0;
}
constexpr int TraderRarity(float Roll,int Level,bool Featured) {
    const int Floor=Clamp((Level-1)/3,0,7);
    const int Tier=Roll<(Floor>=3?.02f:0.f)?4:Roll<.08f+.025f*Floor?3:Roll<.48f?2:1;
    return Featured?std::max(2,Tier):Tier;
}
constexpr float PotionFraction=.22f, PotionChance=.07f;
constexpr float SignatureLeech=.025f, HarvestHealing=2.f;
constexpr float GearHealingPerSecond=.015f;
// Must exceed hurt + recovery + longest regular windup (about 1.75s), or
// fast weapons can still cancel every slow elite attack before its release.
constexpr float StaggerCooldown=2.25f;
constexpr float AttackStartGap(int Floor){return .30f-.015f*Clamp(Floor,0,7);}
constexpr int AttackSlots(int Floor,int Depth){return Floor>=2||Depth>=6?3:2;}
}
