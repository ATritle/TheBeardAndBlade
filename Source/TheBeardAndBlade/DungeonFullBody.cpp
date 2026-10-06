#include "DungeonActors.h"
#include "FullBodyArtMetrics.h"
#include "HeroSockets.h"
#include "AthleticHeroSockets.h"
#include "DungeonBow.h"
#include "HeroMeleeRig.h"
#include "Engine/World.h"

namespace {
int EventFrame(float Progress,float Event)
{
    Progress=FMath::Clamp(Progress,0.f,.9999f);
    return Progress<Event?FMath::Clamp(int(Progress/Event*4),0,3):FMath::Clamp(4+int((Progress-Event)/(1-Event)*4),4,7);
}
}

bool ADungeonHUD::FullBodyHero(ADungeonHero* H,FVector2D P,float HS,FLinearColor Fade)
{
    using namespace FullBodyArt;
    int C=Idle,D=H->GetFacingDirection(),F=0;
    float FrameFraction=0;
    const float Now=GetWorld()->GetTimeSeconds();
    const bool Bow=H->IsBowEquipped();
    if(H->IsAttacking()){
        const float Progress=H->GetAttackProgress();
        if(Progress<FullBodyLastAttack)++FullBodySwing;
        FullBodyLastAttack=Progress;
        C=Bow?BowDrawFire:H->IsComboSwing()?MeleeCombo:(FullBodySwing%2?MeleeSlash:MeleeBackhand);
        F=EventFrame(Progress,Bow?DungeonBow::Release:H->IsComboSwing()?.56f:.4166667f);
        const float Event=Bow?DungeonBow::Release:H->IsComboSwing()?.56f:.4166667f;
        FrameFraction=FMath::Frac(Progress<Event?Progress/Event*4:4+(Progress-Event)/(1-Event)*4);
    }else{
        FullBodyLastAttack=1;
        if(H->IsWalking()){C=Bow?(H->IsRunAnimation()?BowRun:BowWalk):(H->IsRunAnimation()?Run:Walk);D=H->GetTravelDirection();F=H->GetLocomotionFrame();FrameFraction=FMath::Frac(H->WalkCycle()/ (2*PI)*8);}
        else{C=Bow?BowIdle:Idle;F=int(Now*6)%8;}
    }
    if(H->IsBowCharging()){C=BowDrawFire;D=H->GetFacingDirection();F=FMath::Clamp(int(H->BowDrawTime/.12f),0,3);}
    if(H->GetGuardBlend()>.01f&&!H->IsAttacking()&&!H->IsCasting()){
        C=Bow?BowBlock:Block;D=H->GetFacingDirection();
        F=H->GetBlockImpact()>0?5:H->GetGuardBlend()<.8f?FMath::Clamp(int(H->GetGuardBlend()*4),0,2):3+int(Now*4)%2;
    }
    if(H->HurtTime>0&&!H->IsAttacking()&&!H->IsCasting()&&!H->IsWalking()&&!H->IsBlocking()) {C=Hurt;F=FMath::Clamp(int((1-H->HurtTime/.25f)*8),0,7);}
    if(H->IsCasting()){C=TeaThrow;D=H->GetFacingDirection();F=EventFrame(H->GetCastProgress(),.4583333f);}
    if(H->IsRolling()){C=Roll;D=H->GetRollDirection();F=FMath::Clamp(int(H->RollProgress()*8),0,7);}
    if(H->IsDrinking()){
        C=H->IsDrinkingPotion()?PotionDrink:GoldenTeaDrink;
        F=FMath::Clamp(int((H->IsDrinkingPotion()?1-H->PotionSip/ADungeonHero::PotionDrinkDuration:H->GetTeaSipProgress())*8),0,7);
    }
    if(H->Health<=0){C=DeathRecover;F=4;}
#if !UE_BUILD_SHIPPING
    if(MeleeReviewClip>=0){C=MeleeReviewClip;D=MeleeReviewDirection;F=MeleeReviewFrame;FrameFraction=MeleeReviewFraction;}
#endif
    D=FMath::Clamp(D,0,7);F=FMath::Clamp(F,0,7);
    const int Index=C*8+D;
    FString Art=Names[Index];
    if(D==5&&(C==Idle||C==Block))Art=C==Idle?TEXT("MeleeIdle_SW"):TEXT("MeleeBlock_SW");
    auto* T=Texture(Art);if(!T)return false;
    const float K=.5f*HS;
    const auto& S=Sockets[Index][F];
    const FVector2D TL=P-FVector2D(128,232)*K;
    const float U=(F%4)*.25f,V=(F/4)*.5f;
    Shadow(P,28*HS/1.375f,Fade.A);
    if(H->IsBowCharging()){
        const auto Color=DungeonBow::Color(DungeonBow::Element(H->Equipment[0].Effect));
        Box(P.X-24,P.Y+8,48,4,FLinearColor(.04f,.05f,.04f,.85f));
        Box(P.X-24,P.Y+8,48*H->GetBowCharge(),4,H->GetBowCharge()>=1?FLinearColor(1,.83f,.25f):Color);
        if(H->GetBowCharge()>=1)Ring(P-FVector2D(0,65),26+3*FMath::Sin(Now*9),FLinearColor(1,.8f,.2f,.65f),2);
    }
    auto Body=[&](FVector2D Extra,FLinearColor Tint){
        DrawTexture(T,Offset.X+(TL.X+Extra.X)*Scale,Offset.Y+(TL.Y+Extra.Y)*Scale,
            256*K*Scale,256*K*Scale,U,V,.25f,.5f,Tint,BLEND_Translucent);
    };
    if(H->IsTeaEmpowered())for(int I=0;I<8;++I){
        const float A=I*PI/4,R=HS*(2+.6f*FMath::Sin(Now*4*PI));
        Body(FVector2D(FMath::Cos(A),FMath::Sin(A))*R,FLinearColor(2,1.4f,.35f,Fade.A*.15f));
    }
    const bool Weapon=HeroMeleeRig::Supports(C);
    const auto GripPose=Weapon?HeroMeleeRig::Get(C,D,F):HeroMeleeRig::Pose{S.X,S.Y,S.Angle,false};
    const bool Behind=Weapon&&GripPose.Behind;
    float WeaponAngle=GripPose.Angle;
    if(Weapon&&C!=Idle&&C!=Block){
        const int Next=(C==Walk||C==Run)?(F+1)%8:FMath::Min(F+1,7);
        const float NextAngle=HeroMeleeRig::Get(C,D,Next).Angle;
        WeaponAngle+=FMath::FindDeltaAngleDegrees(WeaponAngle,NextAngle)*FrameFraction;
    }
    auto DrawWeapon=[&](){
        if(!Weapon||H->Equipment.Num()==0||H->Equipment[0].IsEmpty())return;
        const auto& Item=H->Equipment[0];
        FVector2D Grip=Item.CatalogId<0?HeroSockets::Grip(Item.Icon)/128.:HeroSockets::CatalogGrip(Item.CatalogId);
        const bool Mirror=HeroSockets::MirrorBlade(Item.CatalogId,D);
        if(Mirror)Grip.X=1-Grip.X;
        const FVector2D Hand=TL+FVector2D(GripPose.X,GripPose.Y)*K;
        const float Size=96*HS*Item.EquippedScale();
        const float Angle=WeaponAngle+HeroMeleeRig::HandleCorrection(Item.CatalogId)*(Mirror?-1.f:1.f);
        DrawSwordTrail(H,Hand,Angle,Size*Grip.Y*.85f,Fade.A);
        Sprite(Item.Art(),Hand.X-Grip.X*Size,Hand.Y-Grip.Y*Size,Size,Size,Fade,Angle,Grip,Mirror);
    };
    if(Behind)DrawWeapon();
    Body(FVector2D::ZeroVector,H->HurtTime>0?FLinearColor(1,.45f,.4f,Fade.A):Fade);
    if(!Behind){
        DrawWeapon();
        if(Weapon){ // Restore the gripping fingers over the equipped handle.
            const FVector2D Hand=TL+FVector2D(GripPose.X-4,GripPose.Y-4)*K;
            DrawTexture(T,Offset.X+Hand.X*Scale,Offset.Y+Hand.Y*Scale,8*K*Scale,8*K*Scale,
                U+(GripPose.X-4)/1024,V+(GripPose.Y-4)/512,8.f/1024,8.f/512,Fade,BLEND_Translucent);
        }
    }
    if(Bow&&H->IsAttacking()){
        const int Element=DungeonBow::Element(H->Equipment[0].Effect);
        if(Element&&H->GetAttackProgress()<DungeonBow::Release){
            const FVector2D Tip=TL+FVector2D(S.MX,S.MY)*K;
            auto Color=DungeonBow::Color(Element);Color.A=Fade.A*.5f;
            Sprite(FString::Printf(TEXT("BowImpact_%d_0"),Element),Tip.X-10,Tip.Y-10,20,20,Color);
        }
    }
    DrawGuard(H);
    return true;
}
