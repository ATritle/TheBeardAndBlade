#include "DungeonBow.h"
#include "DungeonArrowFX.h"
#include "DungeonActors.h"
#include "BowArtMetrics.h"
#include "FullBodyArtMetrics.h"
#include "DungeonRoster.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "InputKeyEventArgs.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Engine/Engine.h"
#include "NiagaraSystem.h"
#include "Materials/MaterialInterface.h"

void ADungeonGameMode::ReviewBow()
{
#if !UE_BUILD_SHIPPING
    static float Start=-1;static int Frame=-1,Element=-1;
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));if(!H)return;
    const float Now=GetWorld()->GetTimeSeconds();
    const bool Poses=FParse::Param(FCommandLine::Get(),TEXT("BowPoseReview"));
    const bool FXReview=FParse::Param(FCommandLine::Get(),TEXT("ArrowFXReview"));
    if(Start<0){
        StartGame();DisableAtlas();bMenu=false;PendingSpawns=0;CancelBossIntro();DialogueLines.Empty();BossGrace=0;
        for(auto& E:Enemies)if(IsValid(E))E->Destroy();Enemies.Empty();
        auto* E=GetWorld()->SpawnActor<ADungeonEnemy>();E->Species=0;E->Health=E->MaxHealth=100000;E->SpawnTime=0;E->Recovery=100;
        E->SetActorLocation(DungeonView::Unproject(FVector2D(920,460)));Enemies.Add(E);
        H->SetActorLocation(DungeonView::Unproject(FVector2D(440,500)));Start=Now;
        if(FXReview){GEngine->bUseFixedFrameRate=true;GEngine->FixedFrameRate=30;FApp::SetUseFixedTimeStep(true);FApp::SetFixedDeltaTime(1./30);Breakables.Empty();}
    }
    const float T=Now-Start;const int Choice=Poses?0:FMath::Min(4,int(T/(FXReview?4.f:3.f)));
    if(Choice!=Element){H->Equip(RollItem(72+Choice,Choice?4:0,1));H->AttackSpeed=1;Element=Choice;}
    if(!Poses){
        for(auto& E:Enemies)if(IsValid(E)){E->Recovery=100;E->SetActorLocation(DungeonView::Unproject(FVector2D(920,460)));}
        if(auto* PC=GetWorld()->GetFirstPlayerController()){
            int W=0,Height=0;PC->GetViewportSize(W,Height);const float Scale=FMath::Min(W/1280.f,Height/800.f);
            PC->SetMouseLocation((W-1280*Scale)/2+920*Scale,(Height-800*Scale)/2+400*Scale);
        }
        H->SetFlashReviewAim(FVector2D(1,0));H->MoveForward(!FXReview&&T>9?FMath::Sin(T*2)*.6f:0);
        if(FXReview){
            H->QuipTime=0;H->QuipCooldown=100;
            static bool Held=false;const float Local=FMath::Fmod(T,4.f);
            const bool Want=(Local>.20f&&Local<.31f)||(Local>1.f&&Local<3.2f);
            if(auto* PC=GetWorld()->GetFirstPlayerController();PC&&PC->PlayerInput){
                const auto Key=DungeonKeys::Key(DungeonKeys::Attack);
                PC->PlayerInput->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,Key,Want?IE_Pressed:IE_Released,0));
                if(auto* State=PC->PlayerInput->GetKeyState(Key))State->bDown=Want;
                if(Want&&!Held)H->GameplayAttack();if(!Want&&Held)H->GameplayAttackReleased();Held=Want;
            }
        }else if(!H->IsAttacking())H->Attack();
    }
    if(FXReview){++Frame;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/ArrowNiagara/Frame%04d.png"),Frame),false,false);if(T>20)FPlatformMisc::RequestExit(false);return;}
    const int Index=FMath::FloorToInt(T*15);
    if(Index!=Frame){Frame=Index;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/%s/Frame%04d.png"),Poses?TEXT("BowV2Poses"):TEXT("BowV2Replay"),Index),false,false);}
    if(T>(Poses?2.f:15.f))FPlatformMisc::RequestExit(false);
#endif
}

void ADungeonGameMode::LaunchArrow(ADungeonHero* H)
{
    if(!H||!H->IsBowEquipped()||H->Health<=0||IsGameplayBlocked())return;
    FDungeonShot S;S.Style=DungeonBow::ShotStyle;S.bFriendly=true;S.Life=DungeonBow::Life;S.Radius=4;
    {
        const auto& Socket=FullBodyArt::Sockets[FullBodyArt::BowDrawFire*8+H->GetFacingDirection()][4];
        S.Position=DungeonView::Project(H->GetActorLocation())+(FVector2D(Socket.MX,Socket.MY)-FVector2D(128,232))*.6875f;
    }
    auto Aim=(H->BowTarget-S.Position).GetSafeNormal();if(Aim.IsNearlyZero())Aim=H->GetAim();
    S.Velocity=Aim*DungeonBow::Speed;S.BowElement=DungeonBow::Element(H->Equipment[0].Effect);
    S.BowCritical=FMath::FRand()<H->CritChance;
    S.Damage=H->AttackPower*H->Equipment[0].DirectDamageScale()*FMath::FRandRange(.9f,1.1f)*(S.BowCritical?H->CritMultiplier:1.f);
    S.Damage*=H->BowShotPower;
    S.BowPower=H->BowShotPower>1;
    for(int E=1;E<=11;++E)if(H->HasEffect(E))S.BowEffects|=1<<E;
    S.BowBleed=H->BleedChance;S.BowPoison=H->PoisonChance;
    if(H->HasEffect(9)&&H->Health<H->MaxHealth*.4f)S.Damage*=1.3f;
    ++H->StrikeCount;S.BowEcho=H->StrikeCount%3==0;
    Shots.Add(S);PlaySound(TEXT("Throw"),.3f,1.2f);
    if(auto* FX=ADungeonArrowFX::Find(GetWorld(),true))FX->Emit(S.Position,Aim,S.BowElement,S.BowPower,1);
}

void ADungeonGameMode::UpdateArrow(FDungeonShot& S,float Dt,ADungeonHero* H)
{
    if(S.Life<=0)return;
    const auto From=S.Position,To=From+S.Velocity*FMath::Min(FMath::Max(0.f,Dt),S.Life);
    float First=DungeonExpansion::WallHit(From,To);ADungeonEnemy* Target=nullptr;
    for(auto& E:Enemies)if(IsValid(E)&&E->Health>0&&E->SpawnTime<=0){
        const auto Feet=DungeonView::Project(E->GetActorLocation());
        const float Size=DungeonRoster::RenderSize(E->Species),R=FMath::Clamp(Size*.14f,16.f,E->bBoss?65.f:38.f)+S.Radius;
        float Hit=2;
        // A swept vertical body volume covers feet, torso and head, including large elites.
        for(float Height:{.12f,.28f,.44f,.60f})Hit=FMath::Min(Hit,DungeonExpansion::CircleHit(From,To,Feet-FVector2D(0,Size*Height),R));
        if(Hit<First){First=Hit;Target=E;}
    }
    FDungeonBreakable* Prop=nullptr;
    for(auto& B:Breakables)if(B.BrokenAge<0){
        const float Hit=DungeonExpansion::CircleHit(From,To,B.Position-FVector2D(0,35),37+S.Radius);
        if(Hit<First){First=Hit;Target=nullptr;Prop=&B;}
    }
    S.Age+=Dt;S.Life-=Dt;S.Position=First<=1?FMath::Lerp(From,To,First):To;
    // Distance sampling keeps the world-space wake continuous at any frame rate.
    if(auto* FX=ADungeonArrowFX::Find(GetWorld(),true)){
        const float Distance=FVector2D::Distance(From,S.Position);const auto Direction=S.Velocity.GetSafeNormal();
        float Step=18-S.BowWakeDistance;
        for(;Step<=Distance;Step+=18)FX->Emit(From+Direction*Step,Direction,S.BowElement,S.BowPower,0);
        S.BowWakeDistance=FMath::Fmod(S.BowWakeDistance+Distance,18.f);
        if(First<=1)FX->Emit(S.Position,Direction,S.BowElement,S.BowPower,2);
    }
    if(First>1)return;
    S.Life=0;
    FDungeonSplash FX;FX.Position=S.Position;FX.Life=.32f;FX.BowElement=S.BowElement;FX.bFriendly=true;
    Splashes.Add(FX);
    if(Prop){BreakProp(*Prop);return;}
    if(!Target)return;
    const auto Has=[&](int Effect){return (S.BowEffects&(1<<Effect))!=0;};
    const float Damage=S.Damage*(Has(8)&&Target->Health<Target->MaxHealth*.3f?1.35f:1.f);
    const float Dealt=FMath::Min(Target->Health,Damage);const auto Impact=S.Position;
    if(FMath::FRand()<S.BowBleed){Target->BleedTime=4;Target->BleedDPS=Damage*.2f;}
    if(FMath::FRand()<S.BowPoison){Target->PoisonTime=6;Target->PoisonDPS=Damage*.14f;}
    if(Has(3))Target->SlowTime=3;
    const bool Leech=Has(4),SecondWind=Has(6)&&S.BowCritical,Echo=Has(7)&&S.BowEcho;
    Target->TakeDungeonDamage(Damage);
    if(Leech)H->RestoreHealth(Dealt*DungeonBalance::SignatureLeech,true);
    if(SecondWind)H->Stamina=FMath::Min(H->MaxStamina,H->Stamina+6);
    if(Echo){const auto Others=Enemies;for(auto& E:Others)if(IsValid(E)&&E!=Target&&E->Health>0&&E->SpawnTime<=0&&
        FVector2D::Distance(DungeonView::Project(E->GetActorLocation())-FVector2D(0,DungeonRoster::RenderSize(E->Species)*.35f),Impact)<85)E->TakeDungeonDamage(Damage*.35f);}
}

void ADungeonGameMode::VerifyBow()
{
#if WITH_EDITOR
    int Checks=0,Errors=0;auto Check=[&](bool OK,const TCHAR* Why){++Checks;if(!OK){++Errors;UE_LOG(LogTemp,Error,TEXT("BOW: %s"),Why);}};
    Check(LoadObject<UNiagaraSystem>(nullptr,TEXT("/Game/Effects/Arrows/NS_ArrowWisp.NS_ArrowWisp"))!=nullptr,TEXT("dedicated Niagara system exists"));
    Check(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Effects/Arrows/M_ArrowWisp.M_ArrowWisp"))!=nullptr,TEXT("animated Niagara wake material exists"));
    for(int Id=72;Id<77;++Id)for(int R=0;R<5;++R){const auto I=RollItem(Id,R,4);Check(I.IsBow()&&I.Slot==0&&I.Rarity==R&&I.Attack>0&&I.CoinValue>0,TEXT("bow rarity and weapon slot"));}
    auto Art=[&](FString N){Check(LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Art/V2/%s.%s"),*N,*N))!=nullptr,TEXT("bow art imported"));};
    for(int D=0;D<8;++D)for(int F=0;F<6;++F){Art(FString::Printf(TEXT("BowHero_%d_%d"),D,F));Art(FString::Printf(TEXT("BowUpper_%d_%d"),D,F));}
    for(int E=0;E<5;++E){Art(FString::Printf(TEXT("Loot_%d"),72+E));Art(FString::Printf(TEXT("BowFlight_%d"),E));for(int F=0;F<4;++F)Art(FString::Printf(TEXT("BowImpact_%d_%d"),E,F));}
    StartGame();DisableAtlas();bMenu=false;PendingSpawns=0;CancelBossIntro();DialogueLines.Empty();BossGrace=0;
    for(auto& E:Enemies)if(IsValid(E))E->Destroy();Enemies.Empty();
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));if(!H){Check(false,TEXT("hero exists"));FPlatformMisc::RequestExitWithStatus(false,1);return;}
    H->Equip(RollItem(72,0,1));H->AttackSpeed=1;H->CritChance=0;H->SetActorLocation(DungeonView::Unproject(FVector2D(400,450)));H->SetFlashReviewAim(FVector2D(1,0));
    Shots.Empty();H->Attack();H->Tick(.64f);Check(Shots.IsEmpty(),TEXT("no arrow before release"));H->Tick(.02f);
    Check(Shots.Num()==1&&Shots[0].Style==DungeonBow::ShotStyle,TEXT("exactly one projectile on release"));H->Tick(.2f);Check(Shots.Num()==1,TEXT("no repeated shot in follow-through"));
    Check(!H->IsComboSwing(),TEXT("bow cannot select sword finisher"));H->CancelCombatActions();Shots.Empty();
    auto* E=GetWorld()->SpawnActor<ADungeonEnemy>();E->Species=0;E->Health=E->MaxHealth=1000;E->SpawnTime=0;E->Recovery=100;E->SetActorLocation(DungeonView::Unproject(FVector2D(800,450)));Enemies.Add(E);
    auto TestShot=[&](){FDungeonShot S;S.Style=DungeonBow::ShotStyle;S.bFriendly=true;S.Position=FVector2D(600,390);S.Velocity=FVector2D(760,0);S.Damage=40;S.Life=1.4f;S.Radius=4;return S;};
    auto S=TestShot();UpdateArrow(S,.5f,H);Check(E->Health==960&&S.Life==0,TEXT("swept torso collision damages once despite large time step"));UpdateArrow(S,.5f,H);Check(E->Health==960,TEXT("consumed arrow cannot hit twice"));
    E->SetActorLocation(DungeonView::Unproject(FVector2D(800,450)));S=TestShot();S.BowEffects=1<<3;S.BowElement=2;UpdateArrow(S,.5f,H);Check(E->SlowTime==3&&Splashes.Last().BowElement==2,TEXT("frost status and matching impact"));
    E->SetActorLocation(DungeonView::Unproject(FVector2D(800,450)));S=TestShot();S.BowPoison=1;UpdateArrow(S,.5f,H);Check(E->PoisonTime==6,TEXT("venom chance applied at impact"));
    S=TestShot();S.Position=FVector2D(1180,650);UpdateArrow(S,.2f,H);Check(S.Life==0&&S.Position.X<=1215.01,TEXT("wall terminates arrow"));
    S=TestShot();S.Life=.05f;UpdateArrow(S,.1f,H);Check(S.Life<=0,TEXT("range/lifetime expires"));
    Breakables.Empty();FDungeonBreakable B;B.Position=FVector2D(720,425);Breakables.Add(B);
    E->SetActorLocation(DungeonView::Unproject(FVector2D(800,450)));const float BeforeProp=E->Health;
    S=TestShot();UpdateArrow(S,.5f,H);Check(Breakables[0].BrokenAge==0&&E->Health==BeforeProp,TEXT("nearest prop blocks arrow, breaks and cannot damage enemy behind it"));
    const auto Rolled=Breakables[0].Loot.Name;BreakProp(Breakables[0]);Check(Breakables[0].Loot.Name==Rolled,TEXT("broken prop cannot reroll loot"));Breakables.Empty();
    if(auto* PC=Cast<APlayerController>(H->GetController());PC&&PC->PlayerInput){
        const auto Key=DungeonKeys::Key(DungeonKeys::Attack);
        PC->PlayerInput->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,Key,IE_Pressed,0));auto* State=PC->PlayerInput->GetKeyState(Key);
        if(State){
            H->CancelCombatActions();H->Health=H->MaxHealth;H->StunTime=0;H->UIClickFrame=MAX_uint64;
            State->bDown=true;H->GameplayAttack();const int Before=H->StrikeCount;
            Shots.Empty();for(int I=0;I<89;++I)H->Tick(1.f/60);
            Check(H->StrikeCount==Before,TEXT("no auto shot before 1.5 seconds"));
            H->Tick(.02f);
            Check(H->StrikeCount==Before+1&&Shots.Num()==1&&Shots[0].BowPower,TEXT("1.5 second completion fires exactly one power arrow"));
            for(int I=0;I<180;++I)H->Tick(1.f/60);
            H->GameplayAttack();Check(H->StrikeCount==Before+1&&!H->IsBowCharging(),TEXT("held button and repeated press cannot rearm before release"));
            Check(H->BowShotPower==2&&Shots.Num()==1&&FMath::IsNearlyEqual(float(Shots[0].Velocity.Size()),1520.f,.1f)&&Shots[0].Radius==4,TEXT("power doubles damage and speed is 1520 without inflating collision"));
            State->bDown=false;H->GameplayAttackReleased();H->GameplayAttackReleased();Check(H->StrikeCount==Before+1,TEXT("release after automatic shot cannot fire twice"));
            H->Tick(1.1f);H->Tick(1.1f);Check(H->StrikeCount==Before+1,TEXT("no repeated bow cycles after release"));
            State->bDown=true;H->GameplayAttack();H->Tick(.08f);Shots.Empty();State->bDown=false;H->GameplayAttackReleased();
            Check(Shots.Num()==1&&!Shots[0].BowPower&&H->BowShotPower==1,TEXT("short click fires a normal arrow"));
            H->Tick(1.1f);State->bDown=true;H->GameplayAttack();H->Tick(1.3f);Shots.Empty();State->bDown=false;H->GameplayAttackReleased();
            Check(Shots.Num()==1&&!Shots[0].BowPower,TEXT("early release remains normal damage"));
            H->Tick(1.1f);State->bDown=true;H->GameplayAttack();H->Tick(1);H->ToggleInventory();Shots.Empty();State->bDown=false;H->GameplayAttackReleased();
            Check(!H->IsAttacking()&&!H->IsBowCharging()&&Shots.IsEmpty(),TEXT("inventory cancels charge without firing"));H->ToggleInventory();
            State->bDown=true;H->GameplayAttack();H->Tick(.1f);H->CancelCombatActions();State->bDown=false;H->GameplayAttackReleased();
            Check(Shots.IsEmpty(),TEXT("cancelled charge cannot release"));
        }
        else Check(false,TEXT("input state exists"));
    }
    H->CancelCombatActions();H->Equip(RollItem(0,0,1));H->Attack();Check(H->AttackDuration()==.48f,TEXT("sword timing unchanged"));
    UE_LOG(LogTemp,Display,TEXT("BOW_VERIFY: %d checks, %d errors"),Checks,Errors);FPlatformMisc::RequestExitWithStatus(false,Errors?1:0);
#endif
}
