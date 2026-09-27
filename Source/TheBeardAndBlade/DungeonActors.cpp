#include "DungeonActors.h"
#include "DungeonInventoryLayout.h"
#include "InventoryGlyphMetrics.h"
#include "DungeonCombatBalance.h"
#include "HeroSockets.h"
#include "AthleticHeroSockets.h"
#include "HeroBreathing.h"
#include "DungeonDescent.h"
#include "DungeonRoster.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "CanvasItem.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformMisc.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/ScopeExit.h"
#if WITH_EDITOR
#include "TextureCompiler.h"
#endif

namespace
{
    const FLinearColor Gold(.94f,.69f,.30f), Pale(.85f,.9f,.86f), Ink(.012f,.021f,.025f,.94f);
    const TCHAR* RarityNames[]={TEXT("COMMON"),TEXT("UNCOMMON"),TEXT("RARE"),TEXT("EPIC"),TEXT("LEGENDARY")};
    FLinearColor RarityColor(int32 R)
    {
        const FLinearColor Colors[]={FLinearColor(.65f,.69f,.72f),FLinearColor(.3f,.88f,.53f),FLinearColor(.3f,.62f,1.f),FLinearColor(.8f,.38f,1.f),FLinearColor(1.f,.6f,.16f)};
        return Colors[FMath::Clamp(R,0,4)];
    }
    ADungeonHero* Player(UObject* Context) { return Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(Context,0)); }
    ADungeonGameMode* Mode(const UObject* Context) { return Cast<ADungeonGameMode>(UGameplayStatics::GetGameMode(Context)); }
    FString ItemArt(int32 Icon) { return FString::Printf(TEXT("Item_%d_%d"),Icon/3,Icon%3); }
}
FString FDungeonItem::Stats() const
{
    FString S;
    if(Attack>0) S+=FString::Printf(TEXT("+%d ATTACK  "),(int32)Attack);
    if(Defense>0) S+=FString::Printf(TEXT("+%d ARMOR  "),(int32)Defense);
    if(Vitality>0) S+=FString::Printf(TEXT("+%d HEALTH"),(int32)Vitality);
    return S;
}
ADungeonHero::ADungeonHero()
{
    PrimaryActorTick.bCanEverTick=true;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("HeroRoot"));
    Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("FixedCamera"));
    Camera->SetupAttachment(RootComponent);
    Camera->SetAbsolute(true,true,true);
    Camera->SetRelativeLocation(FVector(0,0,1200));
    Camera->SetRelativeRotation(FRotator(-90,0,0));
    AutoPossessPlayer=EAutoReceiveInput::Player0;
}
void ADungeonHero::BeginPlay()
{
    Super::BeginPlay();
    Restart();
    if(auto* PC=Cast<APlayerController>(GetController()))
    {
        PC->bShowMouseCursor=true;
        PC->DefaultMouseCursor=EMouseCursor::None;
        PC->CurrentMouseCursor=EMouseCursor::None;
        FInputModeGameAndUI Input;
        Input.SetHideCursorDuringCapture(false);
        Input.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(Input);
    }
}
void ADungeonHero::Restart()
{
    CancelCombatActions();
    Equipment.SetNum(4);
    Equipment[0]=ADungeonGameMode::MakeItem(0,0); Equipment[0].Attack=0; Equipment[0].Name=TEXT("Old Iron Sword");
    Equipment[1]=ADungeonGameMode::MakeItem(3,0); Equipment[1].Defense=0; Equipment[1].Vitality=0; Equipment[1].Name=TEXT("Traveler's Leathers");
    Equipment[2]=FDungeonItem(); Equipment[2].Slot=2; Equipment[2].Icon=6;
    Equipment[3]=FDungeonItem(); Equipment[3].Slot=3;
    Inventory.Empty(); Coins=0;SelectedItem=INDEX_NONE; bInventoryOpen=false; InventoryMessage.Empty();
    Health=MaxHealth=150; AttackPower=24; Armor=8; HurtTime=Invulnerable=AttackTime=0;
    CritChance=.05f;CritMultiplier=1.5f;AttackSpeed=StaminaRegen=1;StrikeCount=0;
    Stamina=MaxStamina=100; StaminaDelay=0; bExhausted=false;
    BleedChance=PoisonChance=Leech=DamageReduction=0;
    InputX=InputY=WalkDistance=FootstepDistance=0; IdleBreathBlend=BreathPhase=0;bSprinting=bWalking=false; Aim=FVector2D(0,1); Facing=4;
    RollTime=RollCooldown=PowerCooldown=PowerCastTime=0; bTeaReleased=false;
    StunTime=SlowTime=StatusClock=FlashBlindTime=0;
    SetActorLocation(DungeonView::Unproject(FVector2D(640,520)));
}
void ADungeonHero::Tick(float Dt)
{
    Super::Tick(Dt);
    ScreenVelocity=FVector2D::ZeroVector;
    if(auto* G=Mode(this)) if(G->IsGameplayBlocked()) return;
    if(bInventoryOpen) { bWalking=false; return; }
    const bool Resting=Health>0&&StunTime<=0&&!IsAttacking()&&!IsCasting()&&!IsRolling()&&FMath::Abs(InputX)+FMath::Abs(InputY)<.01f;
    IdleBreathBlend=FMath::FInterpTo(IdleBreathBlend,Resting?1.f:0.f,Dt,Resting?4.f:12.f);
    const float Fatigue=1.f-FMath::Clamp(Stamina/FMath::Max(1.f,MaxStamina),0.f,1.f);
    BreathPhase=FMath::Fmod(BreathPhase+Dt*(4.6f+Fatigue*2.2f),2.f*PI);
    StatusClock+=Dt;
    FlashBlindTime=FMath::Max(0.f,FlashBlindTime-Dt);
    SlowTime=FMath::Max(0.f,SlowTime-Dt);
    if(StunTime>0) { StunTime=FMath::Max(0.f,StunTime-Dt); bWalking=false; UpdateStamina(Dt,false); HurtTime=FMath::Max(0.f,HurtTime-Dt); Invulnerable=FMath::Max(0.f,Invulnerable-Dt); return; }
    RollCooldown=FMath::Max(0.f,RollCooldown-Dt);
    PowerCooldown=FMath::Max(0.f,PowerCooldown-Dt);
    if(PowerCastTime>0)
    {
        PowerCastTime=FMath::Max(0.f,PowerCastTime-Dt);
        if(!bTeaReleased&&PowerCastTime<=.26f)
        {
            bTeaReleased=true; if(auto* G=Mode(this)) G->LaunchTea(this,PowerTarget);
        }
    }
    HurtTime=FMath::Max(0.f,HurtTime-Dt); Invulnerable=FMath::Max(0.f,Invulnerable-Dt);
    if(Health<=0) { bWalking=false; AttackTime=PowerCastTime=0; return; }
    if(RollTime>0)
    {
        UpdateStamina(Dt,false);
        const float Step=FMath::Min(Dt,RollTime); RollTime=FMath::Max(0.f,RollTime-Dt);
        SetActorLocation(DungeonView::Unproject(DungeonView::Clamp(DungeonView::Project(GetActorLocation())+RollAim*510.f*Step)));
        bWalking=false; return;
    }
    FVector2D P=DungeonView::Project(GetActorLocation());
    FVector2D Move(InputX,InputY);
    Move=Move.GetClampedToMaxSize(1);
    const bool Sprinting=bSprinting&&!bExhausted&&Stamina>0&&!Move.IsNearlyZero()&&
        !DungeonView::Clamp(P+Move).Equals(P,.001f);
    // Split the final sprint frame so speed never exceeds the stamina available.
    const float SprintSeconds=Sprinting?FMath::Min(Dt,Stamina/DungeonCombatBalance::SprintCost):0;
    const FVector2D Next=DungeonView::Clamp(P+Move*(190.f*Dt+190.f*SprintSeconds)*(IsAttacking()?.5f:1.f)*(SlowTime>0?.6f:1.f));
    UpdateStamina(Dt,Sprinting);
    bWalking=FVector2D::Distance(Next,P)>.01f;
    // Cadence is independent of 2x sprint translation: walk ~8 fps, sprint ~10 fps.
    const float GaitStep=FVector2D::Distance(Next,P)/(Sprinting?1.54f:1.f);
    WalkDistance+=GaitStep;
    FootstepDistance+=GaitStep;
    if(bWalking&&FootstepDistance>=72) { FootstepDistance=FMath::Fmod(FootstepDistance,72.f); if(auto* G=Mode(this)) G->PlaySound(TEXT("Step"),.35f,FMath::FRandRange(.9f,1.1f)); }
    SetActorLocation(DungeonView::Unproject(Next));
    if(Dt>SMALL_NUMBER) ScreenVelocity=(Next-P)/Dt;
    if(auto* PC=Cast<APlayerController>(GetController()))
    {
        int32 W=0,H=0; float MX=0,MY=0; PC->GetViewportSize(W,H);
        const float S=FMath::Min(W/1280.f,H/800.f);
        if(S>0&&PC->GetMousePosition(MX,MY))
        {
            FVector2D Mouse=(FVector2D(MX,MY)-FVector2D((W-1280*S)/2,(H-800*S)/2))/S;
            if(Mouse.X>=0&&Mouse.X<=1280&&Mouse.Y>=0&&Mouse.Y<=800)
            {
                const FVector2D Delta=Mouse-Next;
                if(Delta.SizeSquared()>144)
                {
                    Aim=Delta.GetSafeNormal();
                    Facing=DungeonView::Direction(Aim);
                }
            }
        }
    }
    if(AttackTime>0)
    {
        AttackTime=FMath::Max(0.f,AttackTime-Dt*AttackSpeed);
        // Strike once on the impact pose, not during the wind-up.
        if(!bAttackHit&&AttackTime<=.28f)
        {
            bAttackHit=true;
            if(auto* G=Mode(this)) G->PlayerAttack(this);
        }
    }
}
int32 ADungeonHero::GetAnimationFrame() const
{
    if(IsCasting()) return FMath::Clamp((int32)(GetCastProgress()*6),0,5);
    if(IsAttacking()) return FMath::Clamp((int32)(GetAttackProgress()*6),0,5);
    return bWalking?((int32)(WalkDistance/24.f)%6):0;
}
void ADungeonHero::SetupPlayerInputComponent(UInputComponent* I)
{
    I->BindAxis("MoveForward",this,&ADungeonHero::MoveForward);
    I->BindAxis("MoveRight",this,&ADungeonHero::MoveRight);
    I->BindAction("Attack",IE_Pressed,this,&ADungeonHero::Attack);
    I->BindAction("PowerMove",IE_Pressed,this,&ADungeonHero::PowerMove);
    I->BindAction("Freedom",IE_Pressed,this,&ADungeonHero::Freedom);
    I->BindAction("ToggleMusic",IE_Pressed,this,&ADungeonHero::ToggleMusic);
    I->BindAction("ToggleEffects",IE_Pressed,this,&ADungeonHero::ToggleEffects);
    I->BindAction("Interact",IE_Pressed,this,&ADungeonHero::Interact);
    I->BindAction("Inventory",IE_Pressed,this,&ADungeonHero::ToggleInventory);
    I->BindAction("Dodge",IE_Pressed,this,&ADungeonHero::Dodge);
    I->BindAction("Menu",IE_Pressed,this,&ADungeonHero::Menu);
    I->BindAction("Confirm",IE_Pressed,this,&ADungeonHero::Confirm);
    I->BindAction("Sprint",IE_Pressed,this,&ADungeonHero::SprintPressed);
    I->BindAction("Sprint",IE_Released,this,&ADungeonHero::SprintReleased);
#if WITH_EDITOR
    I->BindKey(EKeys::F1,IE_Pressed,this,&ADungeonHero::TestFinance);
    I->BindKey(EKeys::F2,IE_Pressed,this,&ADungeonHero::TestMack);
    I->BindKey(EKeys::F3,IE_Pressed,this,&ADungeonHero::TestWebroot);
    I->BindKey(EKeys::F4,IE_Pressed,this,&ADungeonHero::TestRime);
    I->BindKey(EKeys::F5,IE_Pressed,this,&ADungeonHero::TestCinder);
    I->BindKey(EKeys::F6,IE_Pressed,this,&ADungeonHero::TestTwister);
    I->BindKey(EKeys::F7,IE_Pressed,this,&ADungeonHero::TestIce);
    I->BindKey(EKeys::F9,IE_Pressed,this,&ADungeonHero::TestFinance);
    I->BindKey(EKeys::F10,IE_Pressed,this,&ADungeonHero::TestTwister);
    I->BindKey(EKeys::F11,IE_Pressed,this,&ADungeonHero::TestFlashBang);
    // Main keyboard number row: available on laptops without a numeric keypad.
    I->BindKey(EKeys::One,IE_Pressed,this,&ADungeonHero::TestGreaseEnemies);
    I->BindKey(EKeys::Two,IE_Pressed,this,&ADungeonHero::TestBunkerEnemies);
    I->BindKey(EKeys::Three,IE_Pressed,this,&ADungeonHero::TestStormEnemies);
    I->BindKey(EKeys::Four,IE_Pressed,this,&ADungeonHero::TestFinance);
    I->BindKey(EKeys::Five,IE_Pressed,this,&ADungeonHero::TestMack);
    I->BindKey(EKeys::Six,IE_Pressed,this,&ADungeonHero::TestFlashBang);
    I->BindKey(EKeys::Seven,IE_Pressed,this,&ADungeonHero::TestWebroot);
    I->BindKey(EKeys::Eight,IE_Pressed,this,&ADungeonHero::TestRime);
    I->BindKey(EKeys::Nine,IE_Pressed,this,&ADungeonHero::TestCinder);
    I->BindKey(EKeys::Zero,IE_Pressed,this,&ADungeonHero::TestTwister);
#endif
}
void ADungeonHero::Attack()
{
    if(auto* G=Mode(this))if(G->IsTraderOpen()) {
        if(auto* PC=Cast<APlayerController>(GetController()))if(auto* HUD=Cast<ADungeonHUD>(PC->GetHUD())) {
            if(bInventoryOpen)HUD->InventoryClick();else HUD->TraderClick();
        }
        return;
    }
    if(StunTime>0) return;
    if(auto* G=Mode(this))
    {
        if(G->HasEnding()) { if(auto* PC=Cast<APlayerController>(GetController())) if(auto* HUD=Cast<ADungeonHUD>(PC->GetHUD())) HUD->EndingClick(); return; }
        if(G->IsBossIntroActive()&&!G->IsMenu()) { SkipIntro();return; }
        if(G->IsMenu()) { if(auto* PC=Cast<APlayerController>(GetController())) if(auto* HUD=Cast<ADungeonHUD>(PC->GetHUD())) HUD->InventoryClick(); return; }
        if(G->IsBossDialogueActive()) { if(auto* PC=Cast<APlayerController>(GetController())) if(auto* HUD=Cast<ADungeonHUD>(PC->GetHUD())) HUD->DialogueClick(); return; }
        if(G->IsGameplayBlocked()) return;
    }
    if(bInventoryOpen)
    {
        if(auto* PC=Cast<APlayerController>(GetController()))
            if(auto* HUD=Cast<ADungeonHUD>(PC->GetHUD())) HUD->InventoryClick();
        return;
    }
    if(Health<=0||IsAttacking()||IsRolling()||IsCasting()) return;
    AttackTime=.48f; bAttackHit=false; AttackAim=Aim; AttackDirection=Facing;
    if(QuipCooldown<=0&&FMath::FRand()<.35f)
    {
        AttackQuip=TEXT("Take this you C*NT!"); QuipTime=1.5f; QuipCooldown=4.f;
    }
    if(auto* G=Mode(this)) G->PlaySound(TEXT("Sword"),.8f,FMath::FRandRange(.92f,1.08f));
}
void ADungeonHero::Interact() { if(!bInventoryOpen) if(auto* G=Mode(this)) G->PlayerInteract(this); }
void ADungeonHero::ToggleInventory()
{
    if(auto* G=Mode(this)) if(G->IsGameplayBlocked()&&!G->IsTraderOpen()) return;
    if(IsRolling()) return;
    if(Health<=0) return;
    bInventoryOpen=!bInventoryOpen; InventoryMessage.Empty();
    if(auto* PC=Cast<APlayerController>(GetController())) if(auto* HUD=Cast<ADungeonHUD>(PC->GetHUD())) HUD->CancelInventoryGesture();
    if(auto* G=Mode(this)) G->PlaySound(TEXT("UI"),.4f);
}
bool ADungeonHero::CanPlace(FIntPoint Cell,FIntPoint Size,int32 Ignore) const
{
    if(Cell.X<0||Cell.Y<0||Cell.X+Size.X>6||Cell.Y+Size.Y>6) return false;
    for(int32 I=0;I<Inventory.Num();++I) if(I!=Ignore)
    {
        const auto& E=Inventory[I]; const FIntPoint S=E.Item.Size();
        if(Cell.X<E.Cell.X+S.X&&Cell.X+Size.X>E.Cell.X&&Cell.Y<E.Cell.Y+S.Y&&Cell.Y+Size.Y>E.Cell.Y) return false;
    }
    return true;
}
bool ADungeonHero::AddToInventory(const FDungeonItem& Item)
{
    if(Item.IsEmpty()) return false;
    for(int32 Y=0;Y<6;++Y) for(int32 X=0;X<6;++X) if(CanPlace({X,Y},Item.Size()))
    {
        Inventory.Add({Item,{X,Y}}); InventoryMessage=TEXT("Item added to your bag."); return true;
    }
    InventoryMessage=TEXT("Bag full. Make space, then press E at the chest again."); return false;
}
bool ADungeonHero::EquipFromInventory(int32 Index)
{
    if(!Inventory.IsValidIndex(Index)) return false;
    const FDungeonItem Item=Inventory[Index].Item;
    if(!Equipment.IsValidIndex(Item.Slot)) return false;
    const FDungeonItem Old=Equipment[Item.Slot];
    // Same-slot items have identical footprints: even a full bag can swap safely.
    if(!Old.IsEmpty()) Inventory[Index].Item=Old;
    else { Inventory.RemoveAt(Index); SelectedItem=INDEX_NONE; }
    Equip(Item); InventoryMessage.Empty(); return true;
}
bool ADungeonHero::Unequip(int32 Slot)
{
    if(!Equipment.IsValidIndex(Slot)||Equipment[Slot].IsEmpty()) return false;
    if(!AddToInventory(Equipment[Slot])) return false;
    FDungeonItem Empty; Empty.Slot=Slot; Equip(Empty);
    InventoryMessage=TEXT("Equipment returned to your bag."); return true;
}
bool ADungeonHero::IsBulletImmune() const
{
    const auto* G=Mode(this);
    return (RollTime>.14f)||(G&&G->IsFreedomActive());
}
void ADungeonHero::ReceiveHit(float Damage,bool PerProjectile)
{
    if(auto* G=Mode(this)) if(G->IsFreedomActive()) return;
    if(auto* G=Mode(this)) if(G->IsGameplayBlocked()) return;
    if(Health<=0||(PerProjectile?IsBulletImmune():Invulnerable>0)) return;
    const float Actual=FMath::Max(2.f,Damage*(HasEffect(10)?.88f:1.f)-Armor*.3f)*(1-DamageReduction);
    Health=FMath::Max(0.f,Health-Actual);
    if(Health<=0) StunTime=SlowTime=FlashBlindTime=0;
    HurtTime=.18f; if(!PerProjectile) Invulnerable=.65f;
    if(auto* G=Mode(this)) G->PlaySound(Health<=0?TEXT("Death"):TEXT("Hurt"));
    if(auto* G=Mode(this)) G->AddImpact(DungeonView::Project(GetActorLocation()),Actual,true);
}
void ADungeonHero::Dodge()
{
    if(auto* Intro=Mode(this))if(Intro->IsBossIntroActive()){SkipIntro();return;}
    if(StunTime>0) return;
    auto* G=Mode(this);
    if(!G||G->IsGameplayBlocked()||bInventoryOpen||Health<=0||RollCooldown>0||IsCasting()||bExhausted||Stamina<DungeonCombatBalance::DodgeCost) return;
    SpendStamina(DungeonCombatBalance::DodgeCost);
    RollAim=FVector2D(InputX,InputY).GetSafeNormal(); if(RollAim.IsNearlyZero()) RollAim=Aim;
    RollDirection=(FMath::RoundToInt(FMath::Atan2(RollAim.Y,RollAim.X)/(PI/2))+4)%4;
    RollTime=.48f; RollCooldown=1.15f; Invulnerable=.34f; AttackTime=0; bAttackHit=true;
    G->PlaySound(TEXT("Roll"));
}
void ADungeonHero::Menu() { if(auto* G=Mode(this)) G->ToggleMenu(); }
void ADungeonHero::Confirm() { if(auto* G=Mode(this)) {if(G->HasEnding()){G->RestartFromEnding();return;}if(G->IsBossIntroActive()&&!G->IsMenu()){SkipIntro();return;}if(G->IsMenu()) { if(G->HasRun()) G->ToggleMenu(); else G->StartGame(); }} }
void ADungeonHero::TransitionWalk(FVector2D From,FVector2D To,float Progress,float GaitScale)
{
    SetActorLocation(DungeonView::Unproject(FMath::Lerp(From,To,Progress)));
    Facing=DungeonView::Direction(To-From); Aim=(To-From).GetSafeNormal(); bWalking=true;
    // Compressed screen-space stair travel still advances a slow, readable gait.
    WalkDistance=Progress*FVector2D::Distance(From,To)*GaitScale; AttackTime=RollTime=PowerCastTime=HurtTime=0; IdleBreathBlend=0;
}
void ADungeonHero::Equip(const FDungeonItem& Item)
{
    if(!Equipment.IsValidIndex(Item.Slot)) return;
    Equipment[Item.Slot]=Item;
    if(auto* G=Mode(this)) G->PlaySound(TEXT("Equip"),.65f);
    RebuildStats();
    Health=FMath::Clamp(Health,0.f,MaxHealth); // Swapping vitality gear must not repeatedly heal.
}
ADungeonEnemy::ADungeonEnemy()
{
    PrimaryActorTick.bCanEverTick=true;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("EnemyRoot"));
}
void ADungeonEnemy::TakeDungeonDamage(float Damage)
{
    if(auto* G=Mode(this)) if(G->IsGameplayBlocked()) return;
    if(Health<=0||SpawnTime>0) return;
    const float Dealt=FMath::Min(Health,FMath::Max(0.f,Damage));
    Health=FMath::Max(0.f,Health-Damage); HurtTime=.2f;
    if(auto* H=Player(this)) if(H->Health>0) H->Health=FMath::Min(H->MaxHealth,H->Health+Dealt*H->Leech);
    auto* G=Mode(this);
    const FVector2D P=DungeonView::Project(GetActorLocation());
    if(G) G->AddImpact(P,Damage);
    if(G) G->PlaySound(Health<=0?TEXT("Death"):TEXT("Hit"),.75f,FMath::FRandRange(.9f,1.1f));
    if(auto* H=Player(this))
    {
        auto Push=(P-DungeonView::Project(H->GetActorLocation())).GetSafeNormal();
        SetActorLocation(DungeonView::Unproject(DungeonView::Clamp(P+Push*(bBoss?5:18))));
    }
    if(Health<=0&&G) G->EnemyDefeated(this);
}
ADungeonGameMode::ADungeonGameMode()
{
    DefaultPawnClass=ADungeonHero::StaticClass(); HUDClass=ADungeonHUD::StaticClass();
    PrimaryActorTick.bCanEverTick=true;
}
void ADungeonGameMode::BeginPlay()
{
    Super::BeginPlay();
    GConfig->GetBool(TEXT("DungeonAudio"),TEXT("MuteMusic"),bMusicMuted,GGameUserSettingsIni);
    GConfig->GetFloat(TEXT("DungeonAudio"),TEXT("Volume"),MasterVolume,GGameUserSettingsIni);
    MasterVolume=FMath::Clamp(MasterVolume,0.f,1.f);
    GConfig->GetBool(TEXT("DungeonAudio"),TEXT("MuteEffects"),bEffectsMuted,GGameUserSettingsIni);
#if !UE_BUILD_SHIPPING
    if(FParse::Param(FCommandLine::Get(),TEXT("TraderVerify"))) { VerifyTrader(); return; }
    if(FParse::Param(FCommandLine::Get(),TEXT("BreakablesVerify"))) { VerifyBreakables(); return; }
    if(FParse::Param(FCommandLine::Get(),TEXT("SeptemberVerify"))) { VerifySeptember(); return; }
    if(FParse::Param(FCommandLine::Get(),TEXT("FlashVerify"))) { VerifyFlashBang(); return; }
    if(FParse::Param(FCommandLine::Get(),TEXT("ProgressionVerify"))) { VerifyProgression(); return; }
    if(FParse::Param(FCommandLine::Get(),TEXT("IntroVerify"))) { VerifyBossIntro(); return; }
    if(FParse::Param(FCommandLine::Get(),TEXT("WeekendVerify"))) { VerifyWeekend(); return; }
    if(FParse::Param(FCommandLine::Get(),TEXT("EndingVerify"))) { VerifyEndings(); return; }
    if(FParse::Param(FCommandLine::Get(),TEXT("DungeonVerify"))) VerifyCampaign();
    if(FParse::Param(FCommandLine::Get(),TEXT("DungeonCapture"))&&!FParse::Param(FCommandLine::Get(),TEXT("DungeonMenuPreview"))) { bMenu=false; bHasRun=true; SpawnWave(); }
    int32 PreviewBiome=0;
    FParse::Value(FCommandLine::Get(),TEXT("DungeonBiome="),PreviewBiome);
    if(FParse::Param(FCommandLine::Get(),TEXT("DungeonCapture"))&&!bMenu) { Room=1+DungeonProgression::RoomsPerChapter*FMath::Clamp(PreviewBiome,0,6); SpawnWave(); }
    if(FParse::Param(FCommandLine::Get(),TEXT("DungeonBossPreview"))) { Room=DungeonProgression::RoomsPerChapter*(1+FMath::Clamp(PreviewBiome,0,6)); SpawnWave(); }
    if(FParse::Param(FCommandLine::Get(),TEXT("DungeonFlashPreview"))) { Room=DungeonProgression::BossRoom(30);SpawnWave(); }
#endif
}
void ADungeonGameMode::Tick(float Dt)
{
    Super::Tick(Dt);
    RunEndingPreview();
    if(!HasEnding()&&IsDead()) FinishRun(false);
    if(HasEnding()) { UpdateEnding(Dt);UpdateAudio();return; }
    UpdateAudio();
    RunPackagedSmokeTest();
    RunSeptemberSmoke();
#if !UE_BUILD_SHIPPING
    // Explicit review-only launch flags. Never run during normal editor play.
    if(FParse::Param(FCommandLine::Get(),TEXT("DungeonCapture")))
    {
        static bool Prepared=false, Captured=false;
        const float Time=GetWorld()->GetTimeSeconds();
        if(!Prepared&&Time>2.f)
        {
            Prepared=true;
            if(FParse::Param(FCommandLine::Get(),TEXT("DungeonTraderPreview"))) {
                for(auto& E:Enemies)if(IsValid(E))E->Destroy();Enemies.Empty();PendingSpawns=0;Room=2;bLootRolled=true;
                if(auto* H=Player(this)){H->Coins=1200;H->AddToInventory(RollItem(24,2,3));}
                OpenTrader();TraderStock.Empty();for(int I=0;I<5;++I)TraderStock.Add(RollItem(I==4?51:I*12,I%5,3));
                if(auto* PC=GetWorld()->GetFirstPlayerController())if(auto* HUD=Cast<ADungeonHUD>(PC->GetHUD()))HUD->TraderSelection=4;
            }
            if(FParse::Param(FCommandLine::Get(),TEXT("DungeonBreakablesPreview"))) {
                for(auto& E:Enemies)if(IsValid(E))E->Destroy();Enemies.Empty();PendingSpawns=0;
                Breakables.Empty();
                for(int I=0;I<6;++I){FDungeonBreakable B;B.Position=FVector2D(I<3?155:1125,310+(I%3)*125);B.Variant=I%3;Breakables.Add(B);}
            }
            if(FParse::Param(FCommandLine::Get(),TEXT("DungeonControlsPreview"))) bShowControls=true;
            if(FParse::Param(FCommandLine::Get(),TEXT("DungeonMenuHoverPreview")))
                if(auto* PC=GetWorld()->GetFirstPlayerController())
                {
                    int W=0,H=0;PC->GetViewportSize(W,H);const float S=FMath::Min(W/1280.f,H/800.f);
                    PC->SetMouseLocation((W-1280*S)/2+425*S,(H-800*S)/2+420*S);
                }
            if(FParse::Param(FCommandLine::Get(),TEXT("DungeonBossClarityPreview")))
            {
                StartGame();PendingSpawns=0;CancelBossIntro();DialogueLines.Empty();BossGrace=0;
                for(auto& E:Enemies)if(IsValid(E))E->Destroy();Enemies.Empty();
                for(int I=0;I<3;++I)
                {
                    auto* E=GetWorld()->SpawnActor<ADungeonEnemy>();E->Species=25+I;E->bBoss=true;E->SpawnTime=0;
                    E->Health=E->MaxHealth=100;E->SetActorLocation(DungeonView::Unproject(FVector2D(260+I*380,520)));
                    E->SetActorTickEnabled(false);Enemies.Add(E);
                }
                if(auto* H=Player(this))H->SetActorLocation(DungeonView::Unproject(FVector2D(640,700)));
            }
            if(FParse::Param(FCommandLine::Get(),TEXT("DungeonWeekendPreview")))
            {
                StartGame();PendingSpawns=0;CancelBossIntro();DialogueLines.Empty();BossGrace=0;
                for(auto& E:Enemies)if(IsValid(E))E->Destroy();Enemies.Empty();
                for(int I=0;I<4;++I)
                {
                    auto* E=GetWorld()->SpawnActor<ADungeonEnemy>();E->Species=I==0?24:I==1?0:I==2?6:12;E->bBoss=I==0;
                    E->SpawnTime=0;E->Health=E->MaxHealth=100;E->SetActorLocation(DungeonView::Unproject(FVector2D(300+I*230,490)));
                    E->SetActorTickEnabled(false);Enemies.Add(E);
                }
                if(auto* H=Player(this))H->SetActorLocation(DungeonView::Unproject(FVector2D(640,680)));
            }
            if(FParse::Param(FCommandLine::Get(),TEXT("DungeonThemeRosterPreview")))
            {
                PendingSpawns=0;for(auto& E:Enemies)if(IsValid(E))E->Destroy();Enemies.Empty();Shots.Empty();
                FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                for(int I=0;I<6;++I)
                {
                    auto* E=GetWorld()->SpawnActor<ADungeonEnemy>(ADungeonEnemy::StaticClass(),DungeonView::Unproject(FVector2D(260+(I%3)*380,390+(I/3)*250)),FRotator::ZeroRotator,Params);
                    E->Species=DungeonProgression::RosterBase(GetBiome())+I;E->Facing=I%2?3:1;E->SpawnTime=0;E->Health=E->MaxHealth=100;
                    E->Recovery=I>=3?DungeonRoster::Get(E->Species).Recovery:0;E->SetActorTickEnabled(false);Enemies.Add(E);
                }
                if(auto* H=Player(this))H->SetActorLocation(DungeonView::Unproject(FVector2D(640,710)));
            }
            if(FParse::Param(FCommandLine::Get(),TEXT("DungeonFlashPreview")))
            {
                PendingSpawns=0;for(auto& E:Enemies)if(IsValid(E))E->Destroy();Enemies.Empty();Shots.Empty();
                SpawnOneEnemy();CancelBossIntro();DialogueLines.Empty();BossGrace=0;
                for(auto& E:Enemies){E->SpawnTime=0;E->Action=1;E->ActionTime=.5f;E->ActionDuration=1.2f;E->Facing=6;E->SetActorTickEnabled(false);}
                if(auto* H=Player(this)){H->SetActorLocation(DungeonView::Unproject(FVector2D(650,610)));H->SetFlashReviewAim(FVector2D(0,FParse::Param(FCommandLine::Get(),TEXT("ReviewLeft"))?1:-1));H->SetActorTickEnabled(false);}
                FDungeonShot Grenade;Grenade.Style=13;Grenade.Art=20;Grenade.BlastRadius=300;Grenade.Origin=Grenade.Position=FVector2D(615,350);Grenade.Target=FVector2D(650,510);Grenade.FlightTime=.8f;Grenade.Life=2.25f;Shots.Add(Grenade);
            }
            if(FParse::Param(FCommandLine::Get(),TEXT("DungeonDrakePreview")))
            {
                PendingSpawns=0;for(auto& E:Enemies) if(IsValid(E)) E->Destroy();Enemies.Empty();Shots.Empty();
                FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                for(int I=0;I<2;++I)
                {
                    auto* E=GetWorld()->SpawnActor<ADungeonEnemy>(ADungeonEnemy::StaticClass(),DungeonView::Unproject(FVector2D(360+I*560,490)),FRotator::ZeroRotator,Params);
                    E->Species=19;E->Facing=I?3:1;E->SpawnTime=0;E->Health=E->MaxHealth=100;
                    E->AttackTarget=E->DrakeMouth()+FVector2D(I?-200:200,65);
                    FireAttack(E);E->Recovery=2;E->SetActorTickEnabled(false);Enemies.Add(E);
                }
                if(auto* H=Player(this))H->SetActorLocation(DungeonView::Unproject(FVector2D(640,680)));
            }
            if(FParse::Param(FCommandLine::Get(),TEXT("DungeonRewardPreview"))||FParse::Param(FCommandLine::Get(),TEXT("DungeonInventoryPreview"))||FParse::Param(FCommandLine::Get(),TEXT("DungeonChestPreview")))
            {
                PendingSpawns=0;
                for(auto& E:Enemies) if(IsValid(E)) E->Destroy();
                Enemies.Empty(); bChest=true;
                if(auto* H=Player(this))
                {
                    H->SetActorLocation(DungeonView::Unproject(FVector2D(640,424)));
                    if(!FParse::Param(FCommandLine::Get(),TEXT("DungeonChestPreview"))) PlayerInteract(H);
                    if(FParse::Param(FCommandLine::Get(),TEXT("DungeonInventoryPreview")))
                    {
                        H->Inventory.Empty();
                        for(int I=0;I<9;++I) H->AddToInventory(RollItem(I<5?I*2:I<7?24+I:36+I,I%5,4));
                        for(int I=48;I<60;++I)H->AddToInventory(RollItem(I,(I-48)%5,4));
                        H->Equip(RollItem(51,4,4));H->SelectedItem=9;
                        FParse::Value(FCommandLine::Get(),TEXT("InventoryReviewIndex="),H->SelectedItem);H->ToggleInventory();
                        if(FParse::Param(FCommandLine::Get(),TEXT("InventoryStatsPreview"))) {
                            H->Equip(RollItem(3,4,4));H->Equip(RollItem(25,4,4));H->Equip(RollItem(42,4,4));
                            if(auto* PC=Cast<APlayerController>(H->GetController())) {
                                int32 W=0,HH=0;PC->GetViewportSize(W,HH);
                                const float S=FMath::Min(W/1280.f,HH/800.f);
                                PC->SetMouseLocation(FMath::RoundToInt((W-1280*S)*.5f+220*S),FMath::RoundToInt((HH-800*S)*.5f+350*S));
                            }
                        }
                    }
                }
            }
        }
        static bool TeaPrepared=false;
        if(!TeaPrepared&&Time>3.85f&&FParse::Param(FCommandLine::Get(),TEXT("DungeonTeaPreview")))
        {
            TeaPrepared=true; PendingSpawns=0;
            for(auto& E:Enemies) if(IsValid(E)) E->Destroy();
            Enemies.Empty(); Shots.Empty();
            FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            for(int I=0;I<2;++I)
            {
                auto* E=GetWorld()->SpawnActor<ADungeonEnemy>(ADungeonEnemy::StaticClass(),DungeonView::Unproject(FVector2D(600+I*80,425)),FRotator::ZeroRotator,Params);
                E->Health=E->MaxHealth=200; E->SpawnTime=0; E->SetActorTickEnabled(false); Enemies.Add(E);
            }
            if(auto* H=Player(this)) { H->SetActorLocation(DungeonView::Unproject(FVector2D(640,610))); LaunchTea(H,FVector2D(640,425)); }
        }
        if(FParse::Param(FCommandLine::Get(),TEXT("DungeonDescentPreview"))&&Time>4.f)
        {
            for(auto& E:Enemies) if(IsValid(E)) E->Destroy();
            Enemies.Empty();PendingSpawns=0;bChest=false;bLootClaimed=true;
            TransitionDoor=1;TransitionFrom=DoorPosition(1)+FVector2D(0,75);TransitionTime=DungeonDescent::Duration*.4f;
        }
        if(FParse::Param(FCommandLine::Get(),TEXT("DungeonBreakablesPreview"))&&Time>4.1f)
            for(int I=3;I<Breakables.Num();++I)if(Breakables[I].BrokenAge<0){Breakables[I].BrokenAge=I==5?2.f:0.f;Breakables[I].Loot=RollChestLoot(false,Room);}
        if(!Captured&&Time>4.5f)
        {
            Captured=true;
            FString Name=FParse::Param(FCommandLine::Get(),TEXT("DungeonRewardPreview"))?TEXT("RewardReview"):Room==2?TEXT("BossReview"):TEXT("ArenaReview");
            if(FParse::Param(FCommandLine::Get(),TEXT("DungeonInventoryPreview"))) Name=TEXT("InventoryReview");
            if(FParse::Param(FCommandLine::Get(),TEXT("DungeonTraderPreview"))) Name=TEXT("TraderReview");
            if(FParse::Param(FCommandLine::Get(),TEXT("DungeonBreakablesPreview"))) Name=FString::Printf(TEXT("BreakablesReview%d"),GetBiome());
            if(FParse::Param(FCommandLine::Get(),TEXT("DungeonMenuPreview"))) Name=TEXT("MenuReview");
            if(FParse::Param(FCommandLine::Get(),TEXT("DungeonControlsPreview"))) Name=TEXT("ControlsReview");
            if(FParse::Param(FCommandLine::Get(),TEXT("DungeonHeroReviewPreview")))
            { int Group=0;FParse::Value(FCommandLine::Get(),TEXT("DungeonBiome="),Group);Name=FString::Printf(TEXT("AthleticHeroReview%d"),Group); }
            if(FParse::Param(FCommandLine::Get(),TEXT("DungeonMenuHoverPreview"))) Name=TEXT("MenuHoverReview");
            if(FParse::Param(FCommandLine::Get(),TEXT("DungeonWeekendPreview"))) Name=TEXT("WeekendReview");
            if(FParse::Param(FCommandLine::Get(),TEXT("DungeonBossClarityPreview"))) Name=TEXT("BossClarityReview");
            if(FParse::Param(FCommandLine::Get(),TEXT("DungeonRosterPreview"))) Name=TEXT("RosterReview");
            if(FParse::Param(FCommandLine::Get(),TEXT("DungeonThemeRosterPreview"))) Name=FString::Printf(TEXT("ThemeRosterReview%d"),GetBiome());
            if(FParse::Param(FCommandLine::Get(),TEXT("DungeonRollPreview"))) Name=TEXT("RollReview");
            if(FParse::Param(FCommandLine::Get(),TEXT("DungeonIdlePreview"))) Name=TEXT("IdleReview");
            if(FParse::Param(FCommandLine::Get(),TEXT("DungeonDescentPreview"))) Name=TEXT("DescentReview");
            if(FParse::Param(FCommandLine::Get(),TEXT("DungeonFXPreview"))) Name=TEXT("FXReview");
            if(FParse::Param(FCommandLine::Get(),TEXT("DungeonTeaPreview"))) Name=TEXT("TeaReview");
            if(FParse::Param(FCommandLine::Get(),TEXT("DungeonChestPreview"))) Name=TEXT("ChestReview");
            if(FParse::Param(FCommandLine::Get(),TEXT("DungeonBossPreview"))) Name=FString::Printf(TEXT("BossReview%d"),GetBossSpecies());
            if(FParse::Param(FCommandLine::Get(),TEXT("DungeonEquipmentReview"))) Name=FParse::Param(FCommandLine::Get(),TEXT("ReviewLeft"))?TEXT("EquipmentLeftReview"):TEXT("EquipmentReview");
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectDir()/TEXT("ArtSource")/(Name+TEXT(".png")),true,false);
        }
        if(Time>5.2f&&FParse::Param(FCommandLine::Get(),TEXT("TraderUISmoke"))) {
            int Errors=1;if(auto* PC=GetWorld()->GetFirstPlayerController())if(auto* HUD=Cast<ADungeonHUD>(PC->GetHUD()))Errors=HUD->VerifyTraderUI();
            UE_LOG(LogTemp,Display,TEXT("TRADER_UI_SMOKE errors=%d"),Errors);FPlatformMisc::RequestExitWithStatus(false,Errors?1:0);return;
        }
        if(Time>6.f) FPlatformMisc::RequestExit(false);
        if(Prepared&&FParse::Param(FCommandLine::Get(),TEXT("DungeonDrakePreview"))) return;
    }
#endif
    if(IsBossIntroActive()) { UpdateBossIntro(Dt);return; }
    if(bMenu||bTraderOpen) return;
    if(IsBossDialogueActive()) { DialogueWait=FMath::Max(0.f,DialogueWait-Dt); return; }
    if(BossGrace>0) { BossGrace=FMath::Max(0.f,BossGrace-Dt); return; }
    if(auto* H=Player(this))
    {
        H->QuipTime=FMath::Max(0.f,H->QuipTime-Dt);
        H->QuipCooldown=FMath::Max(0.f,H->QuipCooldown-Dt);
    }
    if(TransitionTime>0)
    {
        TransitionTime=FMath::Max(0.f,TransitionTime-Dt);
        if(auto* H=Player(this))
        {
            const float T=TransitionProgress(); const auto Door=DoorPosition(TransitionDoor);
            if(T<.25f) H->TransitionWalk(TransitionFrom,Door,T/.25f);
            else H->TransitionWalk(Door,Door-FVector2D(0,DungeonDescent::StairDistance),FMath::Clamp((T-.25f)/.6f,0.f,1.f),DungeonDescent::StairGaitScale);
        }
        if(TransitionTime<=0) NextRoom();
        return;
    }
    if(auto* H=Player(this)) if(H->IsInventoryOpen()) return;
    UpdateFreedom(Dt);
    for(auto& B:Blood) B.Age+=Dt;
    Blood.RemoveAll([](const FDungeonBlood& B){ return B.Age>14.f; });
    LootTimer=FMath::Max(0.f,LootTimer-Dt); TransitionCooldown=FMath::Max(0.f,TransitionCooldown-Dt);
    for(auto& Impact:Impacts) Impact.Life-=Dt;
    Impacts.RemoveAll([](const FDungeonImpact& I){return I.Life<=0;});
    if(IsDead()) return;
    UpdatePotions(Dt);
    UpdateReward(Dt);
    for(auto& Prop:Breakables)if(Prop.BrokenAge>=0)Prop.BrokenAge+=Dt;
    UpdateProjectiles(Dt);
    if(PendingSpawns>0&&!bChest&&!bLootClaimed)
    {
        SpawnTimer-=Dt;
        if(SpawnTimer<=0) { SpawnOneEnemy(); --PendingSpawns; SpawnTimer=.8f; }
    }
}
bool ADungeonGameMode::IsDead() const { auto* H=Player(const_cast<ADungeonGameMode*>(this)); return H&&H->Health<=0; }
void ADungeonGameMode::SpawnWave()
{
    // Combat and rewards must never coexist, including when a wave is rescheduled.
    bChest=bLootClaimed=bLootRolled=false; Reward=FRewardPresentation();LootTimer=0;
    if(Wave==1) { RosterCursor=FMath::RandRange(0,5);SpawnBreakables(); }
    PendingSpawns=IsBossRoom()?1:FMath::Min(9,3+Wave+(Room-1)%DungeonProgression::RoomsPerChapter); SpawnTimer=2.f;
    UE_LOG(LogTemp,Display,TEXT("ROOM_FLOW room=%d wave=%d queued=%d"),Room,Wave,PendingSpawns);
}
void ADungeonGameMode::CompleteRoom()
{
    if(HasEnding()) return;
    if(!Enemies.IsEmpty()||PendingSpawns>0||bChest||bLootClaimed) return;
    if(Room>=DungeonProgression::CampaignRooms) { FinishRun(!IsDead());return; }
    SpawnTimer=0;
    Reward=FRewardPresentation();bChest=true;
    UE_LOG(LogTemp,Display,TEXT("ROOM_FLOW room=%d reward ready; no live or queued enemies"),Room);
}
void ADungeonGameMode::SpawnOneEnemy()
{
    if(HasEnding()) return;
    if(bChest||bLootClaimed||IsTransitioning())
    {
        UE_LOG(LogTemp,Warning,TEXT("ROOM_FLOW blocked late spawn in reward/transition room=%d"),Room);
        return;
    }
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    FVector2D Pos;
    if(IsBossRoom()) Pos=FVector2D(640,GetBiome()==0?310:390);
    else
    {
        float A=FMath::FRandRange(0.f,2*PI);
        Pos=FVector2D(640+FMath::Cos(A)*450,455+FMath::Sin(A)*190);
    }
    if(auto* E=GetWorld()->SpawnActor<ADungeonEnemy>(DungeonView::Unproject(Pos),FRotator::ZeroRotator,Params))
    {
        E->bBoss=IsBossRoom(); E->Species=E->bBoss?GetBossSpecies():DungeonProgression::RosterBase(GetBiome())+(RosterCursor++%6);
        if(E->bBoss&&E->Species>=28) E->Facing=4;
        const auto& S=DungeonRoster::Get(E->Species);
        E->MaxHealth=E->Health=DungeonCombatBalance::SpawnHealth(S.HP,Room,E->bBoss);
        Enemies.Add(E);
        if(E->bBoss) { E->SpawnTime=0; BeginBossDialogue(); }
        PlaySound(TEXT("Spawn"),.4f,E->bBoss?.65f:1.f);
    }
}
void ADungeonGameMode::PlayerAttack(ADungeonHero* H)
{
    if(!H||IsGameplayBlocked()) return;
    const FVector2D P=DungeonView::Project(H->GetActorLocation());
    TArray<TObjectPtr<ADungeonEnemy>> Hits;
    StrikeBreakables(H);
    for(auto& E:Enemies) if(IsValid(E)&&E->SpawnTime<=0)
    {
        FVector2D V=DungeonView::Project(E->GetActorLocation())-P;
        if(DungeonCombatBalance::MeleeHits(V,H->GetAim(),DungeonRoster::RenderSize(E->Species),E->bBoss)) Hits.Add(E);
    }
    ++H->StrikeCount;
    for(auto& E:Hits) if(IsValid(E)&&E->Health>0)
    {
        const bool Crit=FMath::FRand()<H->CritChance;
        float Damage=H->AttackPower*FMath::FRandRange(.9f,1.1f)*(Crit?H->CritMultiplier:1.f);
        if(H->HasEffect(8)&&E->Health<E->MaxHealth*.3f) Damage*=1.35f;
        if(H->HasEffect(9)&&H->Health<H->MaxHealth*.4f) Damage*=1.3f;
        const float Dealt=FMath::Min(E->Health,Damage);
        if(FMath::FRand()<H->BleedChance) { E->BleedTime=4;E->BleedDPS=Damage*.2f; }
        if(FMath::FRand()<H->PoisonChance) { E->PoisonTime=6;E->PoisonDPS=Damage*.14f; }
        if(H->HasEffect(3)) E->SlowTime=3;
        E->TakeDungeonDamage(Damage);
        if(H->HasEffect(4)) H->Health=FMath::Min(H->MaxHealth,H->Health+Dealt*.05f);
        if(Crit&&H->HasEffect(6)) H->Stamina=FMath::Min(H->MaxStamina,H->Stamina+12);
    }
    if(!Hits.IsEmpty()&&H->HasEffect(7)&&H->StrikeCount%3==0)
    {
        const auto Targets=Enemies; AddImpact(P,0,true);
        for(auto& E:Targets) if(IsValid(E)&&E->SpawnTime<=0&&FVector2D::Distance(P,DungeonView::Project(E->GetActorLocation()))<190) E->TakeDungeonDamage(H->AttackPower*.35f);
    }
}
void ADungeonGameMode::EnemyDefeated(ADungeonEnemy* E)
{
    if(!IsValid(E)||!Enemies.Contains(E)) return;
    if(auto* H=Player(this)) if(H->Health>0&&H->HasEffect(5)) H->Health=FMath::Min(H->MaxHealth,H->Health+6);
    if(E->Species!=24)
    {
        if(Blood.Num()>=96) Blood.RemoveAt(0);
        FDungeonBlood B; B.Position=DungeonView::Project(E->GetActorLocation()); B.Size=E->bBoss?125:80;
        B.bRemains=true; B.Variant=(E->Species==6||E->Species==15||E->Species==22||E->Species==25)?2:(E->Species<=1||E->Species==5)?3:E->Species%2;
        Blood.Add(B);
    }
    if(!IsFreedomActive()) FreedomKills=FMath::Min(15,FreedomKills+1);
    if(E->bBoss||FMath::FRand()<.12f) { FDungeonPotion P; P.Position=DungeonView::Clamp(DungeonView::Project(E->GetActorLocation())); Potions.Add(P); }
    Enemies.Remove(E); E->Destroy();
    if(Enemies.IsEmpty()&&PendingSpawns==0)
    {
        if(!IsBossRoom()&&Wave<2) { ++Wave; SpawnWave(); }
        else CompleteRoom();
    }
}
void ADungeonGameMode::AddImpact(FVector2D P,float Damage,bool Boss)
{
    if(Damage>0)
    {
        if(Blood.Num()>=96) Blood.RemoveAt(0);
        FDungeonBlood B; B.Position=P; B.Size=FMath::Clamp(60.f+Damage*.5f,65.f,130.f); B.Variant=FMath::RandRange(0,3); Blood.Add(B);
    }
    FDungeonImpact I; I.Position=P; I.Damage=Damage; I.bBoss=Boss; Impacts.Add(I);
}
FDungeonItem ADungeonGameMode::MakeItem(int32 Icon,int32 Rarity)
{
    const TCHAR* Names[]={TEXT("Keepsteel Longsword"),TEXT("Gravebreaker Axe"),TEXT("Cinderbrand"),
        TEXT("Sentinel Plate"),TEXT("Verdant Scale"),TEXT("Warden's Carapace"),
        TEXT("Jade Heart"),TEXT("Sapphire Ward"),TEXT("Emberheart")};
    FDungeonItem I; I.Icon=FMath::Clamp(Icon,0,8); I.Slot=I.Icon/3; I.Rarity=FMath::Clamp(Rarity,0,4); I.Name=Names[I.Icon];
    float Mult=1.f+I.Rarity*.65f;
    if(I.Slot==0) I.Attack=(6+I.Icon*3)*Mult;
    if(I.Slot==1) { I.Defense=(5+(I.Icon-3)*2)*Mult; I.Vitality=5*Mult; }
    if(I.Slot==2) I.Vitality=(20+(I.Icon-6)*8)*Mult;
    I.CoinValue=FMath::RoundToInt(25*Mult);
    return I;
}
FDungeonItem ADungeonGameMode::RollChestLoot(bool Boss,int32 Level)
{
    const int32 Roll=FMath::RandRange(1,100);
    const int32 Rarity=Roll<=35?0:Roll<=65?1:Roll<=85?2:Roll<=97?3:4;
    return RollItem(FMath::RandRange(0,59),Boss?4:Rarity,Level);
}
void ADungeonGameMode::PlayerInteract(ADungeonHero* H)
{
    if(!H) return;
    if(H->IsInventoryOpen()||IsGameplayBlocked()||H->IsRolling()) return;
    if(IsDead()) { RestartRun(); return; }
    // Exits remain usable even while an uncollected reward is on the floor.
    if(AreDoorsOpen()&&TransitionCooldown<=0)
    {
        for(int32 I=0;I<3;++I) if(FVector2D::Distance(DungeonView::Project(H->GetActorLocation()),DoorPosition(I))<95)
        { StartTransition(I); return; }
    }
    if(CollectBreakableLoot(H)) return;
    if(bChest) InteractReward(H);
}
void ADungeonGameMode::NextRoom()
{
    if(HasEnding()) return;
    if(!AreDoorsOpen()||!Enemies.IsEmpty()||PendingSpawns>0) return;
    if(DungeonProgression::TraderAfter(Room)&&!bTraderVisited){OpenTrader();return;}
    bTraderVisited=false;
    CancelBossIntro();
    FreedomTime=0;bFreedomResolved=false;
    Reward=FRewardPresentation();
    if(auto* H=Player(this)) H->StunTime=H->SlowTime=H->FlashBlindTime=0;
    Blood.Empty();
    Potions.Empty();
    DialogueLines.Empty(); DialogueIndex=0; DialogueWait=BossGrace=0;
    bChest=bLootClaimed=bLootRolled=false; LootTimer=0; ++Room; Wave=1;
    for(bool& Rolled:ChestRolled) Rolled=false;
    Shots.Empty(); Splashes.Empty();
    if(auto* H=Player(this)) H->SetActorLocation(DungeonView::Unproject(FVector2D(640,615)));
    SpawnWave();
}
void ADungeonGameMode::RestartRun()
{
    bTraderOpen=bTraderVisited=false;TraderStock.Empty();TraderMessage.Empty();
    EndState=0;EndTime=0;bEndingCleaned=false;
    CancelBossIntro();
    Reward=FRewardPresentation();
    Blood.Empty(); FreedomKills=0; FreedomTime=0; bFreedomResolved=false;
    Potions.Empty();
    DialogueLines.Empty(); DialogueIndex=0; DialogueWait=BossGrace=0;
    for(auto& E:Enemies) if(IsValid(E)) E->Destroy();
    Enemies.Empty(); Impacts.Empty(); Room=Wave=1; bChest=bLootClaimed=bLootRolled=false; LootTimer=0;
    TransitionTime=TransitionCooldown=0; Shots.Empty(); Splashes.Empty(); for(bool& Rolled:ChestRolled) Rolled=false;
    if(auto* H=Player(this)) H->Restart();
    SpawnWave();
}
FString ADungeonGameMode::GetObjective() const
{
    if(IsDead()) return TEXT("YOU FELL  -  Press E to begin a new run");
    if(bChest) return bLootRolled?TEXT("Exits unlocked / E to collect loot or enter a glowing arch"):TEXT("Choose one mystery chest / E to open");
    if(bLootClaimed) return TEXT("Reward collected - I to equip gear / E at a glowing arch to continue");
    if(IsBossRoom()) return PendingSpawns?TEXT("A GUARDIAN AWAKENS"):TEXT("Watch the telegraph / SPACE to dodge / strike during recovery");
    return FString::Printf(TEXT("ROOM %d   /   WAVE %d OF 2   /   %d REMAINING"),Room,Wave,Enemies.Num()+PendingSpawns);
}

UTexture2D* ADungeonHUD::Texture(const FString& Name)
{
    if(auto* T=Textures.Find(Name)) return *T;
    const bool NewArt=Name.StartsWith(TEXT("Flash"))||Name.StartsWith(TEXT("Mack_"))||Name.StartsWith(TEXT("Twister_"))||Name.StartsWith(TEXT("BossPortrait_"))||Name.StartsWith(TEXT("BossName_"))||Name.StartsWith(TEXT("BossBorder_"))||Name.StartsWith(TEXT("Status_"))||Name.StartsWith(TEXT("RewardChest_"))||Name.StartsWith(TEXT("RarityGlow_"))||Name.StartsWith(TEXT("LootEffect_"))||Name.StartsWith(TEXT("BurgerSplat_"))||Name.StartsWith(TEXT("FoodDebris_"))||Name.StartsWith(TEXT("MackLanding_"));
    FString Path=FString::Printf(TEXT("/Game/Art/%s/%s.%s"),Name.StartsWith(TEXT("Ending"))?TEXT("Endings"):Name.StartsWith(TEXT("Intro"))?TEXT("Intros"):Name.StartsWith(TEXT("Theme"))?TEXT("Progression"):NewArt?TEXT("September"):TEXT("V2"),*Name,*Name);
    auto* T=LoadObject<UTexture2D>(nullptr,*Path);
#if WITH_EDITOR
    if(T&&T->IsCompiling())
    {
        UTexture* Pending[]={T}; FTextureCompilingManager::Get().FinishCompilation(Pending);
    }
#endif
    if(T) T->WaitForPendingInitOrStreaming();
    Textures.Add(Name,T); return T;
}
void ADungeonHUD::Sprite(const FString& N,float X,float Y,float W,float H,FLinearColor Tint,float Rotation,FVector2D Pivot)
{
    if(auto* T=Texture(N)) DrawTexture(T,Offset.X+X*Scale,Offset.Y+Y*Scale,W*Scale,H*Scale,0,0,1,1,Tint,BLEND_Translucent,1,false,Rotation,Pivot);
}
void ADungeonHUD::Box(float X,float Y,float W,float H,FLinearColor Color)
{ DrawRect(Color,Offset.X+X*Scale,Offset.Y+Y*Scale,W*Scale,H*Scale); }
void ADungeonHUD::Label(const FString& T,float X,float Y,FLinearColor C,float Size)
{ DrawText(T,C,Offset.X+X*Scale,Offset.Y+Y*Scale,GEngine->GetMediumFont(),Size*Scale); }
void ADungeonHUD::Ring(FVector2D C,float R,FLinearColor Color,float Width)
{
    for(int32 I=0;I<40;++I)
    {
        float A=I*2*PI/40,B=(I+1)*2*PI/40;
        FVector2D P=C+FVector2D(FMath::Cos(A)*R,FMath::Sin(A)*R*.65f),Q=C+FVector2D(FMath::Cos(B)*R,FMath::Sin(B)*R*.65f);
        DrawLine(Offset.X+P.X*Scale,Offset.Y+P.Y*Scale,Offset.X+Q.X*Scale,Offset.Y+Q.Y*Scale,Color,Width*Scale);
    }
}
void ADungeonHUD::Shadow(FVector2D Center,float Radius,float Opacity)
{
    for(int32 Y=-6;Y<=6;++Y)
    {
        const float Half=Radius*FMath::Sqrt(FMath::Max(0.f,1.f-Y*Y/49.f));
        Box(Center.X-Half,Center.Y+Y*1.5f,Half*2,1.5f,FLinearColor(0,0,0,.28f*Opacity));
    }
}
void ADungeonHUD::Hero(ADungeonHero* H,float HS)
{
    float Opacity=1.f,DescentScale=1.f;
    if(auto* G=Cast<ADungeonGameMode>(UGameplayStatics::GetGameMode(this));G&&G->IsTransitioning())
    {
        DescentScale=DungeonDescent::Size(G->TransitionProgress());
        Opacity=DungeonDescent::Opacity(G->TransitionProgress()); HS*=DescentScale;
    }
    if(Opacity<=0.f) return;
    const FLinearColor Fade(1,1,1,Opacity);
    FVector2D P=DungeonView::Project(H->GetActorLocation());
    if(H->IsRolling())
    {
        Shadow(P,27);
        FString RollName=FString::Printf(TEXT("Roll_%d_%d"),H->GetRollDirection(),FMath::Clamp((int)(H->RollProgress()*8),0,7));
        Sprite(TEXT("Athletic_")+RollName,P.X-64*HS,P.Y-116*HS,128*HS,128*HS,FLinearColor::White);
        return;
    }
    const int32 D=H->GetFacingDirection();
    const bool Action=H->IsAttacking()||H->IsCasting();
    const int32 View=Action?AthleticHeroSockets::AttackView(D,H->GetAnimationFrame()):D;
    const int32 F=Action?AthleticHeroSockets::AttackPose[D][H->GetAnimationFrame()]:H->IsWalking()?H->GetAnimationFrame():AthleticHeroSockets::IdleFrame[D];
    const float BreathWeight=H->Health>0&&!H->IsAttacking()&&!H->IsCasting()&&!H->IsRolling()?FMath::Max(H->IdleBreathBlend,H->IsWalking()?.18f:0.f):0;
    const float FacingX=D==0||D==4?0:D<4?1.f:-1.f;
    auto Pose=[&](FVector2D Q){return HeroBreathing::Map(Q,H->BreathPhase,BreathWeight,FacingX);};
    const float Bob=H->IsWalking()?FMath::Sin(H->WalkCycle()*2.f)*.6f:0.f;
    P.Y+=Bob;
    FString Name=FString::Printf(TEXT("Athletic_%s%s_%d_%d"),Action?TEXT("Attack"):TEXT("Walk"),View%2?TEXT("Diagonal"):TEXT("Cardinal"),View/2,F);
    Shadow(P+FVector2D(0,-2-Bob),28*DescentScale,Opacity);
    // Native alpha avoids the old body mask and its mismatched silhouette.
    auto DrawBody=[&]() { if(auto* T=Texture(Name)) for(int Row=0;Row<128;Row+=2)
    {
        const auto L=Pose(FVector2D(0,Row+1)),R=Pose(FVector2D(128,Row+1));
        const auto Top=Pose(FVector2D(64,Row)),Bottom=Pose(FVector2D(64,Row+2));
        DrawTexture(T,Offset.X+(P.X+(L.X-64)*HS)*Scale,Offset.Y+(P.Y+(Top.Y-116)*HS)*Scale,
            (R.X-L.X)*HS*Scale,(Bottom.Y-Top.Y)*HS*Scale,0,Row/128.f,1,2.f/128,H->HurtTime>0?FLinearColor(1,.35f,.3f,Opacity):Fade,BLEND_Translucent);
    }
    };
    const bool Behind=AthleticHeroSockets::RightHandBehindBody(View);
    if(!Behind||H->IsCasting()||H->Equipment.Num()<3) DrawBody();
    if(H->Equipment.Num()<3) return;
    if(H->IsCasting())
    {
        if(H->GetCastProgress()<.46f)
        {
            const auto Hand=P+(Pose(AthleticHeroSockets::Attack[View][F])-FVector2D(64,116))*HS;
            KeySprite(TEXT("TeaFX_0"),Hand.X-17,Hand.Y-22,34,34,FLinearColor::White);
        }
        return;
    }
    const bool Front=D>=2&&D<=6;
    if(H->Equipment[2].Vitality>0&&Front)
    {
        const auto Charm=P+(Pose(FVector2D(64,65))-FVector2D(64,116))*HS;
        Sprite(ItemArt(H->Equipment[2].Icon),Charm.X-7*HS,Charm.Y-7*HS,14*HS,14*HS,Fade);
    }
    const auto A=H->GetAim();
    const FVector2D Socket=H->IsAttacking()?AthleticHeroSockets::Attack[View][F]:AthleticHeroSockets::Walk[D][F];
    const FVector2D Origin=P-FVector2D(64,116)*HS;
    const FVector2D Hand=Origin+Pose(Socket)*HS;
    const float Angle=H->IsAttacking()?AthleticHeroSockets::Angles[View][F]:(D<=4?145.f:215.f);
    if(!H->Equipment[0].IsEmpty())
    {
        const FVector2D Pivot=H->Equipment[0].CatalogId<0?HeroSockets::Grip(H->Equipment[0].Icon)/128.:HeroSockets::CatalogGrip(H->Equipment[0].CatalogId);
        float WeaponSize=96*HS*H->Equipment[0].EquippedScale();
#if !UE_BUILD_SHIPPING
        if(FParse::Param(FCommandLine::Get(),TEXT("DungeonWeaponOriginal"))) WeaponSize=96*HS;
#endif
        Sprite(H->Equipment[0].Art(),Hand.X-Pivot.X*WeaponSize,Hand.Y-Pivot.Y*WeaponSize,
            WeaponSize,WeaponSize,Fade,Angle,Pivot);
        // The right arm is on the far side in left/rear-left views. Never draw
        // its handle across the near (left) arm or move the grip to that hand.
        if(Behind) DrawBody();
        // Re-draw only the gripping fingers over the handle, never a second hero.
        if(auto* T=Texture(Name))
        {
            const auto TL=Origin+Pose(Socket-FVector2D(2.5f,2.5f))*HS,BR=Origin+Pose(Socket+FVector2D(2.5f,2.5f))*HS;
            DrawTexture(T,Offset.X+TL.X*Scale,Offset.Y+TL.Y*Scale,(BR.X-TL.X)*Scale,(BR.Y-TL.Y)*Scale,
                (Socket.X-2.5f)/128.,(Socket.Y-2.5f)/128.,5.f/128,5.f/128,Fade,BLEND_Translucent);
        }
    }
    if(Behind&&H->Equipment[0].IsEmpty()) DrawBody();
    if(!H->Equipment[0].IsEmpty()&&H->IsAttacking()&&H->GetAttackProgress()>.25f&&H->GetAttackProgress()<.8f)
    {
        float Base=FMath::Atan2(A.Y,A.X);
        float End=Base-1.3f+H->GetAttackProgress()*3.2f;
        for(int I=0;I<12;++I)
        {
            float T=End-I*.065f; FVector2D Q=P+FVector2D(FMath::Cos(T)*76,FMath::Sin(T)*56-30);
            Box(Q.X,Q.Y,4,3,FLinearColor(1,.82f,.4f,1.f-I/12.f));
        }
    }
}
void ADungeonHUD::Enemy(ADungeonEnemy* E)
{
    FVector2D P=DungeonView::Project(E->GetActorLocation());
    const auto& S=DungeonRoster::Get(E->Species);
    const float Size=DungeonRoster::RenderSize(E->Species);
    Shadow(P,Size*.18f);
    if(E->SpawnTime>0) Ring(P,32+E->SpawnTime*25,FLinearColor(.5f,.25f,1,.8f),3);
    const float Lift=S.Flying?18+FMath::Sin(GetWorld()->GetTimeSeconds()*4+E->Species)*5:0;
    const float Breath=1.f+FMath::Sin(GetWorld()->GetTimeSeconds()*(E->IsWalking()?5.5f:2.5f)+E->Species)*.012f;
    const float StepBob=E->IsWalking()&&!S.Flying?FMath::Sin(E->WalkDistance/24.f*PI)*1.4f:0;
    FLinearColor Tint=E->IsHurt()?FLinearColor(1,.45f,.35f):FLinearColor::White;
    const float Pulse=.5f+.5f*FMath::Sin(GetWorld()->GetTimeSeconds()*8);
    if(E->BleedTime>0||E->PoisonTime>0||E->SlowTime>0) {
        if(E->PoisonTime>0) Tint=FLinearColor::LerpUsingHSV(FLinearColor::White,FLinearColor(.35f,1,.18f),.35f+.45f*Pulse);
        else if(E->BleedTime>0) Tint=FLinearColor(1,1-.6f*Pulse,1-.65f*Pulse);
        else if(E->SlowTime>0) Tint=FLinearColor(.55f,.8f,1);
    }
    if(E->BleedTime>0||E->PoisonTime>0) for(int I=0;I<4;++I) {
        const float Age=FMath::Fmod(GetWorld()->GetTimeSeconds()*1.4f+I*.27f,1.f);
        Box(P.X-18+I*12,P.Y-35+Age*32,3,5,E->PoisonTime>0?FLinearColor(.3f,1,.1f,1-Age):FLinearColor(.8f,.04f,.04f,1-Age));
    }
    // Keep the pixel art opaque from its first visible frame; the ring telegraphs spawning.
    Tint.A=1.f;
    if(E->Species==30)
    {
        const int Frame=E->Action==2?(E->ActionTime>.32f?12:E->ActionTime>.12f?14:15):E->ActionTime>0?8+FMath::Clamp(int((1-E->ActionTime/E->ActionDuration)*8),0,7):E->bWalking?int(E->WalkDistance/20)%8:15;
        const bool Left=E->Facing==6;
        if(auto* T=Texture(FString::Printf(TEXT("FlashGuy_%d"),Frame)))
            DrawTexture(T,Offset.X+(P.X-Size*.5f)*Scale,Offset.Y+(P.Y-Size*116.f/128.f)*Scale,Size*Scale,Size*Scale,Left?1:0,0,Left?-1:1,1,Tint);
        DrawStatus(P-FVector2D(0,Size+20),0,E->SlowTime,E->PoisonTime,E->BleedTime,E->FreedomImmuneTime>0,E->MotionClock);
        if(E->Action==2)
        {
            const float Side=Left?-1.f:1.f,Thrust=FMath::Sin((1-E->ActionTime/.5f)*PI)*18;
            // Steel combat knife at the extended throwing-hand socket, pointed into the stab.
            const auto Hand=P+FVector2D(Side*(Size*.27f+Thrust),-Size*.49f);
            Box(Hand.X-Side*8,Hand.Y-2,Side*10,4,FLinearColor(.12f,.13f,.12f));
            DrawLine(Offset.X+Hand.X*Scale,Offset.Y+Hand.Y*Scale,Offset.X+(Hand.X+Side*23)*Scale,Offset.Y+(Hand.Y-2)*Scale,FLinearColor(.82f,.9f,.92f),4*Scale);
        }
        return;
    }
    if(E->Species>=31)
    {
        if(auto* T=Texture(FString::Printf(TEXT("ThemeEnemy_%d_%d"),E->Species,E->AnimationFrame())))
            DrawTexture(T,Offset.X+(P.X-Size/2)*Scale,Offset.Y+(P.Y-Size*116.f/128.f*Breath-Lift-StepBob)*Scale,Size*Scale,Size*Breath*Scale,E->Facing==3?1:0,0,E->Facing==3?-1:1,1,Tint);
        DrawStatus(P-FVector2D(0,Size+22),0,E->SlowTime,E->PoisonTime,E->BleedTime,E->FreedomImmuneTime>0,E->MotionClock);
        Box(P.X-25,P.Y-Size*.82f-Lift,50,4,FLinearColor(.12f,.02f,.025f));
        Box(P.X-25,P.Y-Size*.82f-Lift,50*E->Health/E->MaxHealth,4,FLinearColor(.8f,.15f,.12f));return;
    }
    if(E->Species>=28)
    {
        int Frame=0; float Hop=0;
        const float T=E->ActionDuration>0?1-E->ActionTime/E->ActionDuration:0;
        if(E->Species==28)
        {
            static const int Idle[]={0,1,2,3,2,1}; Frame=Idle[int(E->MotionClock*6)%6];
            if(E->Action==1) { Frame=4+FMath::Clamp(int(T*8),0,7); Hop=FMath::Sin(FMath::Clamp((T-.2f)/.6f,0.f,1.f)*PI)*65; }
            if(E->Action==2) Frame=12+FMath::Clamp(int(T*4),0,3);
        }
        else
        {
            Frame=int(E->MotionClock*12)%12;
            if(E->Action==2) { Frame=12+FMath::Clamp(int(T*6),0,5); if(Frame==14&&E->FlashTime<=0) Frame=15; if(E->FlashTime>0) Frame=14; }
            if(E->Action==3) Frame=T<.3f?18:E->FlashTime>0?19+(E->ShotSerial%2):22+int(E->MotionClock*12)%2;
        }
        Sprite(FString::Printf(TEXT("%s_%d_%d"),E->Species==28?TEXT("Mack"):TEXT("Twister"),E->Facing,Frame),P.X-Size/2,P.Y-Size*(E->Species==28?372.f/384.f:360.f/384.f)-Hop,Size,Size,Tint);
        DrawStatus(P-FVector2D(0,Size+20),0,E->SlowTime,E->PoisonTime,E->BleedTime,E->FreedomImmuneTime>0,E->MotionClock);
        return;
    }
    KeySprite(E->Species==24?FString::Printf(TEXT("Finance_%d"),E->AnimationFrame()):FString::Printf(TEXT("Creature_%d_%d"),E->Species,E->AnimationFrame()),
        P.X-Size/2,P.Y-Size*116.f/128.f*Breath-Lift+StepBob,Size,Size*Breath,Tint,E->Facing==3);
    if(E->Species==19&&E->Recovery>S.Recovery-.22f)
    {
        const auto Mouth=E->DrakeMouth(),Aim=E->ChargeAim;
        const float FlamePulse=1.f+.06f*FMath::Sin(E->Recovery*90);
        // TeaFX_4's tail lies at ~15% width; keep that end at the jaw, not its center.
        const float W=82*FlamePulse,H=38*FlamePulse;
        const auto Center=Mouth+Aim*(W*.35f);
        KeySprite(TEXT("TeaFX_4"),Center.X-W*.5f,Center.Y-H*.5f,W,H,FLinearColor::White,false,FMath::RadiansToDegrees(FMath::Atan2(Aim.Y,Aim.X)));
    }
    DrawStatus(P-FVector2D(0,Size+32),0,E->SlowTime,E->PoisonTime,E->BleedTime,E->FreedomImmuneTime>0,E->MotionClock);
    if(!E->bBoss)
    {
        Box(P.X-25,P.Y-Size-2-Lift,50,4,FLinearColor(.12f,.02f,.025f));
        Box(P.X-25,P.Y-Size-2-Lift,50*E->Health/E->MaxHealth,4,FLinearColor(.8f,.15f,.12f));
    }
}
void ADungeonHUD::KeySprite(const FString& Name,float X,float Y,float W,float H,FLinearColor Tint,bool Flip,float Rotation)
{
    UMaterialInstanceDynamic* M=nullptr;
    const FString Key=TEXT("Key_")+Name;
    if(auto* Cached=ArmorMaterials.Find(Key)) M=*Cached;
    else if(auto* Base=LoadObject<UMaterialInterface>(nullptr,Name.StartsWith(TEXT("TeaFX_"))?TEXT("/Game/Art/V2/M_TeaFX.M_TeaFX"):TEXT("/Game/Art/V2/M_KeySprite.M_KeySprite")))
    {
        M=UMaterialInstanceDynamic::Create(Base,this);
        M->SetTextureParameterValue(TEXT("SpriteTexture"),Texture(Name)); ArmorMaterials.Add(Key,M);
    }
    if(M&&!M->GetMaterial()->IsCompiling())
    {
        M->SetVectorParameterValue(TEXT("Tint"),Tint);
        DrawMaterial(M,Offset.X+X*Scale,Offset.Y+Y*Scale,W*Scale,H*Scale,Flip?1.f:0.f,0,Flip?-1.f:1.f,1,1,false,Rotation,FVector2D(.5,.5));
    }
}
void ADungeonHUD::DrawMenu(ADungeonGameMode* G)
{
    Sprite(TEXT("TeaTitle"),0,0,1280,800);
    const float Time=GetWorld()->GetTimeSeconds();
    for(int I=0;I<22;++I)
    {
        const float X=FMath::Fmod(I*127.3f+FMath::Sin(Time+I)*14,1280.f);
        const float Y=800-FMath::Fmod(Time*(12+I%5*4)+I*51.f,800.f);
        Box(X,Y,2,3,FLinearColor(1,.48f,.10f,.25f+.3f*FMath::Sin(Time+I)*FMath::Sin(Time+I)));
    }
    float MX=-100,MY=-100; if(auto* PC=GetOwningPlayerController()) PC->GetMousePosition(MX,MY);
    const FVector2D Mouse=(FVector2D(MX,MY)-Offset)/Scale;
    if(bVolumeDragging)
    {
        auto* PC=GetOwningPlayerController();
        if(PC&&PC->IsInputKeyDown(EKeys::LeftMouseButton)) G->SetMasterVolume((Mouse.X-1070)/142.f,false);
        else { bVolumeDragging=false; G->SetMasterVolume(G->GetMasterVolume()); }
    }
    auto AudioButton=[&](float X,const TCHAR* Art,const TCHAR* Tip,bool Muted)
    {
        const bool Hover=Mouse.X>=X&&Mouse.X<X+54&&Mouse.Y>=716&&Mouse.Y<772;
        Sprite(Art,X,716,54,54,Muted?FLinearColor(.42f,.42f,.42f,1):Hover?FLinearColor(1.2f,1.2f,1.2f,1):FLinearColor::White);
        if(Muted) DrawLine(Offset.X+(X+7)*Scale,Offset.Y+763*Scale,Offset.X+(X+47)*Scale,Offset.Y+723*Scale,FLinearColor(.85f,.18f,.1f,1),3*Scale);
        if(Hover) Label(FString(Tip)+(Muted?TEXT(" OFF"):TEXT(" ON")),X-5,694,Gold,.75f);
    };
    AudioButton(900,TEXT("AudioMusic"),TEXT("MUSIC"),G->IsMusicMuted());
    AudioButton(974,TEXT("AudioEffects"),TEXT("SFX"),G->AreEffectsMuted());
    Sprite(TEXT("AudioRail"),1040,722,204,48);
    Sprite(TEXT("AudioThumb"),1058+142*G->GetMasterVolume(),734,24,24);
    if(bVolumeDragging||(Mouse.X>=1040&&Mouse.X<=1244&&Mouse.Y>=722&&Mouse.Y<=770))
        Label(FString::Printf(TEXT("VOLUME %d%%"),FMath::RoundToInt(G->GetMasterVolume()*100)),1080,701,Gold,.75f);
    FString CurrentHover;
    auto Button=[&](const TCHAR* Text,float Y)
    {
        const float Shift=FString(Text)==TEXT("BACK")?75.f:0.f;
        const bool Hover=Mouse.X>=150+Shift&&Mouse.X<=480+Shift&&Mouse.Y>=Y&&Mouse.Y<=Y+48;
        FString Art=FString(Text).Replace(TEXT(" "),TEXT("_"));
        float& Amount=MenuHoverAmounts.FindOrAdd(Art);
        Amount=FMath::FInterpTo(Amount,Hover?1.f:0.f,GetWorld()->GetDeltaSeconds(),14.f);
        const bool Pressed=Hover&&GetOwningPlayerController()->IsInputKeyDown(EKeys::LeftMouseButton);
        const float Lift=Pressed?2.f:-2.f*Amount;
        // Stable hit targets, with a subtle lift, warm highlight and emerald selection gems.
        Sprite(TEXT("Menu_")+Art,142+Shift-5*Amount,Y-8+Lift,346+10*Amount,64,FLinearColor(1+.18f*Amount,1+.14f*Amount,1+.06f*Amount,1));
        if(Amount>.01f)
        {
            const float Pulse=.85f+.15f*FMath::Sin(Time*4);
            const FLinearColor Tint(1,1,1,Amount*Pulse);
            Sprite(TEXT("AudioThumb"),121+Shift-3*Amount,Y+15+Lift,18,20,Tint);
            Sprite(TEXT("AudioThumb"),491+Shift+3*Amount,Y+15+Lift,18,20,Tint);
        }
        if(Hover) CurrentHover=Art;
    };
    if(G->bShowControls)
    {
        // One continuous frame: no independently stretched atlas edges or corner seams.
        Box(100,335,580,438,FLinearColor(.006f,.009f,.006f,.82f));
        auto Outline=[&](float X,float Y,float W,float H,FLinearColor C)
        { Box(X,Y,W,1,C);Box(X,Y+H-1,W,1,C);Box(X,Y,1,H,C);Box(X+W-1,Y,1,H,C); };
        Outline(100,335,580,438,Gold);
        Outline(105,340,570,428,FLinearColor(.36f,.25f,.10f,.85f));
        for(float X:{100.f,679.f})for(float Y:{335.f,772.f})
            Sprite(TEXT("AudioThumb"),X-12,Y-14,24,28);
        Sprite(TEXT("AudioThumb"),137,358,19,23);
        Label(TEXT("HOW TO PLAY"),171,360,Gold,1.25f);
        auto Row=[&](const TCHAR* Key,const TCHAR* Action,float Y)
        {
            // Quiet, high-contrast keycaps: decoration never crosses the lettering.
            Box(140,Y-3,92,25,FLinearColor(.025f,.028f,.022f,1));
            Outline(140,Y-3,92,25,FLinearColor(.55f,.41f,.20f,1));
            float TW=0,TH=0;GetTextSize(Key,TW,TH,GEngine->GetMediumFont(),1.05f);
            Label(Key,186-TW/2,Y+9.5f-TH/2,FLinearColor(1,.90f,.64f,1),1.05f);
            Label(Action,260,Y+1,Pale,1.05f);
        };
        Row(TEXT("W A S D"),TEXT("Move in any direction"),398);
        Row(TEXT("SHIFT"),TEXT("Sprint at twice walking speed"),427);
        Row(TEXT("SPACE"),TEXT("Dodge / roll"),456);
        Row(TEXT("LMB"),TEXT("Strike with your weapon"),485);
        Row(TEXT("RMB"),TEXT("Throw tea  -  10s cooldown"),514);
        Row(TEXT("MMB"),TEXT("FREEDOM  -  charge with 15 kills"),543);
        Row(TEXT("E"),TEXT("Choose one chest / enter a gate"),572);
        Row(TEXT("I"),TEXT("Open inventory & equipment"),601);
        Row(TEXT("P"),TEXT("Pause / resume"),630);
        Label(TEXT("A guardian awaits every FOURTH room."),173,674,Gold,.9f);
        Button(TEXT("BACK"),705);
    }
    else
    {
        Button(G->HasRun()?TEXT("RESUME DESCENT"):TEXT("BEGIN DESCENT"),396);
        Button(TEXT("HOW TO PLAY"),460);
        Button(G->HasRun()?TEXT("NEW RUN"):TEXT("EXIT"),524);
        if(G->HasRun()) Button(TEXT("EXIT"),588);
    }
    if(CurrentHover!=HoveredMenuButton&&!CurrentHover.IsEmpty()) G->PlaySound(TEXT("UI"),.25f,1.15f);
    HoveredMenuButton=CurrentHover;
}
void ADungeonHUD::DrawSwordCursor()
{
    auto* PC=GetOwningPlayerController();float X=0,Y=0;
    if(!PC||!PC->GetMousePosition(X,Y)||X<0||Y<0||X>=Canvas->SizeX||Y>=Canvas->SizeY) return;
    const FVector2D SavedOffset=Offset;const float SavedScale=Scale;
    // Screen-space cursor ignores camera shake. The sword tip is the exact mouse hotspot.
    Offset=FVector2D::ZeroVector;Scale=FMath::Clamp(SavedScale,.8f,1.5f);
    const float W=52,TipX=64.f/128,TipY=5.f/128;
    Sprite(TEXT("Loot_0"),X/Scale-W*TipX,Y/Scale-W*TipY,W,W,FLinearColor::White,-35,FVector2D(TipX,TipY));
    Offset=SavedOffset;Scale=SavedScale;
}
void ADungeonHUD::InventoryClick()
{
    auto* H=Player(this); auto* PC=GetOwningPlayerController();
    float X=0,Y=0; if(!H||!PC||!PC->GetMousePosition(X,Y)||Scale<=0) return;
    const FVector2D P=(FVector2D(X,Y)-Offset)/Scale;
    auto In=[&](float L,float T,float W,float Height){return P.X>=L&&P.X<L+W&&P.Y>=T&&P.Y<T+Height;};
    if(auto* G=Mode(this)) if(G->IsMenu())
    {
        if(In(900,716,54,56)) { G->ToggleMusic(); return; }
        if(In(974,716,54,56)) { G->ToggleEffects(); return; }
        if(In(1040,722,204,48)) { bVolumeDragging=true; G->SetMasterVolume((P.X-1070)/142.f); return; }
        if(G->bShowControls) { if(In(225,705,330,48)) G->bShowControls=false; return; }
        if(In(150,396,330,48)) { if(G->HasRun()) G->ToggleMenu(); else G->StartGame(); }
        if(In(150,460,330,48)) G->bShowControls=true;
        if(In(150,524,330,48)) { if(G->HasRun()) G->StartGame(); else UKismetSystemLibrary::QuitGame(this,PC,EQuitPreference::Quit,false); }
        if(G->HasRun()&&In(150,588,330,48)) UKismetSystemLibrary::QuitGame(this,PC,EQuitPreference::Quit,false);
        return;
    }
    if(!H->IsInventoryOpen()) return;
    using namespace DungeonInventoryLayout;
    if(In(1140,63,105,40)) {H->ToggleInventory();return;}
    const int GearSlot=GearAt(P);
    if(GearSlot!=INDEX_NONE&&!H->Equipment[GearSlot].IsEmpty()) {
        const double Now=FPlatformTime::Seconds();
        if(LastClickedGear==GearSlot&&Now-LastClickTime<=.35&&FVector2D::Distance(P,LastClickPosition)<8)
        {H->Unequip(GearSlot);CancelInventoryGesture();return;}
        LastClickedItem=INDEX_NONE;LastClickedGear=GearSlot;LastClickTime=Now;LastClickPosition=P;
        DragItem=INDEX_NONE;DragGear=GearSlot;bDragging=false;DragStart=DragMouse=P;
        const auto Size=H->Equipment[GearSlot].Size();const auto Local=P-Gear(GearSlot);
        DragGrab=FVector2D(FMath::Min(Local.X,Size.X*60.-1),FMath::Min(Local.Y,Size.Y*60.-1));return;
    }
    if(In(BagX,BagY,360,360)) {
        const FIntPoint C=BagCell(P);H->SelectedItem=INDEX_NONE;
        for(int I=0;I<H->Inventory.Num();++I) {
            const auto& E=H->Inventory[I];auto S=E.Item.Size();
            if(C.X>=E.Cell.X&&C.X<E.Cell.X+S.X&&C.Y>=E.Cell.Y&&C.Y<E.Cell.Y+S.Y) {
                H->SelectedItem=I;const double Now=FPlatformTime::Seconds();
                if(LastClickedItem==I&&Now-LastClickTime<=.35&&FVector2D::Distance(P,LastClickPosition)<8)
                {H->EquipFromInventory(I);CancelInventoryGesture();return;}
                LastClickedGear=INDEX_NONE;LastClickedItem=I;LastClickTime=Now;LastClickPosition=P;
                DragGear=INDEX_NONE;DragItem=I;bDragging=false;DragStart=DragMouse=P;
                DragGrab=P-FVector2D(BagX+E.Cell.X*CellSize,BagY+E.Cell.Y*CellSize);return;
            }
        }
    }
    CancelInventoryGesture();
    if(In(456,564,174,44))H->EquipFromInventory(H->SelectedItem);
    if(In(642,564,174,44)&&H->Inventory.IsValidIndex(H->SelectedItem)){
        H->DiscardForCoins(H->SelectedItem);
    }
}
void ADungeonHUD::ItemLettering(const FString& Text,float X,float Y,FLinearColor Color,float Height,float MaxWidth)
{
    auto* T=Texture(TEXT("InventoryGlyphs"));if(!T)return;
    const FString Upper=Text.ToUpper();float CX=X,CY=Y;
    float Longest=0,WordUnits=0;
    for(TCHAR Ch:Upper){if(Ch==' '||Ch=='\n'){Longest=FMath::Max(Longest,WordUnits);WordUnits=0;}else WordUnits+=InventoryGlyphMetrics::Widths[FMath::Clamp(int(Ch),32,127)-32]/28.f;}
    Longest=FMath::Max(Longest,WordUnits);
    if(Longest>0)Height=FMath::Min(Height,MaxWidth*.95f/Longest);
    auto Width=[&](TCHAR Ch){return InventoryGlyphMetrics::Widths[FMath::Clamp(int(Ch),32,127)-32]*Height/28.f;};
    for(int I=0;I<Upper.Len();++I) {
        const TCHAR Ch=Upper[I];
        if(Ch=='\n'){CX=X;CY+=Height;continue;}
        if(Ch!=' '&&(I==0||Upper[I-1]==' ')) {
            float Word=0;for(int J=I;J<Upper.Len()&&Upper[J]!=' '&&Upper[J]!='\n';++J)Word+=Width(Upper[J]);
            if(CX>X&&CX+Word>X+MaxWidth){CX=X;CY+=Height;}
        }
        const float Advance=Width(Ch);
        if(CX+Advance>X+MaxWidth){CX=X;CY+=Height;}
        if(Ch==' '&&CX==X)continue;
        int C=FMath::Clamp(int(Ch),32,127)-32;
        for(int Pass=0;Pass<2;++Pass) {
            const float D=Pass==0?1:0;
            DrawTexture(T,Offset.X+(CX+(Advance-Height*32/28)/2+D)*Scale,Offset.Y+(CY+D)*Scale,Height*32/28*Scale,Height*Scale,
                (C%16)/16.f,((C/16)*40.f+4)/256.f,1.f/16,28.f/256,Pass==0?FLinearColor(0,0,0,.9f):Color,BLEND_Translucent);
        }
        CX+=Advance;
    }
}
float ADungeonHUD::CardText(const FString& Text,float X,float Y,FLinearColor Color,float Height,float MaxWidth,float MaxHeight)
{
    if(!Canvas||Scale<=0||Text.IsEmpty()||MaxHeight<=0)return 0;
    // Rasterize the engine's runtime font at the actual viewport size, not a scaled pixel atlas.
    if(!CardTypeface)CardTypeface=LoadObject<UFont>(nullptr,TEXT("/Engine/EngineFonts/Roboto.Roboto"));
    if(!CardTypeface)return 0;
    const FSlateFontInfo Font(CardTypeface,FMath::Max(8,FMath::RoundToInt(Height*.60f*Scale)),Height>=22?FName("Bold"):FName("Regular"));
    const auto Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
    auto Width=[&](const FString& S){return Measure->Measure(S,Font).X;};
    const float Limit=MaxWidth*Scale;
    const float LineHeight=FMath::Max(1.f,float(Measure->Measure(TEXT("Ag"),Font).Y)/Scale);
    const int MaxLines=FMath::Max(1,FMath::FloorToInt(MaxHeight/LineHeight));
    TArray<FString> Words,Lines;Text.ParseIntoArrayWS(Words);FString Line;
    for(const auto& Word:Words){
        FString Candidate=Line+Word;
        if(!Line.IsEmpty()&&Width(Candidate)>Limit){Lines.Add(Line);Line.Empty();}
        // Also constrain unusually long single tokens.
        for(TCHAR Ch:Word) {
            FString Next=Line+FString::Chr(Ch);
            if(Width(Next)>Limit&&!Line.IsEmpty()){Lines.Add(Line);Line.Empty();}
            Line+=Ch;
        }
        Line+=TEXT(" ");
    }
    Line.TrimEndInline();if(!Line.IsEmpty())Lines.Add(Line);
    if(Lines.Num()>MaxLines){
        Lines.SetNum(MaxLines);
        auto& Last=Lines.Last();Last.TrimEndInline();
        while(!Last.IsEmpty()&&Width(Last+TEXT("..."))>Limit)Last.LeftChopInline(1);
        Last+=TEXT("...");
    }
    for(int I=0;I<Lines.Num();++I){
        Lines[I].TrimEndInline();
        FCanvasTextItem Item(Offset+FVector2D(X,Y+I*LineHeight)*Scale,FText::FromString(Lines[I]),Font,Color);
        Item.EnableShadow(FLinearColor(0,0,0,.65f),FVector2D(0,1));
        Canvas->DrawItem(Item);
    }
    return Lines.Num()*LineHeight;
}
void ADungeonHUD::DrawItemCard(const FDungeonItem& E,const ADungeonHero* CompareHero)
{
    const TCHAR* Types[]={TEXT("WEAPON"),TEXT("ARMOR"),TEXT("AMULET"),TEXT("RING")};
    Sprite(TEXT("InventoryCard"),842,104,400,628);
        const auto R=RarityColor(E.Rarity);
        CardText(E.Name,892,151,R,23,304);
        CardText(FString::Printf(TEXT("%s / LV %d"),RarityNames[E.Rarity],E.ItemLevel),892,198,R,18,304);
        Sprite(E.Art(),888,230,96,96);
        CardText(Types[E.Slot],996,231,Gold,20,196);
        if(E.Slot==0) {
            CardText(FString::Printf(TEXT("%.1f DPS"),(24+E.Attack)*(1+E.Speed)/.48f),996,252,Pale,28,196);
            CardText(FString::Printf(TEXT("%.0f-%.0f DMG"),(24+E.Attack)*.9f,(24+E.Attack)*1.1f),996,286,Pale,18,196);
            CardText(FString::Printf(TEXT("%.2f ATK/SEC"),(1+E.Speed)/.48f),996,307,Pale,18,196);
        } else if(E.Slot==3) {
            const float V=E.BleedChance+E.PoisonChance+E.StaminaBonus+E.Leech+E.Reduction+E.HealthBonus;
            const TCHAR* Kind=E.BleedChance>0?TEXT("BLEED CHANCE"):E.PoisonChance>0?TEXT("POISON CHANCE"):E.StaminaBonus>0?TEXT("MAX STAMINA"):E.Leech>0?TEXT("DAMAGE LEECH"):E.Reduction>0?TEXT("MITIGATION"):TEXT("MAX HEALTH");
            CardText(FString::Printf(TEXT("+%.1f%%"),V*100),996,258,Pale,30,196);
            CardText(Kind,996,296,Gold,20,196);
        } else {
            CardText(E.Slot==1?FString::Printf(TEXT("+%.0f ARMOR"),E.Defense):FString::Printf(TEXT("+%.0f HEALTH"),E.Vitality),996,258,Pale,22,196);
        }
        Box(884,337,322,1,R);
        CardText(TEXT("BONUSES"),892,344,Gold,21,304);
        float Y=375;
        auto Line=[&](const FString& T,FLinearColor C){Y+=FMath::Max(24.f,CardText(T,892,Y,C,21,304,FMath::Max(0.f,564-Y))+4);};
        const FLinearColor Purple(.78f,.58f,1),Green(.4f,1,.58f);
        auto Bonus=[&](float V,const TCHAR* Name,bool Percent,FLinearColor C){if(!FMath::IsNearlyZero(V))Line((Percent?FString::Printf(TEXT("%+.1f%% %s"),V*100,Name):FString::Printf(TEXT("%+.0f %s"),V,Name)),C);};
        Bonus(E.Attack,TEXT("WEAPON DAMAGE"),false,Purple);
        if(E.Slot!=1)Bonus(E.Defense,TEXT("ARMOR"),false,Purple);
        if(E.Slot!=2)Bonus(E.Vitality,TEXT("HEALTH"),false,Purple);
        Bonus(E.CritChance,TEXT("CRITICAL CHANCE"),true,Purple);Bonus(E.CritDamage,TEXT("CRITICAL DAMAGE"),true,Purple);
        Bonus(E.Speed,TEXT("ATTACK SPEED"),true,Purple);Bonus(E.Regen,TEXT("STAMINA REGEN"),true,Green);
        if(Y==375)Line(TEXT("No bonus affixes"),Pale);
        if(E.Effect) {Y+=CardText(E.EffectText(),892,Y,Green,20,304,FMath::Max(0.f,564-Y));}
        if(E.Slot==3) {
            const TCHAR* Hint=E.BleedChance>0?TEXT("Hits may cause bleed for 4s."):E.PoisonChance>0?TEXT("Hits may poison for 6s."):E.Leech>0?TEXT("Heal from actual damage dealt."):E.Reduction>0?TEXT("Reduces incoming attack damage."):TEXT("Raises capacity, not a refill.");
            CardText(Hint,892,520,Green,20,304);
        }
        Box(884,578,322,1,R);
        if(CompareHero) {
        const auto* H=CompareHero;const auto& Current=H->Equipment[E.Slot];
        CardText(TEXT("EQUIPPED: ")+(Current.IsEmpty()?TEXT("NONE"):Current.Name),892,586,Gold,16,304);
        const float Delta=E.Slot==0?(24+E.Attack)*(1+E.Speed)/.48f-(24+Current.Attack)*(1+Current.Speed)/.48f:E.Defense-Current.Defense;
        float FlatHP=150,BonusHP=0;
        for(int I=0;I<H->Equipment.Num();++I)if(I!=E.Slot&&!H->Equipment[I].IsEmpty()){FlatHP+=H->Equipment[I].Vitality;BonusHP+=H->Equipment[I].HealthBonus;}
        const float HealthDelta=(FlatHP+E.Vitality)*(1+FMath::Clamp(BonusHP+E.HealthBonus,0.f,1.f))-H->MaxHealth;
        CardText(FString::Printf(TEXT("%+.1f %s"),Delta,E.Slot==0?TEXT("DPS"):TEXT("ARMOR")),892,626,Delta>=0?FLinearColor(.4f,1,.58f):FLinearColor(1,.4f,.3f),17,155);
        CardText(FString::Printf(TEXT("%+.0f MAX HP"),HealthDelta),1050,626,HealthDelta>=0?FLinearColor(.4f,1,.58f):FLinearColor(1,.4f,.3f),17,155);
        }
        CardText(FString::Printf(TEXT("VALUE  %d GOLD"),E.CoinValue),892,652,Gold,22,304);
}
void ADungeonHUD::DrawAdventurerStats(const ADungeonHero* H)
{
    Sprite(TEXT("InventoryCard"),842,104,400,628);
    CardText(TEXT("ADVENTURER STATS"),892,149,Gold,23,304);
    CardText(TEXT("Base stats + all equipped gear"),892,180,Pale,17,304);
    Box(884,207,322,1,Gold);
    float Y=217;
    const FLinearColor Green(.4f,1,.58f),Purple(.78f,.58f,1);
    auto Row=[&](const TCHAR* Label,const FString& Value,FLinearColor Color) {
        CardText(Label,892,Y,Pale,17,166,22);
        CardText(Value,1062,Y,Color,17,134,22);Y+=23;
    };
    Row(TEXT("Health"),FString::Printf(TEXT("%.1f / %.1f"),H->Health,H->MaxHealth),Green);
    Row(TEXT("Stamina"),FString::Printf(TEXT("%.1f / %.1f"),H->Stamina,H->MaxStamina),Green);
    Row(TEXT("Stamina recovery"),FString::Printf(TEXT("%.1f / sec"),25*H->StaminaRegen),Green);
    Row(TEXT("Hit damage"),FString::Printf(TEXT("%.1f - %.1f"),H->AttackPower*.9f,H->AttackPower*1.1f),Pale);
    Row(TEXT("Attacks / sec"),FString::Printf(TEXT("%.2f"),H->AttackSpeed/.48f),Pale);
    Row(TEXT("DPS (before crits)"),FString::Printf(TEXT("%.1f"),H->AttackPower*H->AttackSpeed/.48f),Pale);
    Row(TEXT("Critical chance"),FString::Printf(TEXT("%.1f%%"),100*H->CritChance),Purple);
    Row(TEXT("Critical damage"),FString::Printf(TEXT("%.0f%% of hit"),100*H->CritMultiplier),Purple);
    Row(TEXT("Armor"),FString::Printf(TEXT("%.1f"),H->Armor),Green);
    Row(TEXT("Damage reduction"),FString::Printf(TEXT("%.1f%%"),100*H->DamageReduction),Green);
    Row(TEXT("Bleed chance"),FString::Printf(TEXT("%.1f%%"),100*H->BleedChance),Purple);
    Row(TEXT("Poison chance"),FString::Printf(TEXT("%.1f%%"),100*H->PoisonChance),Purple);
    Row(TEXT("All-damage leech"),FString::Printf(TEXT("%.1f%%"),100*H->Leech),Green);
    Box(884,Y+2,322,1,Gold);Y+=12;
    CardText(TEXT("GEAR EFFECTS"),892,Y,Gold,18,304,22);Y+=24;
    // Signatures are presence-based in combat, not additive per equipped copy.
    // Bleed/poison chances above already include their weapon signatures.
    bool Any=false;
    for(int Effect=1;Effect<=10;++Effect)if(H->HasEffect(Effect)||(Effect==1&&H->BleedChance>0)||(Effect==2&&H->PoisonChance>0)) {
        FDungeonItem Description;Description.Effect=Effect;
        const FString Detail=Effect==1?TEXT("Bleed: 20% hit damage/sec for 4s"):Effect==2?TEXT("Poison: 14% hit damage/sec for 6s"):Description.EffectText();
        Y+=CardText(Detail,892,Y,Green,16,304,680-Y)+4;Any=true;
    }
    if(!Any)CardText(TEXT("No additional gear effects"),892,Y,Pale,16,304,40);
}
void ADungeonHUD::DrawInventory(ADungeonHero* H)
{
    using namespace DungeonInventoryLayout;
    UpdateInventoryDrag(H);
    Box(0,0,1280,800,FLinearColor(0,0,0,.85f));
    Sprite(TEXT("InventoryFrame"),10,40,1260,720);
    Sprite(TEXT("InventoryCard"),24,102,400,594);
    Sprite(TEXT("InventoryAdventurer"),84,155,280,420);
    Sprite(TEXT("InventorySatchel"),593,91,86,80);
    CardText(FString::Printf(TEXT("COINS  %lld"),H->Coins),456,635,Gold,23,360);
    Sprite(TEXT("InventoryFrame"),1140,63,105,40);ItemLettering(TEXT("CLOSE"),1160,70,Pale,20);
    const TCHAR* Future[]={TEXT("InventoryHelmet"),TEXT("InventoryGloves"),TEXT("InventoryPants"),TEXT("InventoryBoots")};
    for(int I=0;I<4;++I) {
        float X=I%2==0?40:320,Y=I<2?166:282;
        Sprite(TEXT("InventoryFrame"),X,Y,80,80,FLinearColor(.45f,.45f,.42f));
        Sprite(Future[I],X+9,Y+9,62,62,FLinearColor(.6f,.6f,.6f,.85f));
    }
    const TCHAR* SlotArt[]={TEXT("InventorySlotWeapon"),TEXT("InventorySlotArmor"),TEXT("InventorySlotAmulet"),TEXT("InventorySlotRing")};
    const FDungeonItem* Hover=nullptr;
    float MX=0,MY=0;FVector2D Mouse(-1,-1);
    if(auto* PC=GetOwningPlayerController()) if(Scale>0&&PC->GetMousePosition(MX,MY))Mouse=(FVector2D(MX,MY)-Offset)/Scale;
    for(int S=0;S<4;++S) {
        auto P=Gear(S);const auto& E=H->Equipment[S];
        Sprite(TEXT("InventoryFrame"),P.X,P.Y,80,80);
        const bool ShowGuide=E.IsEmpty()||(bDragging&&DragGear==S);
        Sprite(SlotArt[S],P.X+9,P.Y+9,62,62,ShowGuide?FLinearColor(.6f,.6f,.6f,.85f):FLinearColor(.25f,.25f,.25f,.22f));
        if(!E.IsEmpty()){
            if(!bDragging||DragGear!=S)Sprite(E.Art(),P.X+5,P.Y+5,70,70);
            Box(P.X+8,P.Y+73,64,2,RarityColor(E.Rarity));
            if(In(Mouse,P.X,P.Y,80,80))Hover=&E;
        }
    }
    for(int Y=0;Y<6;++Y)for(int X=0;X<6;++X)
        Sprite(TEXT("InventoryFrame"),BagX+X*CellSize,BagY+Y*CellSize,58,58,FLinearColor(.55f,.55f,.48f));
    for(int I=0;I<H->Inventory.Num();++I) {
        if(bDragging&&I==DragItem)continue;
        const auto& B=H->Inventory[I];auto S=B.Item.Size();
        float X=BagX+B.Cell.X*CellSize,Y=BagY+B.Cell.Y*CellSize,W=S.X*CellSize-2,HH=S.Y*CellSize-2;
        Box(X,Y,W,HH,I==H->SelectedItem?Gold:RarityColor(B.Item.Rarity));
        Box(X+2,Y+2,W-4,HH-4,FLinearColor(.008f,.012f,.012f));
        const float Icon=FMath::Min(W-6,HH-6);
        if(B.Item.Slot==0&&B.Item.CatalogId>=0)Sprite(B.Item.BagArt(),X+3,Y+3,W-6,HH-6);
        else Sprite(B.Item.Art(),X+(W-Icon)/2,Y+(HH-Icon)/2,Icon,Icon);
        if(In(Mouse,X,Y,W,HH))Hover=&B.Item;
    }
    // Central portrait hit area excludes the equipment slots on either side.
    const bool HoverAdventurer=!bDragging&&!Hover&&In(Mouse,122,155,196,420);
    if(HoverAdventurer)DrawAdventurerStats(H);
    else if(Hover||H->Inventory.IsValidIndex(H->SelectedItem)) DrawItemCard(Hover?*Hover:H->Inventory[H->SelectedItem].Item,H);
    else Sprite(TEXT("InventoryCard"),842,104,400,628);
    if(H->Inventory.IsValidIndex(H->SelectedItem)) {
        Sprite(TEXT("InventoryFrame"),456,564,174,44);ItemLettering(TEXT("EQUIP"),501,574,Pale,22,150);
        Sprite(TEXT("InventoryFrame"),642,564,174,44);ItemLettering(TEXT("DISCARD"),675,574,Pale,22,150);
    }
    DrawInventoryDrag(H);
}
void ADungeonHUD::DrawHUD()
{
    Super::DrawHUD();
    UpdateFlashScreen();
    auto* G=Mode(this); auto* H=Player(this); if(!G||!H||!Canvas) return;
    ON_SCOPE_EXIT { DrawSwordCursor(); };
    if(Textures.IsEmpty())
    {
        LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/V2/M_KeySprite.M_KeySprite"));
        for(int I=0;I<16;++I) Texture(FString::Printf(TEXT("TeaFX_%d"),I));
        for(int I=0;I<8;++I) Texture(FString::Printf(TEXT("Finance_%d"),I));
        for(int S=31;S<=48;++S)for(int F=0;F<8;++F)Texture(FString::Printf(TEXT("ThemeEnemy_%d_%d"),S,F));
        for(int F=0;F<16;++F)Texture(FString::Printf(TEXT("FlashGuy_%d"),F));
        for(int S=0;S<28;++S) for(int F=0;F<8;++F) Texture(FString::Printf(TEXT("Creature_%d_%d"),S,F));
        for(int D=0;D<8;++D)Texture(FString::Printf(TEXT("Athletic_Idle_%d"),D));
        for(int D=0;D<4;++D) for(int F=0;F<8;++F)
            Texture(FString::Printf(TEXT("Athletic_Roll_%d_%d"),D,F));
        for(const auto& State:{TEXT("Walk"),TEXT("Attack")}) for(const auto& Group:{TEXT("Cardinal"),TEXT("Diagonal")})
            for(int R=0;R<4;++R) for(int F=0;F<6;++F) Texture(FString::Printf(TEXT("Athletic_%s%s_%d_%d"),State,Group,R,F));
        for(int R=0;R<4;++R) for(int F=0;F<4;++F)
        { Texture(FString::Printf(TEXT("EnemyWalk_%d_%d"),R,F)); Texture(FString::Printf(TEXT("EnemyAttack_%d_%d"),R,F)); }
        for(int R=0;R<2;++R) for(int F=0;F<4;++F) Texture(FString::Printf(TEXT("BossMotion_%d_%d"),R,F));
    }
    Scale=FMath::Min(Canvas->SizeX/1280.f,Canvas->SizeY/800.f);
    Offset=FVector2D((Canvas->SizeX-1280*Scale)/2,(Canvas->SizeY-800*Scale)/2);
    DrawRect(FLinearColor(.005f,.008f,.012f),0,0,Canvas->SizeX,Canvas->SizeY);
    if(G->HasEnding()) { DrawEnding(G);return; }
    if(G->IsMenu()) { DrawMenu(G); return; }
    if(G->IsTraderOpen()) {if(H->IsInventoryOpen())DrawInventory(H);else DrawTrader(G,H);return;}
    const FVector2D StableOffset=Offset;
    if(G->IsFreedomActive())
    {
        const float T=GetWorld()->GetTimeSeconds();
        Offset+=FVector2D(FMath::Sin(T*71)*5,FMath::Cos(T*57)*4)*Scale;
    }
    Sprite(G->GetBiome()==0?TEXT("Arena"):FString::Printf(TEXT("Arena%d"),G->GetBiome()),0,0,1280,800);
    if(G->IsBossIntroActive()){DrawBossIntro(G);return;}
#if !UE_BUILD_SHIPPING
    if(FParse::Param(FCommandLine::Get(),TEXT("DungeonHeroReviewPreview")))
    {
        int Group=0;FParse::Value(FCommandLine::Get(),TEXT("DungeonBiome="),Group);
        Box(0,0,1280,800,FLinearColor(.035f,.045f,.05f,1));
        const auto Position=H->GetActorLocation();H->HurtTime=0;
        Label(Group>=2?TEXT("ATHLETIC HERO / WALK / HAND ANCHORS"):TEXT("ATHLETIC HERO / ATTACK / HAND ANCHORS"),30,15,Gold);
        for(int R=0;R<4;++R)for(int F=0;F<6;++F)
        {
            const int D=(Group%2)*4+R;
            H->SetActorLocation(DungeonView::Unproject(FVector2D(130+F*207,195+R*193)));
            if(Group>=2)H->SetWalkReviewPose(D,F);else H->SetReviewPose(D,F);
            Hero(H,1.25f);Label(FString::Printf(TEXT("D%d / F%d"),D,F),75+F*207,198+R*193,Pale,.75f);
        }
        H->SetActorLocation(Position);return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("DungeonHealthBarsPreview"))&&!G->GetEnemies().IsEmpty())
    {
        auto* E=G->GetEnemies()[0].Get();
        const int SavedSpecies=E->Species;
        const float SavedHealth=E->Health,SavedMax=E->MaxHealth,SavedLag=E->HealthLag;
        const auto SavedOffset=Offset;
        Box(0,0,1280,800,FLinearColor(.035f,.045f,.05f));
        Label(TEXT("FULL HEALTH"),30,24,Gold);Label(TEXT("35% HEALTH / DAMAGE TRAIL"),650,24,Gold);
        for(int Row=0;Row<6;++Row)for(int Column=0;Column<2;++Column)
        {
            E->Species=24+Row;E->MaxHealth=100;E->Health=Column?35:100;E->HealthLag=Column?55:100;
            Offset=SavedOffset+FVector2D((Column?640:20)-322,55+Row*115-12)*Scale;
            DrawBossUI(E);
        }
        E->Species=SavedSpecies;E->Health=SavedHealth;E->MaxHealth=SavedMax;E->HealthLag=SavedLag;Offset=SavedOffset;
        return;
    }
#endif
    DrawCombatFX(G,false);
#if !UE_BUILD_SHIPPING
    if(FParse::Param(FCommandLine::Get(),TEXT("DungeonWeaponsPreview")))
    {
        Box(0,0,1280,800,Ink);
        Label(TEXT("EQUIPPED WEAPON SCALE REVIEW / 24 WEAPONS"),25,16,Gold);
        const auto Position=H->GetActorLocation();const auto Gear=H->Equipment;
        for(int I=0;I<24;++I) {
            H->Equipment[0]=ADungeonGameMode::RollItem(I,0);
            H->SetActorLocation(DungeonView::Unproject(FVector2D(100+(I%6)*210,180+(I/6)*185)));
            H->SetReviewPose(FParse::Param(FCommandLine::Get(),TEXT("ReviewLeft"))?6:2,2);
            Hero(H,.85f);Label(H->Equipment[0].Name,25+(I%6)*210,195+(I/6)*185,Pale,.7f);
        }
        H->Equipment=Gear;H->SetActorLocation(Position);return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("DungeonIdlePreview")))
    {
        Box(0,0,1280,800,Ink);Label(TEXT("IDLE BREATHING / EXHALE AND INHALE / EIGHT DIRECTIONS"),25,15,Gold);
        const auto Position=H->GetActorLocation();
        for(int D=0;D<8;++D) for(int Phase=0;Phase<2;++Phase)
        {
            const int Cell=D*2+Phase;const float X=150+(Cell%4)*320,Y=188+(Cell/4)*185;
            H->SetActorLocation(DungeonView::Unproject(FVector2D(X,Y)));H->SetIdleReviewPose(D,Phase*PI);
            Hero(H,1.2f);Label(FString::Printf(TEXT("Direction %d / %s"),D,Phase?TEXT("inhale"):TEXT("exhale")),X-70,Y+12,Pale,.8f);
        }
        H->SetActorLocation(Position);return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("DungeonRosterPreview")))
    {
        Box(0,0,1280,800,Ink); Label(TEXT("BESTIARY / 24 ENEMIES + 4 BOSSES"),30,18,Gold);
        for(int I=0;I<28;++I)
        {
            float X=30+(I%7)*178,Y=66+(I/7)*181;
            KeySprite(I==24?FString::Printf(TEXT("Finance_%d"),(int)(GetWorld()->GetTimeSeconds()*8)%8):FString::Printf(TEXT("Creature_%d_%d"),I,(int)(GetWorld()->GetTimeSeconds()*8)%8),X,Y,146,146,FLinearColor::White);
            Label(DungeonRoster::Get(I).Name,X,Y+151,Pale,.75f);
        }
        return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("DungeonRollPreview")))
    {
        Box(0,0,1280,800,Ink); Label(TEXT("DODGE / EIGHT POSES / FOUR DIRECTIONS"),30,18,Gold);
        for(int D=0;D<4;++D) for(int F=0;F<8;++F)
            Sprite(FString::Printf(TEXT("Athletic_Roll_%d_%d"),D,F),16+F*156,58+D*181,146,146,FLinearColor::White);
        return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("DungeonFXPreview")))
    {
        Label(TEXT("TEA / STOCKS / BONDS / RANGED EFFECTS"),30,20,Gold);
        for(int I=0;I<16;++I)
            KeySprite(FString::Printf(TEXT("TeaFX_%d"),I),90+(I%4)*285,55+(I/4)*180,155,155,FLinearColor::White);
        return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("DungeonEquipmentReview")))
    {
        const auto Position=H->GetActorLocation(); const auto Equipped=H->Equipment;
        H->HurtTime=0;
        const bool Left=FParse::Param(FCommandLine::Get(),TEXT("ReviewLeft"));
        Box(0,0,1280,800,Ink);
        Label(Left?TEXT("LEFT ATTACK / SIX POSES / FOUR OUTFITS"):TEXT("RIGHT ATTACK / SIX POSES / FOUR OUTFITS"),30,20,Gold);
        for(int R=0;R<4;++R) for(int F=0;F<6;++F)
        {
            if(R==0) { H->Equipment[1]=FDungeonItem(); }
            else H->Equipment[1]=ADungeonGameMode::MakeItem(R+2,2);
            H->SetActorLocation(DungeonView::Unproject(FVector2D(140+F*200,197+R*185)));
            H->SetReviewPose(Left?6:2,F); Hero(H,1.1f);
        }
        H->SetActorLocation(Position); H->Equipment=Equipped;
        return;
    }
#endif
    for(int I=0;I<3;++I)
    {
        FVector2D P=ADungeonGameMode::DoorPosition(I);
        // Align light to the measured bunker openings; interaction reach is unchanged.
        if(G->GetBiome()==5) { const float Shift[]={17,0,10};P.X+=Shift[I]; }
        if(G->GetBiome()==6) { const float Shift[]={-17,0,18};P.X+=Shift[I]; }
        if(G->AreDoorsOpen())
        {
            const float Pulse=(G->AreDoorsOpen()?.32f:.13f)+.06f*FMath::Sin(GetWorld()->GetTimeSeconds()*3);
            // Illuminate the existing arch and stair pixels, retaining each biome's masonry.
            const auto Arena=G->GetBiome()==0?FString(TEXT("Arena")):FString::Printf(TEXT("Arena%d"),G->GetBiome());
            const int Top=G->GetBiome()==6?40:12,Bottom=G->GetBiome()==6?176:142;
            if(auto* T=Texture(Arena)) for(int Y=Top;Y<Bottom;Y+=2)
            {
                const float Arch=Top+45.f;
                const float Half=Y<Arch?FMath::Sqrt(FMath::Max(0.f,1.f-FMath::Square((Y-Arch)/45.f)))*58.f:58.f;
                const float X=P.X-Half;
                DrawTexture(T,Offset.X+X*Scale,Offset.Y+Y*Scale,Half*2*Scale,2*Scale,X/1280.f,Y/800.f,Half*2/1280.f,2/800.f,FLinearColor(.15f,2.4f,.85f,Pulse*2),BLEND_Additive);
                if(Y>23&&Y<123) Box(P.X-Half*.77f,Y,Half*1.54f,2,FLinearColor(.1f,1,.5f,Pulse*.5f));
            }
        }
    }
    for(auto& E:G->GetEnemies()) if(IsValid(E)&&E->Windup>0&&(E->Species<28||E->Species>=31))
    {
        const auto& Spec=DungeonRoster::Get(E->Species);
        const float Radius=E->bBoss?E->AttackRadius:Spec.AttackStyle==4?Spec.Range:Spec.AttackStyle==0?Spec.Range:35;
        Ring(E->AttackTarget,Radius,FLinearColor(1,.15f,.08f,.9f),3);
        Ring(E->AttackTarget,Radius*(1.f-E->Windup/(E->bBoss?E->AttackWindup:Spec.Windup)),FLinearColor(1,.5f,.12f,.65f),2);
        if(Spec.AttackStyle==2)
        {
            const auto From=DungeonView::Project(E->GetActorLocation());
            DrawLine(Offset.X+From.X*Scale,Offset.Y+From.Y*Scale,Offset.X+E->AttackTarget.X*Scale,Offset.Y+E->AttackTarget.Y*Scale,FLinearColor(1,.25f,.1f),3*Scale);
        }
    }
    DrawBreakables(G,H);
    DrawReward(G,H);
    for(const auto& FX:G->GetSplashes())
    {
        if(FX.Art>=40)
        {
            const float Size=FX.Radius*2*(.6f+.4f*FMath::Clamp((.65f-FX.Life)/.16f,0.f,1.f));
            Sprite(FString::Printf(TEXT("ThemeFX_%d"),FX.Art-40),FX.Position.X-Size/2,FX.Position.Y-Size*.35f,Size,Size*.7f,FLinearColor(1,1,1,FMath::Clamp(FX.Life/.3f,0.f,1.f)));continue;
        }
        if(FX.Art==20||FX.Art==21)
        {
            const float Age=.65f-FX.Life,Size=FX.Art==21?70+Age*80:175+Age*350;
            Sprite(TEXT("FlashBurst"),FX.Position.X-Size/2,FX.Position.Y-Size/2,Size,Size,FLinearColor(FX.Art==21?.3f:1,FX.Art==21?.65f:1,1,FMath::Clamp(FX.Life/.4f,0.f,1.f)));
            continue;
        }
        if(FX.Art>=16)
        {
            const float Age=.65f-FX.Life;
            if(FX.Art==16||FX.Art==17)
            {
                const float Size=FX.Art==16?140:210;
                Sprite(FString::Printf(TEXT("%s_%d"),FX.Art==16?TEXT("BurgerSplat"):TEXT("MackLanding"),FMath::Clamp(int(Age/.65f*12),0,11)),FX.Position.X-Size/2,FX.Position.Y-Size/2,Size,Size);
                if(FX.Art==16) for(int J=0;J<6;++J)
                {
                    const float A=J*PI/3; auto P=FX.Position+FVector2D(FMath::Cos(A)*Age*130,FMath::Sin(A)*Age*65-Age*100+Age*Age*180);
                    Sprite(FString::Printf(TEXT("FoodDebris_%d"),(J%4)*6+int(Age*12)%6),P.X-15,P.Y-15,30,30,FLinearColor(1,1,1,FX.Life/.65f));
                }
            }
            else if(FX.Art==18&&Age<.08f) Sprite(TEXT("LootEffect_2"),FX.Position.X-20,FX.Position.Y-20,40,40);
            continue;
        }
        const float Age=.65f-FX.Life,Size=FX.Radius*2*(.7f+.3f*FMath::Clamp(Age/.13f,0.f,1.f));
        KeySprite(FString::Printf(TEXT("TeaFX_%d"),FX.Art),FX.Position.X-Size*.5f,FX.Position.Y-Size*.325f,Size,Size*.65f,FLinearColor(1,1,1,FMath::Min(1.f,FX.Life/.25f)));
    }
    DrawPotions(G);
    // Draw actor sprites once, sorted by foot depth.
    TArray<AActor*> Actors; Actors.Add(H);
    for(auto& E:G->GetEnemies()) if(IsValid(E)) Actors.Add(E);
    Actors.Sort([](const AActor& A,const AActor& B){return DungeonView::Project(A.GetActorLocation()).Y<DungeonView::Project(B.GetActorLocation()).Y;});
    for(auto* A:Actors) { if(A==H) Hero(H); else Enemy(CastChecked<ADungeonEnemy>(A)); }
    if(H->Health>0) DrawStatus(DungeonView::Project(H->GetActorLocation())-FVector2D(0,173),H->StunTime,H->SlowTime,0,0,H->IsBulletImmune(),H->StatusClock);
    for(const auto& Shot:G->GetShots())
    {
        FVector2D P=Shot.Position;
        if(Shot.Style==14||Shot.Style==15)
        {
            float Angle=FMath::RadiansToDegrees(FMath::Atan2(Shot.Velocity.Y,Shot.Velocity.X));
            if(Shot.Style==14)
            {
                const float T=FMath::Clamp(Shot.Age/Shot.FlightTime,0.f,1.f);
                Shadow(P,8);P.Y-=FMath::Sin(T*PI)*95;Angle=T*270;
                Ring(Shot.Target,Shot.BlastRadius,FLinearColor(1,.4f,.1f,.55f),2);
            }
            Sprite(FString::Printf(TEXT("ThemeFX_%d"),Shot.Art-40),P.X-32,P.Y-32,64,64,FLinearColor::White,Angle);continue;
        }
        if(Shot.Style==13)
        {
            const float T=FMath::Clamp(Shot.Age/Shot.FlightTime,0.f,1.f);
            Shadow(P,9);P.Y-=FMath::Sin(T*PI)*90;
            Sprite(TEXT("FlashGrenade"),P.X-18,P.Y-18,36,36,FLinearColor::White,T*360);
            if(T>=1) Ring(Shot.Target,16+(Shot.Age-Shot.FlightTime)*24,FLinearColor(1,.65f,.12f,.65f),2);
            continue;
        }
        if(Shot.Style==10||Shot.Style==11)
        {
            if(Shot.Style==10) Sprite(TEXT("BurgerSplat_0"),P.X-45,P.Y-45,90,90);
            else
            {
                // A short exposure streak, brass jacket and white-hot nose, aligned to flight.
                const auto D=Shot.Velocity.GetSafeNormal();
                auto Stroke=[&](float From,float To,FLinearColor C,float Width) {
                    const auto A=Offset+(P-D*From)*Scale,B=Offset+(P-D*To)*Scale;
                    DrawLine(A.X,A.Y,B.X,B.Y,C,Width*Scale);
                };
                Stroke(7,42,FLinearColor(1,.35f,.04f,.25f),3);
                Stroke(6,25,FLinearColor(1,.7f,.15f,.7f),2);
                Stroke(1,9,FLinearColor(.95f,.65f,.2f),4);
                Stroke(-2,2,FLinearColor(1,1,.86f),2);
            }
            continue;
        }
        const float Angle=Shot.bFriendly?Shot.Age*260:FMath::RadiansToDegrees(FMath::Atan2(Shot.Velocity.Y,Shot.Velocity.X));
        if(Shot.bFriendly)
        {
            Shadow(P,12); const float T=FMath::Clamp(Shot.Age/Shot.FlightTime,0.f,1.f);
            P.Y-=50*(1-T)+FMath::Sin(T*PI)*75;
            Ring(Shot.Target,Shot.BlastRadius,FLinearColor(.95f,.71f,.33f,.3f),1);
        }
        else if(Shot.Style!=12) P.Y-=22;
        const float Size=Shot.Art==1||Shot.Art==2?46:Shot.Art==3?42:Shot.bFriendly?42:48;
        KeySprite(FString::Printf(TEXT("TeaFX_%d"),Shot.Art),P.X-Size*.5f,P.Y-Size*.5f,Size,Size,FLinearColor::White,false,Angle);
    }
    for(const auto& I:G->GetImpacts())
    {
        float Age=.65f-I.Life; FLinearColor C=I.bBoss?FLinearColor(1,.23f,.07f):Gold; C.A=I.Life/.65f;
        if(Age<.25f) for(int J=0;J<9;++J)
        {
            float A=J*2*PI/9; FVector2D Q=I.Position+FVector2D(FMath::Cos(A),FMath::Sin(A))*(12+Age*160)-FVector2D(0,35);
            Box(Q.X,Q.Y,4,4,C);
        }
        if(I.Damage>0) Label(FString::Printf(TEXT("%.0f"),I.Damage),I.Position.X-8,I.Position.Y-75-Age*42,C,1.4f);
        else Ring(I.Position,30+Age*150,C,5);
    }
    DrawCombatFX(G,true);
    Offset=StableOffset; // Shake the dungeon, never the HUD or mouse-input mapping.
    Label(FString::Printf(TEXT("%s / ROOM %02d"),DungeonRoster::Biome(G->GetBiome()),G->GetRoom()),900,28,Gold,.85f);
    for(int I=0;I<3&&I<H->Equipment.Num();++I)
    {
        const auto& E=H->Equipment[I]; float X=1040+I*66;
        Box(X,56,60,62,Ink); Box(X,116,60,2,RarityColor(E.Rarity));
        if(!E.IsEmpty()) Sprite(E.Art(),X+5,59,50,50);
        else Label(TEXT("--"),X+23,78,Pale);
    }
    for(auto& E:G->GetEnemies()) if(IsValid(E)&&E->bBoss)
    {
        DrawBossUI(E);
    }
    DrawVitals(H);
    DrawDialogue(G,H);
    if(H->IsInventoryOpen()) DrawInventory(H);
    if(G->IsTransitioning())
    {
        const float T=G->TransitionProgress();
        Box(0,0,1280,800,FLinearColor(0,0,0,DungeonDescent::Blackout(T)));
    }
}

void ADungeonGameMode::VerifyGameplay()
{
#if !UE_BUILD_SHIPPING
    int Errors=0;
    auto Check=[&](bool B,const TCHAR* What){if(!B){++Errors;UE_LOG(LogTemp,Error,TEXT("DUNGEON_VERIFY: %s"),What);}};
    auto* H=Player(this); Check(H!=nullptr,TEXT("Hero spawned"));
    if(!H) { FPlatformMisc::RequestExitWithStatus(false,1); return; }
    H->Restart();
    FVector2D Start=DungeonView::Project(H->GetActorLocation());
    H->MoveForward(1); H->Tick(.1f); H->MoveForward(0);
    FVector2D WalkEnd=DungeonView::Project(H->GetActorLocation());
    Check(WalkEnd.Y<Start.Y&&FMath::Abs(WalkEnd.X-Start.X)<.01f,TEXT("W moves screen-up"));
    H->SetActorLocation(DungeonView::Unproject(Start)); H->SprintPressed(); H->MoveForward(1); H->Tick(.1f);
    FVector2D SprintEnd=DungeonView::Project(H->GetActorLocation());
    Check(FMath::IsNearlyEqual((float)(Start.Y-SprintEnd.Y),(float)(2*(Start.Y-WalkEnd.Y)),.01f),TEXT("Sprint is 2x walk speed"));
    H->SprintReleased(); H->MoveForward(0); H->Restart();
    for(int D=0;D<8;++D)
    {
        float A=D*PI/4;
        Check(DungeonView::Direction(FVector2D(FMath::Sin(A),-FMath::Cos(A)))==D,TEXT("8-way direction mapping"));
    }
    FVector2D Test(811,355);
    Check(DungeonView::Project(DungeonView::Unproject(Test)).Equals(Test,.01f),TEXT("Projection round-trip"));
    PendingSpawns=0; SpawnOneEnemy(); auto* E=Enemies[0].Get(); E->SpawnTime=0;
    H->SetActorLocation(DungeonView::Unproject(FVector2D(640,400)));
    E->SetActorLocation(DungeonView::Unproject(FVector2D(640,468)));
    float Before=E->Health; PlayerAttack(H); Check(E->Health<Before,TEXT("Melee hits without overlap"));
    while(!bChest)
    {
        while(PendingSpawns>0) { SpawnOneEnemy(); --PendingSpawns; }
        const auto Batch=Enemies;
        for(auto& Target:Batch) { Target->SpawnTime=0; Target->TakeDungeonDamage(10000); }
    }
    Check(Room==1&&Wave==2,TEXT("Two waves before reward"));
    H->SetActorLocation(DungeonView::Unproject(FVector2D(640,424)));
    const float BeforeAttack=H->AttackPower,BeforeArmor=H->Armor,BeforeMax=H->MaxHealth;
    PlayerInteract(H); Check(bLootClaimed&&!bChest&&H->Inventory.Num()==1,TEXT("Chest adds one bag item and unlocks"));
    Check(H->AttackPower==BeforeAttack&&H->Armor==BeforeArmor&&H->MaxHealth==BeforeMax,TEXT("Pickup does not equip or change stats"));
    PlayerInteract(H); Check(H->Inventory.Num()==1,TEXT("Claimed chest cannot duplicate reward"));
    FDungeonItem Item=MakeItem(2,4); H->Equip(Item); float Attack=H->AttackPower; H->Equip(Item);
    Check(FMath::IsNearlyEqual(Attack,H->AttackPower),TEXT("Equipment replacement does not stack"));
    H->Equip(MakeItem(5,3)); H->Equip(MakeItem(8,2));
    Check(H->Armor>8&&H->MaxHealth>150,TEXT("Armor and health slots apply"));
    TransitionCooldown=0; H->SetActorLocation(DungeonView::Unproject(DoorPosition(1))); PlayerInteract(H);
    Check(Room==2,TEXT("Door enters room two"));
    PendingSpawns=0; SpawnOneEnemy(); E=Enemies[0]; E->SpawnTime=0;
    Check(E->bBoss&&E->Health==360,TEXT("Room two boss"));
    E->SetActorLocation(H->GetActorLocation()); E->Tick(.01f);
    Check(E->Windup>0,TEXT("Boss telegraphs before damage"));
    E->TakeDungeonDamage(10000); Check(bChest,TEXT("Boss defeat unlocks chest"));
    H->SetActorLocation(DungeonView::Unproject(FVector2D(640,424))); PlayerInteract(H);
    Check(Loot.Rarity==4&&Loot.Icon==2,TEXT("Boss legendary Cinderbrand reward"));
    Check(!Impacts.IsEmpty(),TEXT("Damage creates impact effects"));
    for(int I=0;I<3;++I)
    {
        const int BeforeRoom=Room;
        bLootClaimed=true; TransitionCooldown=0; PendingSpawns=0;
        H->SetActorLocation(DungeonView::Unproject(DoorPosition(I))); PlayerInteract(H);
        Check(Room==BeforeRoom+1,TEXT("All three visible doors usable"));
    }
    H->Restart(); H->Inventory.Empty();
    for(int I=0;I<18;++I) Check(H->AddToInventory(MakeItem(I%3,I%5)),TEXT("18 weapons fit the 6x6 grid"));
    Check(!H->AddToInventory(MakeItem(0,0)),TEXT("19th weapon rejected"));
    for(int I=0;I<H->Inventory.Num();++I) Check(H->CanPlace(H->Inventory[I].Cell,H->Inventory[I].Item.Size(),I),TEXT("Inventory entries in bounds with no overlaps"));
    const FDungeonItem Starter=H->Equipment[0];
    Check(H->EquipFromInventory(2)&&H->Inventory.Num()==18,TEXT("Full inventory permits a weapon swap"));
    Check(H->Equipment[0].Icon==2&&H->Inventory[2].Item.Name==Starter.Name,TEXT("Swap preserves old weapon"));
    Check(!H->Unequip(0)&&H->Equipment[0].Icon==2,TEXT("Full bag cannot lose unequipped item"));
    H->Inventory.Empty();
    for(int I=0;I<9;++I) Check(H->AddToInventory(MakeItem(3+I%3,2)),TEXT("Nine armor pieces fit"));
    Check(!H->AddToInventory(MakeItem(6,0)),TEXT("Armor fills exactly 36 cells"));
    Check(H->EquipFromInventory(2)&&H->Inventory.Num()==9&&H->Equipment[1].Icon==5,TEXT("Full bag armor swap"));
    bChest=true; bLootClaimed=bLootRolled=false; Room=2;
    H->SetActorLocation(DungeonView::Unproject(FVector2D(640,424))); PlayerInteract(H);
    Check(bChest&&!bLootClaimed&&Loot.Icon==2,TEXT("Full bag leaves boss reward in chest"));
    const FString SavedLoot=Loot.Name; PlayerInteract(H);
    Check(Loot.Name==SavedLoot&&H->Inventory.Num()==9,TEXT("Retry neither rerolls nor duplicates loot"));
    H->Inventory.RemoveAt(0); PlayerInteract(H);
    Check(!bChest&&bLootClaimed&&H->Inventory.Num()==9,TEXT("Make space then collect saved reward"));
    H->ToggleInventory(); const auto PausedPos=H->GetActorLocation(); H->MoveForward(1); H->Tick(.1f); H->MoveForward(0);
    Check(H->GetActorLocation().Equals(PausedPos),TEXT("Inventory pauses movement")); H->ToggleInventory();
    H->Inventory.Empty(); H->AddToInventory(MakeItem(8,4)); H->EquipFromInventory(0);
    const float HP=H->Health; H->Unequip(2); H->EquipFromInventory(0);
    Check(H->Health<=HP,TEXT("Vitality swaps cannot heal repeatedly"));
    for(const auto& Set:{TEXT("Sentinel"),TEXT("Verdant"),TEXT("Warden")})
        for(const auto& State:{TEXT("Walk"),TEXT("Attack")}) for(const auto& Group:{TEXT("Cardinal"),TEXT("Diagonal")})
            for(int R=0;R<4;++R) for(int F=0;F<6;++F)
            {
                FString N=FString::Printf(TEXT("%s_%s%s_%d_%d"),Set,State,Group,R,F);
                Check(LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Art/V2/%s.%s"),*N,*N))!=nullptr,TEXT("Worn armor animation texture load"));
            }
    Check(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/V2/M_WornArmor.M_WornArmor"))!=nullptr,TEXT("Worn armor material load"));
    for(const auto& State:{TEXT("Walk"),TEXT("Attack")}) for(const auto& Group:{TEXT("Cardinal"),TEXT("Diagonal")})
        for(int R=0;R<4;++R) for(int F=0;F<6;++F)
        {
            FString N=FString::Printf(TEXT("%s%s_%d_%d"),State,Group,R,F);
            FString P=FString::Printf(TEXT("/Game/Art/V2/%s.%s"),*N,*N);
            Check(LoadObject<UTexture2D>(nullptr,*P)!=nullptr,TEXT("Animation texture load"));
        }
    for(const auto& State:{TEXT("EnemyWalk"),TEXT("EnemyAttack"),TEXT("BossMotion")})
        for(int R=0;R<(FString(State)==TEXT("BossMotion")?2:4);++R) for(int F=0;F<4;++F)
        {
            FString N=FString::Printf(TEXT("%s_%d_%d"),State,R,F);
            FString P=FString::Printf(TEXT("/Game/Art/V2/%s.%s"),*N,*N);
            Check(LoadObject<UTexture2D>(nullptr,*P)!=nullptr,TEXT("Enemy and boss animation texture load"));
        }
    UE_LOG(LogTemp,Display,TEXT("DUNGEON_VERIFY_COMPLETE errors=%d"),Errors);
    FPlatformMisc::RequestExitWithStatus(false,Errors?1:0);
#endif
}
