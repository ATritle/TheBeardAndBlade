#pragma once
#include "CoreMinimal.h"

namespace RustbladeSquire
{
inline constexpr int32 Species=49;
// ~120px standing body vs the adventurer's ~132px at normal gameplay scale.
inline constexpr float RenderSize=210.f;
inline constexpr float RootY=340.f/384.f;
inline constexpr float AttackFrameTime=.085f;
inline constexpr int32 AttackFrames=13;
inline constexpr float StrikeTime=6*AttackFrameTime;
inline constexpr float AttackDuration=AttackFrames*AttackFrameTime;
inline constexpr float HurtDuration=.28f;
inline constexpr float DeathFrameTime=.13f;
inline constexpr float Stride=78.f;
inline const TCHAR* Directions[]={TEXT("N"),TEXT("NE"),TEXT("E"),TEXT("SE"),TEXT("S"),TEXT("SW"),TEXT("W"),TEXT("NW")};
inline int32 Direction(FVector2D Delta)
{
    return (FMath::RoundToInt(FMath::Atan2(Delta.Y,Delta.X)/(PI/4.f))+2+8)%8;
}
inline FVector2D Aim(int32 Direction)
{
    const float A=(Direction-2)*PI/4.f;
    return FVector2D(FMath::Cos(A),FMath::Sin(A));
}
inline FString Art(int32 Direction,const TCHAR* State,int32 Frame)
{
    return FString::Printf(TEXT("Rustblade_%s_%s_%02d"),Directions[FMath::Clamp(Direction,0,7)],State,Frame);
}
inline bool CrossedStrike(float Previous,float Current)
{ return Previous<StrikeTime && Current>=StrikeTime; }
// Ground-space sword sweep, deliberately independent of artwork dimensions.
// Contact never causes damage; only this single authored attack event does.
inline bool Hits(FVector2D Delta,FVector2D LockedAim)
{
    const float Forward=FVector2D::DotProduct(Delta,LockedAim);
    const float Side=FMath::Abs(Delta.X*LockedAim.Y-Delta.Y*LockedAim.X);
    return Forward>=-8.f && Forward<=92.f && Side<=32.f && Delta.SizeSquared()<=96.f*96.f;
}
inline int32 WalkFrame(float Distance) { return FMath::FloorToInt(Distance/Stride*8.f)%8; }
inline int32 AttackFrame(float Age) { return FMath::Clamp(FMath::FloorToInt(Age/AttackFrameTime),0,AttackFrames-1); }
inline int32 HurtFrame(float Age) { return FMath::Clamp(FMath::FloorToInt(Age/HurtDuration*4),0,3); }
inline int32 DeathFrame(float Age) { return FMath::Clamp(FMath::FloorToInt(Age/DeathFrameTime),0,7); }
}
