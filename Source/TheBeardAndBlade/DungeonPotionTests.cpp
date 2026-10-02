#include "DungeonActors.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/PlatformMisc.h"
#include "Engine/Texture2D.h"

void ADungeonGameMode::VerifyPotions()
{
#if WITH_EDITOR
    int Errors=0,Checks=0;
    auto Check=[&](bool OK,const TCHAR* Why){++Checks;if(!OK){++Errors;UE_LOG(LogTemp,Error,TEXT("POTIONS: %s"),Why);}};
    StartGame();
    auto* Sheet=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Art/V2/Potion_DrinkSheet.Potion_DrinkSheet"));
    Check(Sheet&&Sheet->Source.GetSizeX()==1536&&Sheet->Source.GetSizeY()==1024,TEXT("dedicated bottle sheet and crop dimensions"));
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));
    if(!H){FPlatformMisc::RequestExitWithStatus(false,1);return;}
    H->Restart();Potions.Empty();
    FDungeonPotion P;P.Position=DungeonView::Project(H->GetActorLocation());
    for(int I=0;I<5;++I)Potions.Add(P);
    UpdatePotions(1);
    Check(H->PotionCharges==4&&Potions.Num()==1&&H->Health==H->MaxHealth,TEXT("four carried, excess remains, full health pickups"));
    H->DrinkPotion();Check(H->PotionCharges==4&&!H->IsDrinking(),TEXT("full health does not waste charge"));
    H->Health=30;H->DrinkPotion();
    Check(H->Health==67.5f&&H->PotionCharges==3&&H->IsDrinkingPotion(),TEXT("quarter max heal and quick sip"));
    H->DrinkPotion();H->DrinkTea();H->Dodge();H->BlockPressed();
    Check(H->PotionCharges==3&&!H->IsTeaEmpowered()&&!H->IsRolling()&&!H->IsBlocking()&&!H->CanStrike(),TEXT("repeat and conflicting actions locked"));
    const auto Position=H->GetActorLocation();H->MoveRight(1);H->Tick(.2f);H->MoveRight(0);
    Check(H->GetActorLocation().Equals(Position),TEXT("feet planted while drinking"));
    H->Tick(.46f);Check(!H->IsDrinkingPotion()&&H->CanStrike(),TEXT("sip completes"));
    UpdatePotions(.1f);Check(H->PotionCharges==4&&Potions.IsEmpty(),TEXT("freed charge picks up remaining potion"));
    H->Health=149;H->DrinkPotion();Check(H->Health==150,TEXT("heal clamps"));
    H->CancelCombatActions();H->Health=50;H->PotionCharges=0;H->DrinkPotion();
    Check(H->Health==50&&!H->IsDrinking(),TEXT("empty meter cannot heal"));
    H->PotionCharges=2;H->ToggleInventory();H->DrinkPotion();
    Check(H->PotionCharges==2&&H->Health==50,TEXT("inventory blocks use"));H->ToggleInventory();
    H->Health=0;H->DrinkPotion();Check(H->PotionCharges==2,TEXT("dead player cannot use"));
    H->RestoreAfterDeath();Check(H->PotionCharges==2&&!H->IsDrinkingPotion(),TEXT("revival retains carried charges"));
    H->MaxHealth=400;H->Health=100;H->DrinkPotion();Check(H->Health==200,TEXT("gear-scaled max health fraction"));
    H->Restart();Check(H->PotionCharges==0&&!H->IsDrinking(),TEXT("new run resets potions"));
    UE_LOG(LogTemp,Display,TEXT("POTIONS: %d checks, %d errors"),Checks,Errors);
    FPlatformMisc::RequestExitWithStatus(false,Errors?1:0);
#endif
}
