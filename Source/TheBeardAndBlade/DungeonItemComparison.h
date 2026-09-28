#pragma once
#include "DungeonActors.h"

namespace DungeonItemComparison {
struct FRow {
    const TCHAR* Label;
    float Equipped,Offered;
    bool Percent;
    float Difference() const { return Offered-Equipped; }
    // Match the displayed precision: avoid a red/green zero caused by hidden decimals.
    int32 Direction() const { return FMath::Sign(FMath::RoundToInt(Difference()*(Percent?1000.f:10.f))); }
};
inline TArray<FRow> Rows(const FDungeonItem& Equipped,const FDungeonItem& Offered) {
    TArray<FRow> Result;
    auto Add=[&](const TCHAR* Label,float FDungeonItem::* Field,bool Percent=false) {
        const float A=Equipped.IsEmpty()?0:Equipped.*Field,B=Offered.IsEmpty()?0:Offered.*Field;
        if(!FMath::IsNearlyZero(A)||!FMath::IsNearlyZero(B))Result.Add({Label,A,B,Percent});
    };
    Add(TEXT("Damage"),&FDungeonItem::Attack);
    Add(TEXT("Armor"),&FDungeonItem::Defense);
    Add(TEXT("Health"),&FDungeonItem::Vitality);
    Add(TEXT("Attack speed"),&FDungeonItem::Speed,true);
    Add(TEXT("Move speed"),&FDungeonItem::Movement,true);
    Add(TEXT("Crit chance"),&FDungeonItem::CritChance,true);
    Add(TEXT("Crit damage"),&FDungeonItem::CritDamage,true);
    Add(TEXT("Stamina regen"),&FDungeonItem::Regen,true);
    Add(TEXT("Max stamina"),&FDungeonItem::StaminaBonus,true);
    Add(TEXT("Max health"),&FDungeonItem::HealthBonus,true);
    Add(TEXT("Bleed chance"),&FDungeonItem::BleedChance,true);
    Add(TEXT("Poison chance"),&FDungeonItem::PoisonChance,true);
    Add(TEXT("Damage leech"),&FDungeonItem::Leech,true);
    Add(TEXT("Reduction"),&FDungeonItem::Reduction,true);
    return Result;
}
}
