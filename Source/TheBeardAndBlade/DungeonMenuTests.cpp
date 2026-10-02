#include "DungeonActors.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/PlatformMisc.h"

void ADungeonGameMode::VerifyMenu()
{
#if WITH_EDITOR
    int Checks=0,Errors=0;
    auto Check=[&](bool OK,const TCHAR* Why){++Checks;if(!OK){++Errors;UE_LOG(LogTemp,Error,TEXT("MENU: %s"),Why);}};
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));
    if(!H){FPlatformMisc::RequestExitWithStatus(false,1);return;}
    Check(IsMenu()&&!HasRun(),TEXT("initial title"));
    ToggleMenu();Check(IsMenu()&&!HasRun(),TEXT("escape does not start new game"));
    bShowSettings=true;H->Confirm();Check(IsMenu()&&!HasRun()&&bShowSettings,TEXT("settings enter cannot launch"));
    ToggleMenu();Check(IsMenu()&&!bShowSettings,TEXT("settings back to title"));
    bShowControls=true;H->Confirm();Check(IsMenu()&&!HasRun(),TEXT("help enter cannot launch"));
    ToggleMenu();H->Confirm();Check(HasRun()&&!IsMenu(),TEXT("new game"));
    AtlasArrivalTime=AtlasTravelTime=0;H->Coins=321;H->PotionCharges=3;H->Health=80;
    ToggleMenu();Check(IsMenu()&&HasRun(),TEXT("pause menu"));
    bShowSettings=true;H->Tick(2);Check(H->Health==80&&H->PotionCharges==3,TEXT("settings pauses player"));
    ToggleMenu();Check(IsMenu()&&!bShowSettings,TEXT("settings back does not resume"));
    bConfirmQuit=true;H->Confirm();Check(IsMenu()&&bConfirmQuit,TEXT("enter cannot accidentally quit"));
    ToggleMenu();Check(IsMenu()&&!bConfirmQuit,TEXT("cancel quit"));
    H->Confirm();Check(!IsMenu()&&H->Coins==321&&H->PotionCharges==3&&H->Health==80,TEXT("resume preserves run"));
    UE_LOG(LogTemp,Display,TEXT("MENU: %d checks, %d errors"),Checks,Errors);
    FPlatformMisc::RequestExitWithStatus(false,Errors?1:0);
#endif
}
