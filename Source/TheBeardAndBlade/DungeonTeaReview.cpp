#include "DungeonActors.h"
#include "DungeonTeaVisuals.h"
#include "TeaV4Metrics.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

void ADungeonGameMode::VerifyTeaV4()
{
#if !UE_BUILD_SHIPPING
    int Checks=0,Errors=0;
    auto Check=[&](bool OK,const TCHAR* What){++Checks;if(!OK){++Errors;UE_LOG(LogTemp,Error,TEXT("TEA_V4_FAIL %s"),What);}};
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));if(!H)return;
    StartGame();CancelBossIntro();DialogueLines.Empty();BossGrace=0;AtlasArrivalTime=AtlasTravelTime=0;bAtlasMap=bTraderOpen=false;
    PendingSpawns=0;for(auto& E:Enemies)if(IsValid(E))E->Destroy();Enemies.Empty();Shots.Empty();Splashes.Empty();
    const FVector2D P(640,450);H->SetActorLocation(DungeonView::Unproject(P));H->AttackPower=20;
    Check(TeaV4::Flight[0].W<=38&&TeaV4::Ground[3].W==112&&TeaV4::Enemy[2].W==124,TEXT("hand-sized cup and reduced art are independent of damage radius"));
    for(int D=0;D<8;++D)for(float Distance:{40.f,350.f}){
        const FVector2D Target=P+FVector2D(FMath::Cos(D*PI/4),FMath::Sin(D*PI/4))*Distance;
        LaunchTea(H,Target);const auto Shot=Shots.Last();
        Check(Shot.Damage==71&&Shot.BlastRadius==135,TEXT("damage formula and radius unchanged"));
        Check(Shot.FlightTime>=.35f&&Shot.FlightTime<=.75f&&Shot.Target.Equals(DungeonView::Clamp(Target)),TEXT("all directions near/far retain target clamp and flight"));
        Shots.Empty();
    }
    H->PowerMove();Check(H->GetPowerCooldown()==10,TEXT("ten second cooldown unchanged"));H->CancelCombatActions();
    auto Spawn=[&](float X,bool Boss){auto* E=GetWorld()->SpawnActor<ADungeonEnemy>();E->Species=Boss?24:0;E->bBoss=Boss;E->SpawnTime=0;E->Health=E->MaxHealth=1000;E->SetActorLocation(DungeonView::Unproject(P+FVector2D(X,0)));E->SetActorTickEnabled(false);Enemies.Add(E);return E;};
    auto* A=Spawn(0,false);auto* B=Spawn(146,false);auto* Outside=Spawn(148,false);auto* Boss=Spawn(-158,true);auto* BossOutside=Spawn(-160,true);
    LaunchTea(H,P);auto Shot=Shots.Last();Shots.Empty();ResolveProjectile(Shot,P);
    Check(A->Health==929&&B->Health==929&&Boss->Health==929,TEXT("multiple enemies and boss hit once"));
    Check(Outside->Health==1000&&BossOutside->Health==1000,TEXT("wider art does not extend existing enemy/boss radius"));
    Check(Splashes.Num()==4,TEXT("one ground burst plus one overlay per confirmed hit"));
    UpdateProjectiles(.399f);Check(Splashes.Num()==4,TEXT("overlays still present just before 0.4"));
    UpdateProjectiles(.001f);Check(Splashes.Num()==1&&Splashes[0].TeaKind==1,TEXT("enemy overlay ends at 0.4 exactly"));
    UpdateProjectiles(.199f);Check(Splashes.Num()==1,TEXT("ground still present just before 0.6"));
    UpdateProjectiles(.001f);Check(Splashes.IsEmpty(),TEXT("ground disappears at 0.6 without aftermath"));
    Check(A->Health==929&&B->Health==929,TEXT("visual updates never repeat damage"));
    for(int N:{4,6,8})for(int F=0;F<N;++F)Check(TeaV4::Frame(F*.1f,N)==F,TEXT("ten fps frame boundaries"));
    for(float FPS:{15.f,30.f,60.f,120.f}){
        Splashes.Empty();FDungeonSplash Ground;Ground.TeaKind=1;Ground.Life=.6f;Splashes.Add(Ground);
        for(int I=0;I<FMath::RoundToInt(.6f*FPS);++I)UpdateProjectiles(1/FPS);
        Check(Splashes.IsEmpty(),TEXT("lifetime independent of frame rate"));
    }
    const TCHAR* Names[]={TEXT("Flight"),TEXT("Ground"),TEXT("Enemy")};const int Counts[]={8,6,4};
    for(int K=0;K<3;++K)for(int F=0;F<Counts[K];++F){
        const FString Name=FString::Printf(TEXT("TeaV4_%s_%d"),Names[K],F);
        Check(LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Art/TeaV4/%s.%s"),*Name,*Name))!=nullptr,TEXT("dedicated frame loads"));
    }
    LaunchTea(H,P);ResolveProjectile(Shots.Last(),P);FinishRun(false);UpdateEnding(0);
    Check(Shots.IsEmpty()&&Splashes.IsEmpty(),TEXT("death teardown cancels tea visuals"));
    UE_LOG(LogTemp,Display,TEXT("TEA_V4_VERIFY_COMPLETE checks=%d errors=%d"),Checks,Errors);
    FPlatformMisc::RequestExitWithStatus(false,Errors?1:0);
#endif
}

void ADungeonGameMode::ReviewTeaV4(float Dt)
{
#if !UE_BUILD_SHIPPING
    const bool Auto=FParse::Param(FCommandLine::Get(),TEXT("TeaV4Review"));
    if(!Auto&&!FParse::Param(FCommandLine::Get(),TEXT("TeaV4Test")))return;
    static bool Started=false,HeldPreview=false;static float Time=0,Next=2;static int Frame=-1,CastCount=0;
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));if(!H)return;
    auto* PC=UGameplayStatics::GetPlayerController(this,0);
    if(!Started||(PC&&PC->WasInputKeyJustPressed(EKeys::R))){
        Started=true;HeldPreview=false;Time=0;Next=2;CastCount=0;StartGame();CancelBossIntro();DialogueLines.Empty();BossGrace=0;AtlasArrivalTime=AtlasTravelTime=0;bAtlasMap=bTraderOpen=false;PendingSpawns=0;
        for(auto& E:Enemies)if(IsValid(E))E->Destroy();Enemies.Empty();Shots.Empty();Splashes.Empty();
        for(int I=0;I<3;++I){auto* E=GetWorld()->SpawnActor<ADungeonEnemy>();E->Species=I==2?24:0;E->bBoss=I==2;E->Health=E->MaxHealth=10000;E->SpawnTime=0;E->SetActorLocation(DungeonView::Unproject({580.+I*70,380}));E->SetActorTickEnabled(false);Enemies.Add(E);}
        H->SetActorLocation(DungeonView::Unproject({640,600}));
    }
    if(!Auto)return;
    Time+=Dt;
    if(!HeldPreview&&Time>=1){HeldPreview=true;H->SetFlashReviewAim({0,-1});H->PowerMove();}
    if(Time>=Next){const FVector2D Targets[]={{350,500},{640,380},{900,500},{640,650}};LaunchTea(H,Targets[CastCount++%4]);Next+=2;}
    const int Capture=int(Time*10);
    FString CaptureDir=TEXT("Saved/TeaV4Review");FParse::Value(FCommandLine::Get(),TEXT("TeaCapture="),CaptureDir);
    if(Capture!=Frame){Frame=Capture;FScreenshotRequest::RequestScreenshot(FPaths::ProjectDir()/CaptureDir/FString::Printf(TEXT("frame-%05d.png"),Frame),true,false);}
    if(Time>12)FPlatformMisc::RequestExit(false);
#endif
}
