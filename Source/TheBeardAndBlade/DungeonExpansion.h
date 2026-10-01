#pragma once
#include "CoreMinimal.h"
#include "RustbladeSquire.h"
#include "DungeonExpansionV2.h"

namespace DungeonExpansion
{
inline constexpr int First=50,Last=78;
inline bool Is(int S) { return S>=First&&S<=Last; }
inline const TCHAR* Names[]={TEXT("GraveglassSlinger"),TEXT("ChainboundBailiff"),TEXT("CandleHexer"),TEXT("SepulcherLancer")};
inline constexpr float RenderSize=290.f,RootY=450.f/512.f;
inline constexpr int WalkFrames=8,AttackFrames=16,IdleFrames=4,HurtFrames=4,DeathFrames=8;
inline float Windup(int S) {return DungeonExpansionV2::Is(S)?DungeonExpansionV2::Get(S).Windup:S==50?.65f:S==53?1.2f:.9f;}
inline float Recovery(int S) {return DungeonExpansionV2::Is(S)?DungeonExpansionV2::Get(S).Recovery:S==50?.9f:S==53?1.6f:1.2f;}
inline const TCHAR* Folder(int S){return DungeonExpansionV2::Is(S)?DungeonExpansionV2::Get(S).Folder:Names[FMath::Clamp(S-First,0,3)];}
inline float Size(int S){return DungeonExpansionV2::Is(S)?DungeonExpansionV2::RenderSize*(S==68||S==75?1.3f*1.15f:DungeonExpansionV2::Get(S).Elite?1.12f:1.f):RenderSize;}
inline float Root(int S){return DungeonExpansionV2::Is(S)?DungeonExpansionV2::RootY:RootY;}
inline int StableFacing(int Current,FVector2D Move){
    if(Move.IsNearlyZero())return Current;
    // Hold the current view through a small overlap at eight-way boundaries.
    if(FVector2D::DotProduct(Move.GetSafeNormal(),RustbladeSquire::Aim(Current))>.8660254f)return Current;
    return RustbladeSquire::Direction(Move);
}
inline int Count(int S,const FString& State){return DungeonExpansionV2::Is(S)?DungeonExpansionV2::Count(State):State==TEXT("idle")||State==TEXT("hurt")?4:State==TEXT("walk")||State==TEXT("death")?8:16;}
inline float Duration(int S) {return Windup(S)+Recovery(S);}
// Authored release pose is index 8; the second half is exposed recovery.
inline int AttackFrame(int S,float Age) {
    const float W=Windup(S);
    return Age<W?FMath::Clamp(FMath::FloorToInt(Age/W*8),0,7):FMath::Clamp(8+FMath::FloorToInt((Age-W)/Recovery(S)*8),8,15);
}
inline bool Crossed(int S,float Before,float After) {return Before<Windup(S)&&After>=Windup(S);}
inline FString Art(int S,int D,const TCHAR* State,int F) {
    return FString::Printf(TEXT("%s_%s_%s_%02d"),Folder(S),RustbladeSquire::Directions[FMath::Clamp(D,0,7)],State,F);
}
inline FString FX(int S,const TCHAR* State,int F) {
    return FString::Printf(TEXT("%s_FX_%s_%02d"),Folder(S),State,F);
}
inline FString Path(const FString& Name) {
    for(const auto& P:DungeonExpansionV2::Profiles)if(Name.StartsWith(FString(P.Folder)+TEXT("_")))
        return FString::Printf(TEXT("/Game/Art/EnemyExpansion/%s/%s.%s"),P.Folder,*Name,*Name);
    for(const auto* N:Names)if(Name.StartsWith(FString(N)+TEXT("_")))
        return FString::Printf(TEXT("/Game/Art/EnemyExpansion/%s/%s.%s"),N,*Name,*Name);
    return FString();
}
// Collision uses logical ground space, independent of long weapons/art canvas.
inline bool MeleeHit(int S,FVector2D Delta,FVector2D Aim) {
    const float Along=FVector2D::DotProduct(Delta,Aim),Side=FMath::Abs(Delta.X*Aim.Y-Delta.Y*Aim.X);
    return Along>=0&&Along<=(S==53?120.f:100.f)&&Side<=(S==53?25.f:38.f);
}
inline float CircleHit(FVector2D From,FVector2D To,FVector2D Center,float Radius) {
    const auto V=To-From,Q=From-Center;const double A=V.SizeSquared(),C=Q.SizeSquared()-Radius*Radius;
    if(C<=0)return 0;
    if(A<1.e-8)return 2;
    const double B=2*FVector2D::DotProduct(Q,V),Disc=B*B-4*A*C;
    if(Disc<0)return 2;
    const double T=(-B-FMath::Sqrt(Disc))/(2*A);
    return T>=0&&T<=1?float(T):2.f;
}
inline float WallHit(FVector2D From,FVector2D To) {
    if(From.X<65||From.X>1215||From.Y<145||From.Y>755)return 0;
    float T=2;const auto V=To-From;
    if(To.X<65)T=FMath::Min(T,float((65-From.X)/V.X));
    if(To.X>1215)T=FMath::Min(T,float((1215-From.X)/V.X));
    if(To.Y<145)T=FMath::Min(T,float((145-From.Y)/V.Y));
    if(To.Y>755)T=FMath::Min(T,float((755-From.Y)/V.Y));
    return T;
}
// Projectile-only swept vertical capsule: torso through feet, not melee/contact.
inline float PlayerBodyHit(FVector2D From,FVector2D To,FVector2D Feet,float ProjectileRadius){
    const float R=22.f+ProjectileRadius;
    const auto Top=Feet-FVector2D(0,88),Bottom=Feet-FVector2D(0,20);
    float Hit=FMath::Min(CircleHit(From,To,Top,R),CircleHit(From,To,Bottom,R));
    const auto V=To-From;float Enter=0,Leave=1;
    for(int Axis=0;Axis<2;++Axis){
        const double P=Axis?From.Y:From.X,D=Axis?V.Y:V.X;
        const double Lo=Axis?Top.Y:Feet.X-R,Hi=Axis?Bottom.Y:Feet.X+R;
        if(FMath::Abs(D)<1.e-8){if(P<Lo||P>Hi)return Hit;}
        else{float A=(Lo-P)/D,B=(Hi-P)/D;if(A>B)Swap(A,B);Enter=FMath::Max(Enter,A);Leave=FMath::Min(Leave,B);}
    }
    if(Enter<=Leave&&Leave>=0&&Enter<=1)Hit=FMath::Min(Hit,Enter);
    return Hit;
}
inline FVector2D ShotVisualPosition(int S,FVector2D P,float Age,float Flight){
    if(DungeonExpansionV2::Is(S)){
        P.Y-=55.f;
        if(DungeonExpansionV2::Get(S).Arc)P.Y-=FMath::Sin(FMath::Clamp(Age/FMath::Max(.01f,Flight),0.f,1.f)*PI)*75.f;
    }
    return P;
}
}
