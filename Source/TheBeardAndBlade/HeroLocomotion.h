#pragma once
#include "CoreMinimal.h"

// Timing for the authored walk/run cycles. Never crossfade whole sprites:
// it creates translucent double limbs instead of genuine in-between poses.
namespace HeroLocomotion
{
inline float Blend(float Current,float Target,float Dt,float Rate)
{ return FMath::Lerp(Current,Target,1.f-FMath::Exp(-Rate*FMath::Max(0.f,Dt))); }
constexpr float GroundDepth=.6f;
constexpr int32 GroundedFrames=32;
inline float ProjectionScale(FVector2D Travel)
{
    Travel=Travel.GetSafeNormal();
    return Travel.IsNearlyZero()?1.f:1.f/FMath::Sqrt(FMath::Square(Travel.X)+FMath::Square(Travel.Y/GroundDepth));
}
inline float Stride(float Run,FVector2D Travel=FVector2D(1,0))
{ return FMath::Lerp(112.f,210.f,FMath::Clamp(Run,0.f,1.f))*ProjectionScale(Travel); }
}
