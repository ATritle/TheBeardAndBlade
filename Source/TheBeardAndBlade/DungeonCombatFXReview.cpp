#include "DungeonActors.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "HAL/PlatformMisc.h"

void ADungeonGameMode::ReviewCombatFX()
{
#if !UE_BUILD_SHIPPING
    static int Frame=-1,LastBeat=-1;
    static float Start=-1;
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));if(!H)return;
    if(Start<0){
        StartGame();InitializeAtlasFloor(12);bMenu=false;PendingSpawns=0;CancelBossIntro();DialogueLines.Empty();BossGrace=0;
        AtlasArrivalTime=AtlasTravelTime=0;bAtlasMap=bTraderOpen=false;Breakables.Empty();
        for(auto& E:Enemies)if(IsValid(E))E->Destroy();Enemies.Empty();
        auto* E=GetWorld()->SpawnActor<ADungeonEnemy>();E->Species=0;E->Health=E->MaxHealth=100000;E->SpawnTime=0;E->Recovery=100;Enemies.Add(E);
        H->Equip(RollItem(0,0,1));H->AttackSpeed=1;H->AttackPower=40;H->Health=H->MaxHealth;H->StunTime=0;
        H->SetActorLocation(DungeonView::Unproject({600,430}));
        GEngine->bUseFixedFrameRate=true;GEngine->FixedFrameRate=30;Start=GetWorld()->GetTimeSeconds();
    }
    const float T=GetWorld()->GetTimeSeconds()-Start;
    H->QuipTime=0;H->QuipCooldown=100;H->SetFlashReviewAim({1,0});
    if(auto* PC=GetWorld()->GetFirstPlayerController())PC->SetMouseLocation(730,360);
    for(auto& E:Enemies)if(IsValid(E)){E->Recovery=100;E->SetActorLocation(DungeonView::Unproject({675,430}));}
    H->UIClickFrame=MAX_uint64;
    if(T<6){if(!H->IsAttacking())H->Attack();}
    else if(T<12){
        if(!H->IsBlocking()&&H->CanStartBlock())H->BlockPressed();
        const int Beat=int((T-6)/.8f);
        if(Beat!=LastBeat&&H->IsBlocking()){LastBeat=Beat;H->ReceiveMeleeHit(15,{675,430});}
        if(T>10)H->BlockReleased();
    }else{
        H->BlockReleased();H->MoveRight(T<15?.6f:-.6f);
        if(T>15&&T<15.05f)H->RestoreHealth(20);
    }
    ++Frame;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/CombatNiagara/Frame%04d.png"),Frame),false,false);
    if(T>=18)FPlatformMisc::RequestExit(false);
#endif
}
