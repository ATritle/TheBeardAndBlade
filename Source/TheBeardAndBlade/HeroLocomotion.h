#pragma once
#include "CoreMinimal.h"

// Timing for the authored walk/run cycles. Never crossfade whole sprites:
// it creates translucent double limbs instead of genuine in-between poses.
namespace HeroLocomotion
{
inline float Blend(float Current,float Target,float Dt,float Rate)
{ return FMath::Lerp(Current,Target,1.f-FMath::Exp(-Rate*FMath::Max(0.f,Dt))); }
inline float Stride(float Run) { return FMath::Lerp(144.f,224.f,FMath::Clamp(Run,0.f,1.f)); }
}
