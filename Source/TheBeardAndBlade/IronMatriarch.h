#pragma once
#include "CoreMinimal.h"

namespace IronMatriarch
{
inline constexpr int Species=79,Biome=7;
// First-playtest balance. Raw base health still uses the campaign's SpawnHealth.
inline constexpr float BaseHealth=1800,DrawSize=430,SlamDamage=85,BreathDamage=12,MeteorDamage=55;
inline constexpr float SlamImpact=2.f,SlamEnd=3.1f,SlamRadius=115,FlightHeight=110;
inline constexpr float BreathStart=1.2f,BreathEnd=3.2f,BreathFinish=4.f,BreathInterval=.4f,BreathRange=230;
inline constexpr float MeteorRelease=1.4f,MeteorWarning=1.15f,MeteorFall=.45f,MeteorCadence=.24f,MeteorRadius=110,MeteorEnd=5.f;
inline float MeteorSplash(FVector2D Delta){const float R=FVector2D(Delta.X,Delta.Y/.65).Size();return R>MeteorRadius?0:MeteorDamage*FMath::Lerp(1.f,.25f,FMath::Clamp((R-35.f)/(MeteorRadius-35.f),0.f,1.f));}
inline float FlameMultiplier(float Exposure){return 1.f+FMath::Clamp(Exposure/BreathInterval,0.f,4.f)*.5f;}
inline constexpr int MeteorCount=5;
inline int View(FVector2D Delta){return FMath::Abs(Delta.X)<110?0:Delta.X<0?1:2;}
inline const TCHAR* ViewName(int V){return V==1?TEXT("left"):V==2?TEXT("right"):TEXT("front");}
inline bool Crossed(float Before,float Now,float At){return Before<At&&Now>=At;}
inline int Frame(int Attack,float T){
    if(Attack==0)return T<SlamImpact?FMath::Clamp(int(T/SlamImpact*12),0,11):FMath::Clamp(12+int((T-SlamImpact)/(SlamEnd-SlamImpact)*4),12,15);
    if(Attack==1)return T<BreathStart?FMath::Clamp(int(T/BreathStart*5),0,4):T<BreathEnd?5+int((T-BreathStart)*8)%4:FMath::Clamp(9+int((T-BreathEnd)/(BreathFinish-BreathEnd)*3),9,11);
    return T<MeteorRelease?FMath::Clamp(int(T/MeteorRelease*7),0,6):FMath::Clamp(7+int((T-MeteorRelease)*2.5f),7,11);
}
inline FVector2D FloorTarget(FVector2D P){return {FMath::Clamp(P.X,200.,1080.),FMath::Clamp(P.Y,285.,650.)};}
// Match the floor-projected warning ellipse exactly, including its shorter Y axis.
inline bool GroundHit(FVector2D Delta,float Radius){return Delta.X*Delta.X+FMath::Square(Delta.Y/.65)<=Radius*Radius;}
inline bool BreathHits(FVector2D Delta,FVector2D Aim){const float Along=FVector2D::DotProduct(Delta,Aim);return Along>=0&&Along<=BreathRange&&FMath::Abs(Delta.X*Aim.Y-Delta.Y*Aim.X)<=30+Along*.42f;}
struct FMeteor {FVector2D Target;float ImpactAt=0;bool Hit=false;};
struct FState {
    int Attack=-1,View=0,Serial=0,BreathTicks=0,Landings=0,MeteorHits=0;
    float Age=0,Cooldown=1.5f,DeathAge=-1,FlameExposure=0;
    bool Released=false;
    FVector2D From,Target,Aim=FVector2D(0,1);
    TArray<FMeteor> Meteors;
    void Cancel(){Attack=-1;Age=0;Released=true;Meteors.Empty();Cooldown=1.5f;}
};
}
