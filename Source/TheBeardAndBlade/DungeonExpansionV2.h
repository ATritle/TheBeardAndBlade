#pragma once
#include "CoreMinimal.h"

// Stable indices 54..78 extend, but never renumber, the approved Keep roster.
namespace DungeonExpansionV2
{
struct FProfile {
    const TCHAR* Folder;
    const TCHAR* Name;
    float Health, Damage, Windup, Recovery, ProjectileSpeed, AreaRadius, TrackingDegrees;
    bool Hybrid, Arc, Elite;
};
inline constexpr int First=54,Last=78;
inline const FProfile Profiles[]={
    {TEXT("AshenCantor"),TEXT("Ashen Cantor"),78.f,15.f,0.9f,1.2f,230.f,0.f,60.f,false,false,false},
    {TEXT("BellowsBrute"),TEXT("Bellows Brute"),132.f,23.f,0.9f,1.2f,0.f,0.f,0.f,false,false,false},
    {TEXT("BreachHound"),TEXT("Breach Hound"),105.f,22.f,0.9f,1.2f,0.f,0.f,0.f,false,false,false},
    {TEXT("BriarSpitter"),TEXT("Briar Spitter"),48.f,10.f,0.65f,0.9f,330.f,0.f,0.f,false,false,false},
    {TEXT("BunkerBulwark"),TEXT("Bunker Bulwark"),210.f,27.f,1.2f,1.6f,270.f,65.f,0.f,true,true,true},
    {TEXT("CinderPitcher"),TEXT("Cinder Pitcher"),50.f,12.f,0.65f,0.9f,280.f,0.f,0.f,false,true,false},
    {TEXT("Coalgnash"),TEXT("Coalgnash"),68.f,15.f,0.65f,0.9f,0.f,0.f,0.f,false,false,false},
    {TEXT("CoilSaboteur"),TEXT("Coil Saboteur"),80.f,16.f,0.9f,1.2f,230.f,0.f,60.f,false,false,false},
    {TEXT("CrucibleColossus"),TEXT("Crucible Colossus"),220.f,30.f,1.2f,1.6f,270.f,65.f,0.f,true,true,true},
    {TEXT("GaleTalon"),TEXT("Gale Talon"),62.f,15.f,0.65f,0.9f,0.f,0.f,0.f,false,false,false},
    {TEXT("GlacierbackRam"),TEXT("Glacierback Ram"),200.f,28.f,1.2f,1.6f,270.f,65.f,0.f,true,true,true},
    {TEXT("HailshotGargoyle"),TEXT("Hailshot Gargoyle"),55.f,12.f,0.65f,0.9f,330.f,0.f,0.f,false,false,false},
    {TEXT("IcicleFlinger"),TEXT("Icicle Flinger"),46.f,11.f,0.65f,0.9f,280.f,0.f,0.f,false,true,false},
    {TEXT("MossmawStalker"),TEXT("Mossmaw Stalker"),105.f,20.f,0.9f,1.2f,0.f,0.f,0.f,false,false,false},
    {TEXT("PermafrostTemplar"),TEXT("Permafrost Templar"),125.f,22.f,0.9f,1.2f,0.f,0.f,0.f,false,false,false},
    {TEXT("RimeclawCub"),TEXT("Rimeclaw Cub"),60.f,14.f,0.65f,0.9f,0.f,0.f,0.f,false,false,false},
    {TEXT("RivetGunner"),TEXT("Rivet Gunner"),52.f,12.f,0.65f,0.9f,330.f,0.f,0.f,false,false,false},
    {TEXT("SilkfangSkitter"),TEXT("Silkfang Skitter"),58.f,13.f,0.65f,0.9f,0.f,0.f,0.f,false,false,false},
    {TEXT("SnowveilOracle"),TEXT("Snowveil Oracle"),76.f,13.f,0.9f,1.2f,230.f,0.f,60.f,false,false,false},
    {TEXT("SporebellWitch"),TEXT("Sporebell Witch"),75.f,9.f,0.9f,1.2f,230.f,0.f,60.f,false,false,false},
    {TEXT("StormcoilBehemoth"),TEXT("Stormcoil Behemoth"),215.f,29.f,1.2f,1.6f,270.f,65.f,60.f,true,false,true},
    {TEXT("TempestDuelist"),TEXT("Tempest Duelist"),195.f,27.f,0.9f,1.2f,0.f,0.f,0.f,false,false,true},
    {TEXT("ThornweaveSentinel"),TEXT("Thornweave Sentinel"),185.f,25.f,1.2f,1.6f,270.f,0.f,0.f,true,false,true},
    {TEXT("ThunderheadAdept"),TEXT("Thunderhead Adept"),80.f,17.f,0.9f,1.2f,230.f,0.f,60.f,false,false,false},
    {TEXT("TrenchShivver"),TEXT("Trench Shivver"),68.f,15.f,0.65f,0.9f,0.f,0.f,0.f,false,false,false},
};
inline bool Is(int S){return S>=First&&S<=Last;}
inline bool Enabled(int S){return Is(S)&&S!=56&&S!=64&&S!=67&&S!=69&&S!=74;}
inline constexpr int Active[]={54,55,57,58,59,60,61,62,63,65,66,68,70,71,72,73,75,76,77,78};
inline int NextEnabled(int S,int Step){do{S=First+(S-First+Step+25)%25;}while(!Enabled(S));return S;}
inline const FProfile& Get(int S){check(Is(S));return Profiles[S-First];}
inline constexpr float RenderSize=435.f,RootY=338.f/384.f;
inline int Count(const FString& State){
    if(State==TEXT("idle"))return 8;
    if(State==TEXT("walk"))return 16;
    if(State==TEXT("hurt"))return 6;
    if(State==TEXT("death"))return 12;
    if(State==TEXT("projectile")||State==TEXT("hero-impact"))return 12;
    if(State==TEXT("ground-impact"))return 16;
    return 24;
}
inline int ReleaseFrame(int S,int Direction,bool Ranged){
    const int D=FMath::Clamp(Direction,0,7);
    // Zero-based contact/release poses reviewed against the eight authored views.
    if(S==58){static const int Bash[]={9,9,10,10,7,9,9,10};return Ranged?9:Bash[D];}
    if(S==62){static const int Stomp[]={9,8,9,9,8,9,8,8};return Ranged?9:Stomp[D];}
    if(S==64){static const int Charge[]={9,8,8,8,8,8,7,9};return Ranged?9:Charge[D];}
    if(S==76){
        static const int Swipe[]={9,9,10,8,7,9,9,8},Cast[]={9,8,9,10,8,8,9,8};
        return Ranged?Cast[D]:Swipe[D];
    }
    if(S==74){ // Stormcoil: visible ground contact differs by viewing direction.
        static const int Slam[]={8,9,8,7,9,9,9,9};
        return Ranged?8:Slam[FMath::Clamp(Direction,0,7)];
    }
    if(S==54){static const int Cast[]={9,9,7,9,9,9,9,9};return Cast[D];}
    if(S==59){static const int Throw[]={9,10,9,10,9,10,9,10};return Throw[D];}
    if(S==66){static const int Throw[]={9,10,9,9,9,9,9,9};return Throw[D];}
    if(S==68){static const int Hammer[]={10,10,9,8,9,10,9,9};return Hammer[D];}
    if(S==60){static const int Claw[]={8,10,9,8,9,8,9,9};return Claw[D];}
    if(S==75){static const int Slash[]={9,9,9,9,9,10,9,9};return Slash[D];}
    return 9;
}
inline int AttackFrame(int S,int Direction,bool Ranged,float Age){
    const auto& P=Get(S);const int Release=ReleaseFrame(S,Direction,Ranged);
    return Age<P.Windup?FMath::Clamp(int(Age/P.Windup*Release),0,Release-1):
        FMath::Clamp(Release+int((Age-P.Windup)/P.Recovery*(24-Release)),Release,23);
}
inline bool Atlas(const FString& Name,FString& Asset,int& Frame,int& Rows){
    for(const auto& P:Profiles){
        const FString Prefix=FString(P.Folder)+TEXT("_");
        if(!Name.StartsWith(Prefix))continue;
        if(Name.EndsWith(TEXT("_atlas")))return false;
        const FString Suffix=Name.Right(2);
        if(!Suffix.IsNumeric())return false;
        Frame=FCString::Atoi(*Suffix);
        const FString Stem=Name.LeftChop(3);
        int32 Separator;Stem.FindLastChar(TEXT('_'),Separator);
        const FString State=Stem.Mid(Separator+1);
        const int Frames=Count(State);
        Frame=FMath::Clamp(Frame,0,Frames-1);Rows=(Frames+3)/4;
        Asset=Stem+TEXT("_atlas");return true;
    }
    return false;
}
}
