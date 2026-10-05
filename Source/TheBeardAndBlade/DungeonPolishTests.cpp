#include "DungeonActors.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/PlatformMisc.h"

void ADungeonGameMode::VerifyPolish()
{
#if WITH_EDITOR
    int Checks=0,Errors=0;
    auto Check=[&](bool OK,const TCHAR* Why){++Checks;if(!OK){++Errors;UE_LOG(LogTemp,Error,TEXT("POLISH: %s"),Why);}};
    StartGame();
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));
    if(!H){FPlatformMisc::RequestExitWithStatus(false,1);return;}
    Impacts.Empty();Blood.Empty();H->Health=100;H->MaxHealth=150;
    H->RestoreHealth(20);
    Check(H->Health==120&&Impacts.Num()==1&&Impacts[0].Damage==-20,TEXT("actual healing creates positive-number event"));
    Check(Blood.IsEmpty(),TEXT("healing never creates blood"));
    H->RestoreHealth(50);
    Check(H->Health==150&&Impacts.Num()==1&&Impacts[0].Damage==-50,TEXT("rapid heals aggregate actual capped amount"));
    H->RestoreHealth(50);
    Check(Impacts.Num()==1&&Impacts[0].Damage==-50,TEXT("full health creates no phantom healing"));
    Check(FMath::IsNearlyEqual(Impacts[0].Life,1.05f),TEXT("readable floating-number lifetime"));
    Impacts.Empty();AddImpact(FVector2D(640,400),24);
    Check(Impacts.Num()==1&&Impacts[0].Damage==24&&Blood.Num()==1,TEXT("damage preserves impact and blood"));
    AddImpact(FVector2D(640,400),0);
    Check(Impacts.Num()==2&&Blood.Num()==1,TEXT("zero-damage explosion has no blood"));
    for(int I=0;I<200;++I)AddImpact(FVector2D(I,400),0);
    Check(Impacts.Num()==128,TEXT("visual queue bounded under heavy combat"));
    Check(H->Health==150,TEXT("visual events never apply gameplay damage"));
    UE_LOG(LogTemp,Display,TEXT("POLISH: %d checks, %d errors"),Checks,Errors);
    FPlatformMisc::RequestExitWithStatus(false,Errors?1:0);
#endif
}
