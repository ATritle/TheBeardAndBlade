#pragma once
#include "DungeonActors.h"
#include "DungeonArrowFX.h"

// Cosmetic only: no gameplay RNG, hit tests, damage or lifetime mutation.
namespace DungeonSkillFX {
inline FLinearColor Color(int S) {
 if(S==57||S==71||S==73||S==76||S==36)return {.35f,.8f,.16f};
 if(S==65||S==66||S==68||S==72||S==31||S==32||S==33||S==34)return {.3f,.65f,1};
 if(S==61||S==63||S==75||S==77||S>=43&&S<=48)return {.6f,.38f,1};
 if(S==54||S==55||S==59||S==60||S==62||S>=16&&S<=19||S==41||S==42)return {1,.33f,.07f};
 return {.8f,.65f,.4f};
}
inline void Burst(UWorld* W,FVector2D P,FLinearColor C,float Size,float Life,int Phase,float Power=1) {
 if(auto* F=ADungeonArrowFX::Find(W,true))F->EmitStyled(P,{1,0},C,Size,Life,Phase,Power);
}
inline FLinearColor ShotColor(const FDungeonShot& S) {
 if(S.bFriendly)return {1,.65f,.2f};
 if(S.ExpansionSpecies>=0)return Color(S.ExpansionSpecies);
 if(S.SourceEnemy.IsValid())return Color(S.SourceEnemy->Species);
 return S.Art==4||S.Art==40?FLinearColor(.3f,.65f,1):S.Art==7||S.Art==45?FLinearColor(.6f,.38f,1):S.Art==6||S.Art==42?FLinearColor(.35f,.8f,.16f):FLinearColor(1,.4f,.12f);
}
inline void Trail(UWorld* W,FDungeonShot& S,FVector2D A,FVector2D B) {
 if(S.bBossAttack)return;
 const float Length=FVector2D::Distance(A,B);if(Length<.001f)return;
 const float Spacing=S.Style==11?28.f:19.f;
 auto* F=ADungeonArrowFX::Find(W,true);if(!F)return;
 for(float D=Spacing-S.EffectDistance;D<=Length;D+=Spacing)
  F->EmitStyled(FMath::Lerp(A,B,D/Length),(B-A).GetSafeNormal(),ShotColor(S),S.bFriendly?38:68,S.Style==11?.18f:.38f,0,S.bFriendly?.35f:.7f);
 S.EffectDistance=FMath::Fmod(S.EffectDistance+Length,Spacing);
}
inline bool Impact(UWorld* W,const FDungeonShot& S,FVector2D P) {
 if(S.bBossAttack)return false;
 const auto C=ShotColor(S);
 const int Phase=S.bFriendly?7:S.Style==11||S.ExpansionSpecies==70?17:C.G>C.R&&C.G>C.B?15:C.B>C.R&&C.G>C.R?14:C.B>C.G?16:C.R>.9f?13:12;
 Burst(W,P,C,S.bFriendly?150:S.Style==11?65:FMath::Clamp(100+S.BlastRadius*.45f,100.f,190.f),S.bFriendly?.6f:.55f,Phase);
 return true;
}
inline void Melee(ADungeonEnemy* E) {
 if(!E||E->bBoss)return;
 if(auto* F=ADungeonArrowFX::Find(E->GetWorld(),true))F->EmitStyled(DungeonView::Project(E->GetActorLocation())-FVector2D(0,42),E->ChargeAim.IsNearlyZero()?(E->AttackTarget-DungeonView::Project(E->GetActorLocation())).GetSafeNormal():E->ChargeAim,Color(E->Species),140,.28f,3,.65f);
}
}
