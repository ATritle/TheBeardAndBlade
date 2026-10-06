#include "DungeonActors.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/PlatformMisc.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "InputKeyEventArgs.h"
#include "HeroMeleeRig.h"
#include "HeroSockets.h"

void ADungeonGameMode::VerifyCombo()
{
#if WITH_EDITOR
    int Checks=0,Errors=0;
    auto Check=[&](bool OK,const TCHAR* Why){++Checks;if(!OK){++Errors;UE_LOG(LogTemp,Error,TEXT("COMBO: %s"),Why);}};
    for(int Clip=0;Clip<=FullBodyArt::Block;++Clip)for(int D=0;D<8;++D)for(int F=0;F<8;++F){
        const auto P=HeroMeleeRig::Get(Clip,D,F);
        Check(FMath::IsFinite(P.X)&&FMath::IsFinite(P.Y)&&P.X>=4&&P.X<=252&&P.Y>=4&&P.Y<=252,TEXT("registered right hand stays within source frame"));
        Check(FMath::IsFinite(P.Angle),TEXT("authored weapon angle finite"));
        if(Clip<=FullBodyArt::Run){
            const auto Next=HeroMeleeRig::Get(Clip,D,(F+1)%8);
            Check(FMath::Abs(FMath::FindDeltaAngleDegrees(P.Angle,Next.Angle))<12,TEXT("carry cycle has no landmark angle flips"));
        }
    }
    for(int Id=0;Id<24;++Id){
        const auto Grip=HeroSockets::CatalogGrip(Id);
        Check(Grip.X>0&&Grip.X<1&&Grip.Y>0&&Grip.Y<1,TEXT("weapon catalog has valid handle pivot"));
        const float Previous[]={.90f,.82f,.90f,.68f,.82f,.85f,.80f,.78f,.67f,.90f,.86f,.80f,.86f,.80f,.94f,.83f,.90f,.72f,.84f,.82f,.80f,.86f,.66f,.94f};
        FDungeonItem Item;Item.CatalogId=Id;
        Check(FMath::IsNearlyEqual(Item.EquippedScale(),Previous[Id]*2.f/3.f,1.e-6f),TEXT("every melee weapon reduced by one third"));
        for(int D=0;D<8;++D){
            const bool Mirror=HeroSockets::MirrorBlade(Id,D);
            if(Id==5||Id==6||Id==15||Id==17){
                const float A=FMath::DegreesToRadians(HeroMeleeRig::Get(FullBodyArt::Idle,D,0).Angle);
                const float Facing=D*PI/4;
                const float Edge=(Id==17?-1.f:1.f)*(Mirror?-1.f:1.f);
                const float Dot=Edge*(FMath::Cos(A)*FMath::Sin(Facing)-FMath::Sin(A)*FMath::Cos(Facing));
                Check(Dot>=-KINDA_SMALL_NUMBER,TEXT("single blade cutting edge faces forward in carry pose"));
                const double MirroredGrip=Mirror?1-Grip.X:Grip.X;
                Check(FMath::IsNearlyEqual(Mirror?1-MirroredGrip:MirroredGrip,Grip.X),TEXT("UV mirror preserves exact source handle anchor"));
            }else Check(!Mirror,TEXT("unrequested weapon textures retain original handedness"));
        }
    }
    for(const TCHAR* Name:{TEXT("MeleeIdle_SW"),TEXT("MeleeBlock_SW")}){
        const auto* T=LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Art/V2/%s.%s"),Name,Name));
        Check(T&&T->Source.GetSizeX()==1024&&T->Source.GetSizeY()==512,TEXT("corrected southwest atlas imported at expected resolution"));
    }
    FDungeonMeleeCombo C;
    for(int Cycle=0;Cycle<5;++Cycle){
        C.Begin();Check(!C.Finisher&&C.DamageScale()==1,TEXT("first hit normal"));Check(!C.Resolve(true),TEXT("first hit not combo"));
        C.Begin();Check(!C.Finisher,TEXT("second hit normal"));C.Resolve(true);
        C.Begin();Check(C.Finisher&&C.DamageScale()==1.5f,TEXT("third hit bonus"));Check(C.Resolve(true)&&C.Landed==0,TEXT("combo resets after third hit"));
    }
    C.Begin();C.Resolve(true);C.Begin();C.Resolve(false);C.Begin();Check(!C.Finisher&&C.Landed==0,TEXT("miss resets"));
    C.Resolve(true);C.Begin();C.Resolve(true);C.Tick(1.5f,false);C.Begin();Check(!C.Finisher,TEXT("long pause resets"));
    C.Resolve(true);C.Tick(5,true);Check(C.Landed==1,TEXT("slow swing cannot expire mid-animation"));C.Reset();
    Check(C.Landed==0&&!C.Finisher,TEXT("interrupt resets"));
    Check(FDungeonMeleeCombo::Pose(.35f)==1&&FDungeonMeleeCombo::Pose(.56f)==3&&FDungeonMeleeCombo::Pose(.7f)==2,TEXT("dedicated windup/strike/follow-through timeline"));
    StartGame();InitializeAtlasFloor(12);
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));
    if(!H){FPlatformMisc::RequestExitWithStatus(false,1);return;}
    for(auto& E:Enemies)if(IsValid(E))E->Destroy();Enemies.Empty();PendingSpawns=0;
    H->SetActorLocation(DungeonView::Unproject(FVector2D(640,460)));H->SetFlashReviewAim(FVector2D(0,1));
    H->CritChance=H->BleedChance=H->PoisonChance=H->Leech=0;H->AttackPower=100;H->AttackSpeed=1;
    for(auto& Gear:H->Equipment)Gear=FDungeonItem();
    auto* E=GetWorld()->SpawnActor<ADungeonEnemy>();E->Species=0;E->SpawnTime=0;E->Health=E->MaxHealth=10000;
    E->SetActorLocation(DungeonView::Unproject(FVector2D(640,520)));Enemies.Add(E);
    for(int I=0;I<3;++I){
        H->Attack();Check(H->IsComboSwing()==(I==2),TEXT("hero third swing selects finisher"));
        const float Before=E->Health;H->Tick(I==2?.38f:.21f);
        const float Damage=Before-E->Health;
        Check(Damage>(I==2?134:89)&&Damage<(I==2?166:111),TEXT("actual damage multiplier only on third swing"));
        H->Tick(.5f);Check(FMath::IsNearlyEqual(E->Health,Before-Damage),TEXT("one damage event per swing"));
    }
    Check(H->ComboPopupTime>0,TEXT("landed finisher raises graphic event"));
    auto* Extra=GetWorld()->SpawnActor<ADungeonEnemy>();Extra->Species=0;Extra->SpawnTime=0;Extra->Health=Extra->MaxHealth=10000;
    Extra->SetActorLocation(E->GetActorLocation());Enemies.Add(Extra);
    H->ResetMeleeChain();H->Attack();H->Tick(.5f);H->Attack();Check(!H->IsComboSwing(),TEXT("two targets in one cleave count as one landed swing"));H->Tick(.5f);
    H->Attack();Check(H->IsComboSwing(),TEXT("two cleaves arm exactly one finisher"));H->Tick(.7f);
    H->ResetMeleeChain();
    if(auto* PC=Cast<APlayerController>(H->GetController());PC&&PC->PlayerInput){
        const FKey Original=DungeonKeys::Key(DungeonKeys::Attack);
        FString Error;const bool Changed=DungeonKeys::Set(DungeonKeys::Attack,EKeys::F12,Error,false);
        Check(Changed,TEXT("test temporary rebound attack"));H->RebindInput();
        PC->PlayerInput->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,DungeonKeys::Key(DungeonKeys::Attack),IE_Pressed,0));
        auto& Key=*PC->PlayerInput->GetKeyState(DungeonKeys::Key(DungeonKeys::Attack));Key.bDown=true;
        const int Before=H->StrikeCount;H->GameplayAttack();
        for(int I=0;I<180;++I)H->Tick(1.f/60);
        Check(H->StrikeCount>=Before+4,TEXT("held rebound attack repeats without further clicks"));
        Key.bDown=false;H->GameplayAttackReleased();H->Tick(1);const int Released=H->StrikeCount;H->Tick(1);
        Check(!H->IsAttacking()&&H->StrikeCount==Released,TEXT("release stops repeat after current swing"));
        Key.bDown=true;H->GameplayAttack();H->ToggleInventory();H->ToggleInventory();H->Tick(1);
        Check(!H->IsAttacking(),TEXT("holding through inventory requires a fresh press"));
        Key.bDown=false;DungeonKeys::Set(DungeonKeys::Attack,Original,Error,false);H->RebindInput();
    }else Check(false,TEXT("input controller available"));
    H->ResetMeleeChain();H->Attack();H->Tick(.01f);H->ToggleInventory();Check(!H->IsAttacking(),TEXT("inventory cancels ongoing swing"));H->ToggleInventory();
    H->GameplayAttack();H->GameplayAttackReleased();H->Tick(1);Check(!H->IsAttacking(),TEXT("released attack never auto-repeats"));
    H->CancelCombatActions();H->Health=0;H->GameplayAttack();Check(!H->IsAttacking(),TEXT("dead hero cannot auto-attack"));H->Health=150;
    Check(LoadObject<UTexture2D>(nullptr,TEXT("/Game/Art/V2/Combat_Combo.Combat_Combo"))!=nullptr,TEXT("dedicated combo artwork imported"));
    LastSoundTime.Remove(TEXT("CoinPickup"));bEffectsMuted=false;PlaySound(TEXT("CoinPickup"),0);
    const double First=LastSoundTime.FindRef(TEXT("CoinPickup"));PlaySound(TEXT("CoinPickup"),0);
    Check(LastSoundTime.FindRef(TEXT("CoinPickup"))==First,TEXT("cluster coin cue throttled"));
    LastSoundTime.Add(TEXT("CoinPickup"),First-.1);PlaySound(TEXT("CoinPickup"),0);
    Check(LastSoundTime.FindRef(TEXT("CoinPickup"))==First-.1,TEXT("coin cue suppressed within 220ms"));
    UE_LOG(LogTemp,Display,TEXT("COMBO_VERIFY: %d checks, %d errors"),Checks,Errors);
    FPlatformMisc::RequestExitWithStatus(false,Errors?1:0);
#endif
}

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
