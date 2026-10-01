#pragma once
#include "CoreMinimal.h"
namespace TeaV4 {
inline constexpr float FPS=10,GroundDuration=.6f,EnemyDuration=.4f;
inline int Frame(float Age,int Count){return FMath::Clamp(int(FMath::FloorToFloat(FMath::Max(0.f,Age)*FPS+.00001f)),0,Count-1);}
inline bool Alive(float Life){return Life>0.00001f;}
}
