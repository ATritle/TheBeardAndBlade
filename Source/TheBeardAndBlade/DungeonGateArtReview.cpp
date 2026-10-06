#include "DungeonActors.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "HAL/PlatformMisc.h"

void ADungeonGameMode::ReviewGateArt()
{
#if !UE_BUILD_SHIPPING
    static int Chapter=-1,Stage=0;static float Since=0;
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));if(!H)return;
    const float Now=GetWorld()->GetTimeSeconds();
    if(Chapter<0||Now-Since>2.5f){
        ++Chapter;if(Chapter>=8){FPlatformMisc::RequestExit(false);return;}
        StartGame();InitializeAtlasFloor(12,Chapter);EnterAtlasRoom(1,-1);
        for(auto& E:Enemies)if(IsValid(E))E->Destroy();Enemies.Empty();PendingSpawns=0;Breakables.Empty();
        bMenu=bTraderOpen=bAtlasMap=bChest=false;AtlasArrivalTime=AtlasTravelTime=0;BossGrace=0;CancelBossIntro();DialogueLines.Empty();
        AtlasRooms[AtlasCurrent].Cleared=false;
        H->SetActorLocation(DungeonView::Unproject({640,470}));Since=Now;Stage=0;
    }
    if(Stage==0&&Now-Since>.7f){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("GateArtReview/Locked_%d.png"),Chapter),false,false);Stage=1;}
    if(Stage==1&&Now-Since>1.3f){AtlasRooms[AtlasCurrent].Cleared=true;Stage=2;}
    if(Stage==2&&Now-Since>1.8f){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("GateArtReview/Open_%d.png"),Chapter),false,false);Stage=3;}
#endif
}
