#pragma once
#include "CoreMinimal.h"
#include "RustbladeSquire.h"
#include "DungeonExpansion.h"
#include "IronMatriarch.h"
struct FDungeonSpecies
{
    const TCHAR* Name;
    float Speed,HP,Damage,Range,Windup,Recovery;
    int32 AttackStyle;
    bool Flying;
};
namespace DungeonRoster
{
// Atlas depth, not visit count or the clamped loot/health room number.
// Replace slots in the original 4/5/6/6 budgets; retain light Keep wildlife.
inline TArray<int32> KeepEncounter(int32 Depth,int32 Wave=1)
{
    // Later gauntlet mixes established specialists without stacking elites.
    if(Depth>=5){
        if(Wave>1)return {49,50,49,52,4,49};
        return {49,50,Depth>=7?53:51,49,4,49};
    }
    if(Wave>1&&Depth==2)return {49,49,50,4,2};
    if(Wave>1&&Depth>=3)return {49,49,50,4,2,49};
    if(Depth<=1)return {49,49,50,4};
    if(Depth==2)return {49,51,49,50,4};
    if(Depth==3)return {49,52,49,2,49,4};
    return {49,53,49,50,4,49};
}
// Theme-matched campaign encounters. Depth is graph distance, never visit count;
// late specialists replace slots rather than inflating simultaneous enemy counts.
inline TArray<int32> AtlasEncounter(int32 Biome,int32 Depth,int32 Wave)
{
    if(Biome==7){
        if(Depth==1)return {37,38,40,41};
        if(Depth<4)return {78,70,40,37,61};
        if(Depth<6)return {70,55,61,78,40,59};
        return {70,61,Wave==1?(Depth%2?62:58):55,78,40,59};
    }
    if(Biome==0)return KeepEncounter(Depth,Wave);
    const int Bases[]={0,6,12,18,43,37,31};
    const int Base=Bases[FMath::Clamp(Biome,0,6)];
    TArray<int32> Result;
    const int Count=Depth<=1?4:Depth<=3?5:6;
    for(int I=0;I<Count;++I)Result.Add(Base+(I+Depth+Wave-2)%6);
    if(Depth<2)return Result;
    // Light melee/ranged introductions, then casters, then heavy/elite guards.
    const int Newcomers[][4]={{49,50,52,53},{71,57,73,76},{66,65,72,68},
        {60,59,54,62},{63,65,77,75},{78,70,61,58},{32,33,35,31}};
    const auto& N=Newcomers[FMath::Clamp(Biome,0,6)];
    Result[0]=N[(Depth+Wave)%2];
    if(Depth>=4)Result[1]=N[2];
    if(Depth>=5&&Biome==3)Result[2]=55; // Bellows Brute before the Colossus.
    if(Depth>=6&&Wave==1)Result[2]=N[3]; // One late heavy per room, not per wave.
    return Result;
}
// Attack styles: slash, aimed bolt, charge, fan, slam, venom, radial volley.
inline const FDungeonSpecies Species[]={
 {TEXT("Crypt Guard"),61,44,13,48,.55f,.9f,0,false},
 {TEXT("Grave Archer"),43,32,10,280,.85f,1.7f,1,false},
 {TEXT("Carrion Bat"),101,22,8,160,.65f,1.2f,2,true},
 {TEXT("Lantern Wisp"),48,25,9,240,.9f,1.6f,1,true},
 {TEXT("Tomb Rat"),93,24,8,40,.38f,.85f,0,false},
 {TEXT("Bone Hound"),83,38,14,160,.8f,1.3f,2,false},
 {TEXT("Webspinner"),55,40,11,220,.9f,1.8f,5,false},
 {TEXT("Thorn Crawler"),73,44,15,60,.65f,1.1f,0,false},
 {TEXT("Mire Toad"),38,58,13,250,1.f,2.f,5,false},
 {TEXT("Spore Moth"),61,29,9,200,.8f,2.1f,3,true},
 {TEXT("Rootling"),40,67,17,75,1.f,1.6f,4,false},
 {TEXT("Bog Shaman"),45,37,12,280,1.f,2.2f,3,false},
 {TEXT("Frost Knight"),53,72,20,82,.8f,1.4f,0,false},
 {TEXT("Shard Imp"),84,31,11,260,.6f,1.4f,1,false},
 {TEXT("Snow Harpy"),92,39,15,190,.75f,1.5f,2,true},
 {TEXT("Crystal Beetle"),49,83,21,180,1.f,1.8f,2,false},
 {TEXT("Ice Wraith"),46,41,12,260,1.2f,2.4f,6,true},
 {TEXT("Rime Wolf"),96,49,17,175,.7f,1.4f,2,false},
 {TEXT("Ash Raider"),65,69,20,65,.7f,1.2f,0,false},
 {TEXT("Ember Drake"),69,55,14,230,1.f,2.f,3,true},
 {TEXT("Furnace Slime"),37,85,18,95,.95f,1.7f,4,false},
 {TEXT("Cinder Witch"),48,46,16,280,1.f,2.3f,6,false},
 {TEXT("Iron Scorpion"),72,76,22,105,.85f,1.5f,5,false},
 {TEXT("Lava Brute"),35,120,27,120,1.2f,2.f,4,false},
 {TEXT("FINANCE GUY"),420,760,36,300,1.1f,1.6f,7,false},
 {TEXT("THE WEBROOT MATRIARCH"),445,940,34.5f,320,1.1f,1.6f,5,false},
 {TEXT("THE RIME EMPRESS"),460,1060,31.5f,340,1.3f,1.9f,6,true},
 {TEXT("THE CINDER WARDEN"),410,1320,54,145,1.3f,1.7f,4,false},
 {TEXT("BIG MACK"),420,900,32,440,.8f,1.2f,8,false},
 {TEXT("TWISTER"),425,1600,16.8f,500,.65f,1.4f,9,true},
 {TEXT("FLASH BANG GUY"),220,1450,20,360,.85f,2.6f,10,false},
 {TEXT("Patty Brute"),48,100,18,100,1.15f,1.7f,4,false},
 {TEXT("Fry Skitter"),102,34,9,160,.8f,1.3f,2,false},
 {TEXT("Pickle Lobber"),51,46,12,300,1.f,2.1f,5,false},
 {TEXT("Onion Bat"),80,32,10,230,.9f,1.8f,3,true},
 {TEXT("Cleaver Cook"),68,64,17,76,.7f,1.2f,0,false},
 {TEXT("Soda Imp"),66,40,11,265,.85f,1.6f,1,false},
 {TEXT("Rifle Trooper"),62,60,11,370,.95f,1.65f,1,false},
 {TEXT("Shield Breacher"),55,110,15,185,1.f,1.8f,2,false},
 {TEXT("Flash Cadet"),62,46,10,320,1.25f,4.6f,5,false},
 {TEXT("Scout Drone"),90,38,8,340,1.1f,2.f,3,true},
 {TEXT("Shock Trooper"),73,72,15,110,.95f,1.6f,4,false},
 {TEXT("Mortar Engineer"),42,70,22,390,1.4f,2.8f,5,false},
 {TEXT("Tempest Wisp"),86,55,13,300,.85f,1.5f,1,true},
 {TEXT("Thunder Roc"),108,72,17,205,1.05f,1.6f,2,true},
 {TEXT("Storm Knight"),64,115,24,100,.95f,1.6f,0,false},
 {TEXT("Static Spider"),75,80,14,280,1.1f,2.f,3,false},
 {TEXT("Rubble Golem"),40,170,30,140,1.5f,2.2f,4,false},
 {TEXT("Cyclone Imp"),74,63,12,260,1.3f,2.3f,6,true},
 {TEXT("Rustblade Squire"),61,65,14,80,.51f,.32f,0,false},
 {TEXT("Graveglass Slinger"),52,48,11,300,.65f,.9f,1,false},
 {TEXT("Chainbound Bailiff"),46,110,20,90,.9f,1.2f,0,false},
 {TEXT("Candle Hexer"),45,76,15,300,.9f,1.2f,1,true},
 {TEXT("Sepulcher Lancer"),43,175,26,310,1.2f,1.6f,1,false}
};
inline const FDungeonSpecies& Get(int32 I) {
    if(I==IronMatriarch::Species){static const FDungeonSpecies Iron={TEXT("IRON MATRIARCH"),0,IronMatriarch::BaseHealth,IronMatriarch::SlamDamage,230,2,1.7f,11,false};return Iron;}
    if(DungeonExpansionV2::Is(I)){
        static const TArray<FDungeonSpecies> Expanded=[]{
            TArray<FDungeonSpecies> Result;
            for(const auto& P:DungeonExpansionV2::Profiles){
                const int Id=DungeonExpansionV2::First+Result.Num();
                const float MoveSpeed=Id==75?82.f*1.5f:P.Elite?82.f:P.ProjectileSpeed>0?48.f:68.f;
                Result.Add({P.Name,MoveSpeed,P.Health,P.Damage,P.ProjectileSpeed>0?310.f:95.f,P.Windup,P.Recovery,P.ProjectileSpeed>0?1:0,false});
            }
            return Result;
        }();
        return Expanded[I-DungeonExpansionV2::First];
    }
    return Species[FMath::Clamp(I,0,int32(UE_ARRAY_COUNT(Species))-1)];
}
inline float RenderSize(int32 I)
{
    if(I==IronMatriarch::Species)return IronMatriarch::DrawSize;
    if(I==RustbladeSquire::Species)return RustbladeSquire::RenderSize;
    if(DungeonExpansion::Is(I))return 180.f; // Receive-hit volume, not padded 512px art canvas.
    const float Sizes[]={178,168,112,132,110,190,210,195,220,160,240,178,205,145,210,220,185,210,190,235,215,180,235,260,225,350,330,380,320,330,205,240,145,165,180,200,150,190,215,180,155,195,190,170,240,230,205,285,170};
    return Sizes[FMath::Clamp(I,0,48)];
}
inline const TCHAR* Biome(int32 I) { const TCHAR* N[]={TEXT("THE FORGOTTEN KEEP"),TEXT("WEBROOT HOLLOWS"),TEXT("GLACIAL RELIQUARY"),TEXT("CINDER FOUNDRY"),TEXT("STORMBREACH CITADEL"),TEXT("BLACKOUT BUNKER"),TEXT("THE GREASEWORKS"),TEXT("THE IRON AERIE")}; return N[FMath::Clamp(I,0,7)]; }
}
