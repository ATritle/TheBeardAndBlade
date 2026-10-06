#pragma once
#include "CoreMinimal.h"
namespace DungeonBow {
constexpr int First=72,Count=5,ShotStyle=30;
constexpr float Duration=1.f,Release=.65f,Speed=1520.f,Life=.7f;
constexpr float ChargeTime=1.5f,PowerDamage=2.f;
inline bool Is(int Catalog){return Catalog>=First&&Catalog<First+Count;}
inline int Element(int Effect){return Effect==11?1:Effect==3?2:Effect==2?3:Effect==7?4:0;}
inline int Frame(float T){return T<.12f?0:T<.28f?1:T<.46f?2:T<Release?3:T<.80f?4:5;}
inline FLinearColor Color(int E){const FLinearColor C[]={FLinearColor(1,1,1),FLinearColor(1,.23f,.035f),FLinearColor(.15f,.65f,1),FLinearColor(.3f,1,.06f),FLinearColor(.55f,.25f,1)};return C[FMath::Clamp(E,0,4)];}
}
