#pragma once
#include "IronMatriarch.h"
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "DungeonProgression.h"
#include "DungeonDescent.h"
#include "DungeonBlock.h"
#include "DungeonBalance.h"
#include "DungeonTeaSpirit.h"
#include "DungeonRevival.h"
#include "DungeonSettings.h"
#include "DungeonActors.generated.h"

namespace DungeonLootCatalog { constexpr int32 Count=72,EquipmentSlots=8; }
class UCameraComponent;
class UTexture2D;
class UFont;
class UMaterialInstanceDynamic;
class UAudioComponent;
class USoundBase;
class ADungeonGameMode;
class SBackgroundBlur;
class SBorder;
namespace Scalability { struct FQualityLevels; }

// Shared virtual canvas: input, combat and rendering use one mapping.
namespace DungeonView
{
    inline FVector2D Project(const FVector& P) { return {640.+(P.Y-P.X)*.24,424.+(P.X+P.Y)*.12}; }
    inline FVector Unproject(const FVector2D& P) { const FVector2D D=P-FVector2D(640,424); return {(D.Y/.12-D.X/.24)*.5,(D.Y/.12+D.X/.24)*.5,0}; }
    inline FVector2D Clamp(const FVector2D& P) { return {FMath::Clamp(P.X,110.,1170.),FMath::Clamp(P.Y,212.,702.)}; }
    inline int32 Direction(const FVector2D& D) { return (FMath::RoundToInt(FMath::Atan2(D.X,-D.Y)/(PI/4))+8)%8; }
}
struct FDungeonItem
{
    FString Name;
    int32 Slot=0, Icon=0, Rarity=0;
    float Attack=0, Defense=0, Vitality=0;
    int32 CatalogId=-1, ItemLevel=1, Effect=0;
    float CritChance=0,CritDamage=0,Speed=0,Regen=0,Movement=0;
    int32 CoinValue=0;
    float BleedChance=0,PoisonChance=0,StaminaBonus=0,Leech=0,Reduction=0,HealthBonus=0;
    FString Art() const { return CatalogId<0?FString::Printf(TEXT("Item_%d_%d"),Icon/3,Icon%3):FString::Printf(TEXT("Loot_%d"),CatalogId); }
    FString BagArt() const {return CatalogId>=0&&CatalogId<24?FString::Printf(TEXT("BagWeapon_%d"),CatalogId):Art();}
    FString EffectText() const;
    float EquippedScale() const;
    FString Stats() const;
    FIntPoint Size() const { return Slot==0||Slot==6?FIntPoint(1,2):Slot==1?FIntPoint(2,2):FIntPoint(1,1); }
    bool IsEmpty() const { return Name.IsEmpty(); }
};
struct FDungeonBagEntry { FDungeonItem Item; FIntPoint Cell; };
struct FDungeonPotion { FVector2D Position; float Age=0; };
struct FDungeonCoinDrop { FVector2D Position; int32 Amount=0; float Age=0; };
struct FDungeonBreakable
{
    FVector2D Position;
    int32 Variant=0;
    float BrokenAge=-1;
    bool Collected=false,BagFull=false;
    FDungeonItem Loot;
};
enum class ERewardPhase : uint8 { Closed, Opening, Ejecting, Available, Collected };
struct FRewardPresentation
{
    ERewardPhase Phase=ERewardPhase::Closed;
    int32 Choice=INDEX_NONE;
    float Age=0,CollectedAge=0;
    bool BagFull=false;
    FVector2D Landing=FVector2D::ZeroVector;
};
struct FDungeonBlood { FVector2D Position; float Age=0,Size=80; int32 Variant=0,RustDirection=-1,ExpansionSpecies=-1,ExpansionDirection=0; bool bRemains=false; };
enum class EAtlasRoom : uint8 { Entrance, Combat, Trader, Reward, Boss };
// Run-local snapshots; rooms can only be left after combat, so enemies need no respawn snapshot.
struct FAtlasRoom
{
    FIntPoint Cell;
    EAtlasRoom Type=EAtlasRoom::Combat;
    int32 Links[4]={-1,-1,-1,-1}; // north, east, south, west
    int32 Depth=0;
    bool Visited=false,Cleared=false,Chest=false,LootRolled=false,LootClaimed=false,StockMade=false,RetryEncounter=false;
    FRewardPresentation Reward;
    FDungeonItem Loot,ChestLoot[3];
    bool ChestRolled[3]={false,false,false};
    TArray<FDungeonBreakable> Props;
    TArray<FDungeonPotion> Potions;
    TArray<FDungeonCoinDrop> Coins;
    TArray<FDungeonItem> Stock;
};
struct FDungeonImpact
{
    FVector2D Position;
    float Life=.65f, Damage=0;
    bool bBoss=false;
};
struct FDungeonShot
{
    int32 ExpansionSpecies=-1;
    TWeakObjectPtr<class ADungeonEnemy> SourceEnemy;
    FVector2D Position,Velocity;
    float Life=4,Radius=9,Damage=10,HitHeight=0;
    int32 Style=1;
    int32 Art=3;
    FVector2D Origin,Target;
    float Age=0,FlightTime=1,BlastRadius=0;
    bool bFriendly=false;
    bool bMelee=false;
    bool bBossAttack=false; // Snapshot provenance; survives the firing actor's death.
    FVector2D MeleeOrigin=FVector2D::ZeroVector;
    bool bMotionTuned=false,bTrackingStopped=false,bPlayerBodyHit=false;
};
struct FDungeonSplash
{
    int32 TeaKind=0; // 0 shared/non-TEA, 1 ground, 2 confirmed enemy overlay
    TWeakObjectPtr<class ADungeonEnemy> TeaEnemy;
    float TeaHeight=0;
    int32 ExpansionSpecies=-1;
    bool bExpansionPlayerHit=false;
    FVector2D Position;
    float Radius=50,Life=.65f;
    int32 Art=8;
    bool bFriendly=false;
};
UCLASS()
class THEBEARDANDBLADE_API ADungeonHero : public APawn
{
    GENERATED_BODY()
public:
    ADungeonHero();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    void RebindInput();
    bool PendingRebind=false;
    uint64 UIClickFrame=MAX_uint64;
    void InputClick();
    void GameplayAttack();
    bool HeldDirections[4]={false,false,false,false};
    void MoveKey(int Direction,bool Held);
    void MoveForward(float V) { InputY=-V; }
    void MoveRight(float V) { InputX=V; }
    void SprintPressed() { bSprinting=true; }
    void SprintReleased() { bSprinting=false; }
    void Attack();
    void CancelCombatActions();
    void RestoreAfterDeath();
    FString AttackQuip;
    float QuipTime=0,QuipCooldown=0;
    void PowerMove();
    void Freedom();
    void DrinkTea();
    void DrinkPotion();
    bool CanDrinkPotion() const;
    static constexpr int32 PotionCapacity=4;
    static constexpr float PotionDrinkDuration=.65f;
    int32 PotionCharges=0;
    float PotionSip=0;
    bool IsDrinkingPotion() const { return PotionSip>0; }
    bool IsDrinking() const { return IsDrinkingTea()||IsDrinkingPotion(); }
    bool CanDrinkTea() const;
    bool IsTeaEmpowered() const { return Health>0&&TeaSpirit.Active>0; }
    bool IsDrinkingTea() const { return TeaSpirit.Sip>0; }
    float GetTeaSpiritTime() const { return TeaSpirit.Active; }
    float GetTeaSpiritCooldown() const { return TeaSpirit.Cooldown; }
    float GetTeaSipProgress() const { return 1-TeaSpirit.Sip/FDungeonTeaSpirit::SipDuration; }
    void BlockPressed();
    void BlockReleased(){Block.Release();}
    void StopBlock(){Block.Stop();}
    bool IsBlocking() const{return Block.Active;}
    bool CanStartBlock() const;
    bool CanUseTea() const;
    bool CanUseFreedom() const;
    bool CanStrike() const;
    float GetBlockFraction() const{return Block.Fraction();}
    float GetBlockCountdown() const{return Block.Active?FDungeonBlock::Limit-Block.Elapsed:Block.Cooldown;}
    float GetBlockCooldown() const{return Block.Cooldown;}
    float GetBlockImpact() const{return Block.Impact;}
    bool NeedsBlockRelease() const{return Block.Held&&!Block.Active&&!Block.Exhausted;}
    bool ReceiveMeleeHit(float Damage,FVector2D Attacker,bool BossAttack=false);
    bool TryBlockDamage(FVector2D ToSource);
    float GetGuardBlend() const{return GuardBlend;}
    bool IsCasting() const { return PowerCastTime>0; }
    float GetPowerCooldown() const { return PowerCooldown; }
    float GetCastProgress() const { return 1.f-PowerCastTime/.48f; }
    void Dodge();
    float Stamina=100,MaxStamina=100;
    bool bExhausted=false;
    void UpdateStamina(float Dt,bool Sprinting);
    void SpendStamina(float Amount);
    void Menu();
    void ToggleAtlas();
    void ToggleMusic();
    void ToggleEffects();
    void Confirm();
    void SkipIntro();
    void TestFinance();
    void TestMack();
    void TestWebroot();
    void TestRime();
    void TestCinder();
    void TestIce();
    void TestTwister();
    void TestFlashBang();
    void TestGreaseEnemies();
    void TestBunkerEnemies();
    void TestStormEnemies();
    bool IsRolling() const { return RollTime>0; }
    float RollProgress() const { return 1.f-RollTime/.48f; }
    float GetRollCooldown() const { return RollCooldown; }
    int32 GetRollDirection() const { return RollDirection; }
    void TransitionWalk(FVector2D From,FVector2D To,float Progress,float GaitScale=1.f);
    void Interact();
    void Restart();
    void ReceiveHit(float Damage,bool PerProjectile=false,TOptional<FVector2D> Source={},bool BossAttack=false);
    void RestoreHealth(float Amount,bool FromGear=false);
    float GearHealBudget=0;
    bool IsBulletImmune() const;
    FVector2D ScreenVelocity=FVector2D::ZeroVector;
    void Equip(const FDungeonItem& Item);
    void RebuildStats();
    bool HasEffect(int32 Effect) const;
    float CritChance=.05f,CritMultiplier=1.5f,AttackSpeed=1,StaminaRegen=1,MovementSpeed=1;
    float BleedChance=0,PoisonChance=0,Leech=0,DamageReduction=0;
    int32 StrikeCount=0;
#if !UE_BUILD_SHIPPING
    void SetReviewPose(int32 D,int32 F) { Facing=AttackDirection=D; AttackTime=.48f*(1.f-(F+.01f)/6.f); bAttackHit=true; }
    void SetWalkReviewPose(int32 D,int32 F) { Facing=D;AttackTime=PowerCastTime=RollTime=0;bWalking=true;WalkDistance=(F+.01f)*18;IdleBreathBlend=0;MoveBlend=1;RunBlend=0;GaitTravel=FVector2D(FMath::Sin(D*PI/4),-FMath::Cos(D*PI/4)); }
    void SetSprintReviewPose() { RunBlend=1; }
    void SetIdleReviewPose(int32 D,float Phase) { Facing=D;AttackTime=PowerCastTime=RollTime=HurtTime=0;bWalking=false;IdleBreathBlend=1;BreathPhase=Phase; }
    void SetFlashReviewAim(FVector2D Direction) { Aim=Direction;Facing=DungeonView::Direction(Aim); }
#endif
    void ToggleInventory();
    bool AddToInventory(const FDungeonItem& Item);
    bool EquipFromInventory(int32 Index);
    bool Unequip(int32 Slot);
    bool UnequipToCell(int32 Slot,FIntPoint Cell);
    bool CanPlace(FIntPoint Cell,FIntPoint Size,int32 Ignore=INDEX_NONE) const;
    bool MoveBagItem(int32 Index,FIntPoint Cell);
    bool IsInventoryOpen() const { return bInventoryOpen; }
    TArray<FDungeonBagEntry> Inventory;
    int32 SelectedItem=INDEX_NONE;
    int64 Coins=0;
    bool DiscardItem(int32 Index);
    FString InventoryMessage;
    bool IsAttacking() const { return AttackTime>0; }
    bool IsWalking() const { return bWalking; }
    float WalkCycle() const { return WalkDistance/144.f*2.f*PI; }
    float GetMoveBlend() const { return MoveBlend; }
    float GetRunBlend() const { return RunBlend; }
    FVector2D GetGaitTravel() const { return GaitTravel; }
    int32 GetFacingDirection() const { return IsCasting()?PowerDirection:IsAttacking()?AttackDirection:Facing; }
    int32 GetAnimationFrame() const;
    int32 GetLocomotionFrame() const { return bWalking?FMath::FloorToInt(WalkDistance/18.f)%8:2; }
    bool IsRunAnimation() const { return bWalking&&RunBlend>.5f; }
    FVector2D GetVisualFacing() const { const float A=GetFacingDirection()*PI/4;return FVector2D(FMath::Sin(A),-FMath::Cos(A)); }
    float GetAttackProgress() const { return 1.f-AttackTime/.48f; }
    FVector2D GetAim() const { return IsCasting()?PowerAim:IsAttacking()?AttackAim:Aim; }
    float Health=150,MaxHealth=150,AttackPower=24,Armor=8,HurtTime=0;
    float StunTime=0,SlowTime=0,StatusClock=0;
    float FlashBlindTime=0;
    bool ApplyFlashBang(FVector2D Explosion,float Radius,bool BossAttack=false);
    void ReceiveFlashStab(TOptional<FVector2D> Source={});
    float IdleBreathBlend=0,BreathPhase=0;
    bool IsDamageImmune() const;
    void ApplyBurgerStatus();
    TArray<FDungeonItem> Equipment;
    UPROPERTY(VisibleAnywhere) UCameraComponent* Camera;
private:
    FDungeonBlock Block;
    float GuardBlend=0;
    float InputX=0,InputY=0,WalkDistance=0,AttackTime=0,Invulnerable=0;
    float RollTime=0,RollCooldown=0;
    float StaminaDelay=0;
    float PowerCooldown=0,PowerCastTime=0;
    FDungeonTeaSpirit TeaSpirit;
    int32 PowerDirection=4;
    FVector2D PowerAim=FVector2D(0,1),PowerTarget;
    bool bTeaReleased=false;
    int32 RollDirection=2;
    FVector2D RollAim=FVector2D(1,0);
    bool bSprinting=false,bWalking=false,bAttackHit=false;
    bool bInventoryOpen=false;
    float FootstepDistance=0,MoveBlend=0,RunBlend=0;
    FVector2D GaitTravel=FVector2D::ZeroVector;
    int32 Facing=4,AttackDirection=4;
    FVector2D Aim=FVector2D(0,1),AttackAim=FVector2D(0,1);
};
UCLASS()
class THEBEARDANDBLADE_API ADungeonEnemy : public APawn
{
    GENERATED_BODY()
public:
    ADungeonEnemy();
    virtual void Tick(float Dt) override;
    void TakeDungeonDamage(float Damage);
    int32 Species=0;
    float StaggerGuard=0;
    int32 BalanceMeleeHits=0;
    int32 AttackCount=0;
    int32 BossAttack=0; // 0 close strike, 1 aimed volley, 2 heavy area strike, 3 radial barrage
    float MotionClock=0,ActionTime=0,ActionDuration=0,ShotTimer=0,FlashTime=0;
    int32 Action=0,ShotsRemaining=0,ShotSerial=0;
    FVector2D ActionFrom,ActionTo;
    float HealthLag=0;
    void TickNewBoss(float Dt,ADungeonHero* H,ADungeonGameMode* G);
    IronMatriarch::FState Iron;
    void TickIronMatriarch(float Dt,ADungeonHero* H,ADungeonGameMode* G);
    void BeginIronAttack(int Attack,ADungeonHero* H,ADungeonGameMode* G);
    void TickFlashBoss(float Dt,ADungeonHero* H,ADungeonGameMode* G);
    void TickRustblade(float Dt,ADungeonHero* H,ADungeonGameMode* G);
    void TickExpansion(float Dt,ADungeonHero* H,ADungeonGameMode* G);
    float ExpansionAttackAge=-1,ExpansionHurtAge=-1,ExpansionAdvanceTime=0;
    bool bExpansionReleased=false,bExpansionRanged=false;
    float RustAttackAge=-1,RustHurtAge=-1;
    bool bRustStrikeFired=false;
    void BeginFlashAmbush(ADungeonHero* H,ADungeonGameMode* G);
    FVector2D DrakeMouth() const;
    float BleedTime=0,PoisonTime=0,SlowTime=0,BleedDPS=0,PoisonDPS=0;
    float AilmentDamage=0,AilmentTick=0;
    void UpdateAilments(float Dt);
    float AttackWindup=1,AttackRadius=90;
    float ChargeTime=0;
    FVector2D ChargeAim=FVector2D::ZeroVector;
    float Health=50,MaxHealth=50,HurtTime=0,SpawnTime=.6f,Windup=0,Recovery=0,FreedomImmuneTime=0;
    float WalkDistance=0;
    float FlightSoundCooldown=0;
    int32 Facing=2;
    int32 AnimationFrame() const;
    bool bBoss=false,bWalking=false;
    FVector2D AttackTarget=FVector2D::ZeroVector;
    bool IsHurt() const { return HurtTime>0; }
    bool IsAttacking() const { return Windup>0 || Recovery>.6f; }
    bool IsWalking() const { return bWalking; }
};
// Retained names allow existing maps to load. Presentation belongs to one HUD.
UCLASS()
class THEBEARDANDBLADE_API ADungeonChest : public AActor { GENERATED_BODY() };
UCLASS()
class THEBEARDANDBLADE_API ADungeonDoor : public AActor { GENERATED_BODY() };
UCLASS()
class THEBEARDANDBLADE_API ADungeonGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ADungeonGameMode();
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
    void PlayerAttack(ADungeonHero* Hero);
    int BalanceFloor() const;
    int BalanceDepth() const;
    int LootLevel() const;
    float IncomingDamageScale(bool Boss) const;
    bool PermitEnemyAttack(ADungeonEnemy* Enemy);
    void BeginBalanceRoom();
    void VerifyBalance();
    void VerifyTeaSpirit();
    void VerifyPotions();
    void VerifyMenu();
    void VerifySettings();
    void ReviewTeaSpirit(float Dt);
    void EndBalanceRoom(const TCHAR* Outcome);
    void BalanceEvent(const TCHAR* Event,float Value=0,int Species=-1);
    struct FBalanceReport {
        bool Active=false;
        int Floor=0,Depth=0,Node=0,Kills=0,Attacks=0,Landed=0,Blocked=0,Avoided=0,MeleeHits=0;
        float Seconds=0,DamageTaken=0,Healing=0,StartHealth=0;
    } Balance;
    float NextEnemyAttack=0;
    FString BalanceRun;
    void PlaySound(const FString& Name,float Volume=1.f,float Pitch=1.f);
    void VerifyIronMatriarch();
    void ReviewIronMatriarch(float Dt);
    void UpdateAudio();
    void RefreshMusicVolume();
    void StopMusic();
    void RunPackagedSmokeTest();
    void ToggleMusic();
    void ToggleEffects();
    bool IsMusicMuted() const { return bMusicMuted; }
    float GetMasterVolume() const { return MasterVolume; }
    void SetMasterVolume(float Value,bool Save=true);
    float MusicVolume=1,EffectsVolume=1,VoiceVolume=1,InterfaceVolume=1;
    float SoundCategoryGain(const FString& Name) const;
    void LoadAudioSettings();
    void SaveAudioSettings();
    bool AreEffectsMuted() const { return bEffectsMuted; }
    void PlayerInteract(ADungeonHero* Hero);
    void EnemyDefeated(ADungeonEnemy* Enemy);
    bool ActivateFreedom(ADungeonHero* Hero);
    void UpdateFreedom(float Dt);
    int32 FreedomKills=0;
    float FreedomTime=0;
    bool IsFreedomActive() const { return FreedomTime>0; }
    bool IsFreedomReady() const {return !IsFreedomActive()&&FreedomKills>=15&&!bChest&&!bLootClaimed;}
    float FreedomProgress() const { return 1.f-FreedomTime/3.f; }
    TArray<FDungeonBlood> Blood;
    void RestartRun();
    void FinishRun(bool Victory);
    void UpdateEnding(float Dt);
    void RestartFromEnding();
    bool ReviveFromDeath();
    bool CanReviveFromDeath() const;
    int64 RevivalCost() const;
    void VerifyRevival();
    void ReviewRevival(float Dt);
    void VerifyEndings();
    void VerifyWeekend();
    void VerifyRustblade();
    void VerifyExpansion();
    void VerifyHotbar();
    void UpdateHotbarReview(float Dt);
    void StartEliteEncounter(int32 Species);
    int32 EliteReviewSpecies=-1;
    void ReleaseExpansion(ADungeonEnemy* E,ADungeonHero* H);
    void UpdateExpansionShot(FDungeonShot& S,float Dt,ADungeonHero* H);
    void RunEndingPreview();
    bool HasEnding() const { return EndState!=0; }
    bool IsVictory() const { return EndState==2; }
    bool IsDeathSequence() const { return EndState==1; }
    float GetEndingTime() const { return EndTime; }
    void StartGame();
    void StartPlaytestRoom(int32 Number);
    void CompleteRoom();
    bool CanCollectReward(const ADungeonHero* Hero) const;
    void ToggleMenu();
    void StartTransition(int32 Door);
    void FireAttack(ADungeonEnemy* Enemy);
    void LaunchTea(ADungeonHero* Hero,FVector2D Target);
    void UpdateProjectiles(float Dt);
    void ResolveProjectile(const FDungeonShot& Shot,FVector2D Position);
    const TArray<FDungeonSplash>& GetSplashes() const { return Splashes; }
    const TArray<FDungeonPotion>& GetPotions() const { return Potions; }
    void UpdatePotions(float Dt);
    bool IsMenu() const { return bMenu; }
    bool HasRun() const { return bHasRun; }
    bool IsTransitioning() const { return TransitionTime>0; }
    bool IsGameplayBlocked() const { return bAtlasMap||AtlasTravelTime>0||AtlasArrivalTime>0||bTraderOpen||HasEnding()||bMenu||IsTransitioning()||IsBossIntroActive()||IsBossDialogueActive()||BossGrace>0; }
    bool IsAtlasFloor() const { return bAtlasActive; }
    bool IsAtlasMapOpen() const { return bAtlasMap; }
    bool IsAtlasTravel() const { return AtlasTravelTime>0||AtlasArrivalTime>0; }
    const TArray<FAtlasRoom>& GetAtlasRooms() const { return AtlasRooms; }
    int32 GetAtlasRoom() const { return AtlasCurrent; }
    void InitializeAtlasFloor(int32 Seed,int32 Chapter=0);
    int32 GetAtlasChapter() const { return AtlasChapter; }
    void SaveAtlasRoom();
    void EnterAtlasRoom(int32 Index,int32 EntryDoor);
    void ToggleAtlasMap();
    bool AtlasInteract(ADungeonHero* Hero);
    void StartAtlasTravel(int32 Direction);
    void TickAtlasTravel(float Dt);
    void DisableAtlas();
    void VerifyAtlas();
    void RunAtlasReview();
    float AtlasHeroOpacity() const;
    float AtlasBlackout() const;
    bool AtlasHasDoor(int32 D) const;
    bool AtlasIsDescentDoor(int32 D) const;
    static FVector2D AtlasDoor(int32 D);
    static FVector2D AtlasDirection(int32 D);
    bool IsTraderOpen() const { return bTraderOpen; }
    void OpenTrader();
    void ContinueFromTrader();
    bool BuyTraderItem(ADungeonHero* Hero,int32 Index);
    bool SellTraderItem(ADungeonHero* Hero,int32 Index);
    static int64 TraderSellPrice(const FDungeonItem& Item) { return FMath::Max<int64>(1,Item.CoinValue/2); }
    bool DropInventoryItem(ADungeonHero* Hero,const FDungeonItem& Item);
    void UpdateCoinDrops(float Dt);
    void VerifyEconomy();
    const TArray<FDungeonCoinDrop>& GetCoinDrops() const { return CoinDrops; }
    static int64 TraderPrice(const FDungeonItem& Item) { return (FMath::Max<int64>(1,Item.CoinValue)*6+4)/5; }
    const TArray<FDungeonItem>& GetTraderStock() const { return TraderStock; }
    FString TraderMessage;
    void VerifyTrader();
    bool IsBossIntroActive() const { return BossIntroTime>=0; }
    float GetBossIntroTime() const { return BossIntroTime; }
    void StartBossIntro();
    void FinishBossIntro();
    void CancelBossIntro();
    void UpdateBossIntro(float Dt);
    void VerifyBossIntro();
    bool IsBossDialogueActive() const { return DialogueIndex<DialogueLines.Num(); }
    void BeginBossDialogue();
    void AdvanceBossDialogue(bool Skip=false);
    const FString& GetDialogueLine() const { return DialogueLines[DialogueIndex]; }
    FString GetDialogueSpeaker() const;
    int32 GetDialogueIndex() const { return DialogueIndex; }
    int32 GetDialogueCount() const { return DialogueLines.Num(); }
    bool CanAdvanceDialogue() const { return DialogueWait<=0; }
    float TransitionProgress() const { return 1.f-TransitionTime/DungeonDescent::Duration; }
    int32 GetBiome() const { return DungeonProgression::Themes[DungeonProgression::Chapter(Room)]; }
    int32 GetBossSpecies() const { return DungeonProgression::Bosses[DungeonProgression::Chapter(Room)]; }
    void FireThemeAttack(ADungeonEnemy* Enemy);
    void VerifyProgression();
    void ThrowFlashBang(ADungeonEnemy* Enemy);
    void VerifyFlashBang();
    void VerifyTeaV4();
    void ReviewTeaV4(float Dt);
    void UpdateReward(float Dt);
    void SpawnBreakables();
    void StrikeBreakables(ADungeonHero* Hero);
    bool CollectBreakableLoot(ADungeonHero* Hero);
    void VerifyBreakables();
    const TArray<FDungeonBreakable>& GetBreakables() const { return Breakables; }
    bool InteractReward(ADungeonHero* H);
    const FRewardPresentation& GetRewardPresentation() const { return Reward; }
    void EmitBossShot(ADungeonEnemy* E,bool Burger);
    void AddBossFX(FVector2D P,int32 Art);
    void VerifySeptember();
    void RunSeptemberSmoke();
    bool bShowControls=false,bShowSettings=false,bConfirmQuit=false;
    const TArray<FDungeonShot>& GetShots() const { return Shots; }
    void AddImpact(FVector2D P,float Damage,bool bBoss=false);
    FString GetObjective() const;
    int32 GetRoom() const { return Room; }
    int32 GetWave() const { return Wave; }
    const TArray<TObjectPtr<ADungeonEnemy>>& GetEnemies() const { return Enemies; }
    bool HasChest() const { return bChest; }
    bool AreDoorsOpen() const { return bAtlasActive?AtlasRooms.IsValidIndex(AtlasCurrent)&&AtlasRooms[AtlasCurrent].Cleared:bLootRolled||bLootClaimed; }
    bool IsLootRevealed() const { return LootTimer>0; }
    bool IsDead() const;
    bool IsBossRoom() const { return bAtlasActive?AtlasRooms.IsValidIndex(AtlasCurrent)&&AtlasRooms[AtlasCurrent].Type==EAtlasRoom::Boss:Room%DungeonProgression::RoomsPerChapter==0; }
    const FDungeonItem& GetLoot() const { return Loot; }
    const TArray<FDungeonImpact>& GetImpacts() const { return Impacts; }
    static FDungeonItem MakeItem(int32 Icon,int32 Rarity);
    static FDungeonItem RollChestLoot(bool Boss,int32 Level=1);
    static FDungeonItem RollItem(int32 Definition,int32 Rarity,int32 Level=1);
    static TArray<FDungeonItem> CreateTraderStock(int32 Level);
    static FVector2D DoorPosition(int32 I) { return FVector2D(340+I*300,170); }
    static FVector2D ChestPosition(int32 I) { return FVector2D(390+I*250,440); }
private:
    bool bAtlasActive=false,bAtlasMap=false;
    TArray<FAtlasRoom> AtlasRooms;
    int32 AtlasCurrent=0,AtlasTravelDoor=0,AtlasChapter=0;
    int32 AtlasPreviousSafe=0;
    float AtlasTravelTime=0,AtlasArrivalTime=0;
    bool bAtlasDescending=false;
    FVector2D AtlasTravelFrom,AtlasArrivalFrom,AtlasArrivalTo;
    bool bFreedomResolved=false;
    float BossIntroTime=-1;
    UPROPERTY() TObjectPtr<UAudioComponent> IntroAudio;
    TArray<FString> DialogueLines;
    int32 DialogueIndex=0;
    float DialogueWait=0,BossGrace=0;
    void SpawnWave();
    void SpawnOneEnemy();
    void NextRoom();
    void VerifyGameplay();
    void VerifyCampaign();
    UPROPERTY() TArray<TObjectPtr<ADungeonEnemy>> Enemies;
    UPROPERTY() TMap<FString,TObjectPtr<USoundBase>> Sounds;
    UPROPERTY() TObjectPtr<UAudioComponent> MusicComponent;
    TMap<FString,double> LastSoundTime;
    TMap<FString,int32> LastFoleyVariant;
    FString MusicName;
    bool bMusicMuted=false,bEffectsMuted=false;
    float MasterVolume=1.f;
    int32 Room=1,Wave=1,PendingSpawns=0;
    int32 RosterCursor=0;
    float SpawnTimer=0,LootTimer=0,TransitionCooldown=0;
    bool bChest=false,bLootClaimed=false,bLootRolled=false;
    FDungeonItem Loot;
    TArray<FDungeonBreakable> Breakables;
    FRewardPresentation Reward;
    FDungeonItem ChestLoot[3];
    bool ChestRolled[3]={false,false,false};
    bool bMenu=true,bHasRun=false;
    bool bTraderOpen=false,bTraderVisited=false;
    TArray<FDungeonItem> TraderStock;
    int32 EndState=0;
    float EndTime=0;
    bool bEndingCleaned=false;
    int32 DeathReturnRoom=INDEX_NONE;
    float TransitionTime=0;
    int32 TransitionDoor=1;
    FVector2D TransitionFrom;
    TArray<FDungeonShot> Shots;
    TArray<FDungeonSplash> Splashes;
    TArray<FDungeonImpact> Impacts;
    TArray<FDungeonPotion> Potions;
    TArray<FDungeonCoinDrop> CoinDrops;
};
UCLASS()
class THEBEARDANDBLADE_API ADungeonHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    void InventoryClick();
    void TraderClick();
    int32 VerifyTraderUI();
    void DrawTrader(ADungeonGameMode* G,ADungeonHero* H);
    void DrawAtlas(ADungeonGameMode* G,ADungeonHero* H);
    void DrawAtlasDoors(ADungeonGameMode* G,ADungeonHero* H);
    int32 TraderSelection=INDEX_NONE;
    void CancelInventoryGesture();
    int32 VerifyInventoryGestures(ADungeonHero* H);
    void DialogueClick();
    void EndingClick();
    void PreloadBossIntro(int32 Species);
private:
    void UpdateFlashScreen();
    TSharedPtr<SBackgroundBlur> FlashBlur;
    TSharedPtr<SBorder> FlashWhite;
    void DrawDialogue(ADungeonGameMode* G,ADungeonHero* H);
    void DrawBossIntro(ADungeonGameMode* G);
    void DrawReward(ADungeonGameMode* G,ADungeonHero* H);
    void DrawBreakables(ADungeonGameMode* G,ADungeonHero* H);
    void DrawBossUI(ADungeonEnemy* E);
    void DrawStatus(FVector2D P,float Stun,float Slow,float Poison,float Bleed,bool Immune,float Clock);
    void DrawCombatFX(ADungeonGameMode* G,bool Foreground);
    void SpeechBubble(FVector2D Position,FVector2D Size,FVector2D Speaker);
    void DrawVitals(ADungeonHero* H);
    void DrawIronMatriarch(ADungeonEnemy* E);
    void IronSprite(const FString& Name,int Frame,int Frames,FVector2D Origin,FVector2D Size,FLinearColor Tint=FLinearColor::White,float Angle=0,FVector2D Pivot=FVector2D(.5,.5));
    void DrawGuard(ADungeonHero* H);
    void DrawTeaSpiritAura(ADungeonHero* H,FVector2D Position,float HeroScale,float Opacity);
    void MeterArc(FVector2D Center,float Radius,float Start,float Sweep,float Fraction,FLinearColor Color,float Width);
    void GemMeter(FVector2D Center,float Radius,float Width,float Start,float Sweep,float Fraction,FLinearColor Tint,int Pieces);
    float DisplayHealth=-1,DisplayStamina=-1;
    void DrawPotions(ADungeonGameMode* G);
    void DrawCoinDrops(ADungeonGameMode* G);
    void Orb(FVector2D Center,float Fraction,FLinearColor Color);
    void DrawInventory(ADungeonHero* H);
    void UpdateInventoryDrag(ADungeonHero* H);
    void DrawInventoryDrag(ADungeonHero* H);
    void DrawInventoryTurntable();
    void RotateInventoryPortrait(float DeltaX);
    bool bPortraitDragging=false;
    float PortraitYaw=22.5f,PortraitLastX=0;
    int32 DragItem=INDEX_NONE,LastClickedItem=INDEX_NONE;
    int32 DragGear=INDEX_NONE,LastClickedGear=INDEX_NONE;
    bool bDragging=false;
    double LastClickTime=-1;
    FVector2D DragStart,DragGrab,DragMouse,LastClickPosition;
    void DrawMenu(ADungeonGameMode* G);
public:
    void DrawSettings(ADungeonGameMode* G);
    void SettingsClick(ADungeonGameMode* G,FVector2D P);
    void OpenSettings();
    void CloseSettings();
    void SettingsTick();
    int SettingsTab=0,CaptureBinding=-1,AudioDragging=-1;
    double CaptureReady=0,DisplayDeadline=0;
    FString SettingsNotice;
    TArray<FIntPoint> DisplaySizes;
    int DraftResolution=0,DraftMode=1,DraftQuality=2,DraftFPS=0;
    bool DraftVSync=false;
    FIntPoint OldResolution;
    int OldMode=1,OldQuality=2;
    float OldFPS=0;
    bool OldVSync=false;
    TSharedPtr<Scalability::FQualityLevels> OldScalability;
    void RevertDisplay();
private:
    bool bVolumeDragging=false;
    TMap<FString,float> MenuHoverAmounts;
    FString HoveredMenuButton;
    void DrawSwordCursor();
    void DrawEnding(ADungeonGameMode* G);
    void DrawDeathSequence(ADungeonGameMode* G,ADungeonHero* H);
    UTexture2D* Texture(const FString& Name);
    void Sprite(const FString& Name,float X,float Y,float W,float H,FLinearColor Tint=FLinearColor::White,float Rotation=0,FVector2D Pivot=FVector2D(.5,.5));
    void KeySprite(const FString& Name,float X,float Y,float W,float H,FLinearColor Tint,bool Flip=false,float Rotation=0);
    void Box(float X,float Y,float W,float H,FLinearColor Color);
    void Label(const FString& Text,float X,float Y,FLinearColor Color,float Size=1);
    void ItemLettering(const FString& Text,float X,float Y,FLinearColor Color,float Height=16,float MaxWidth=360);
    void DrawItemCard(const FDungeonItem& Item,const ADungeonHero* CompareHero=nullptr,bool TraderComparison=false);
    void DrawTraderComparison(const FDungeonItem& Offered,const FDungeonItem& Equipped);
    void DrawAdventurerStats(const ADungeonHero* Hero);
    float CardText(const FString& Text,float X,float Y,FLinearColor Color,float Height,float MaxWidth,float MaxHeight=48,bool Centered=false);
    UPROPERTY() UFont* CardTypeface=nullptr;
    void Ring(FVector2D Center,float Radius,FLinearColor Color,float Width=2);
    void Shadow(FVector2D Center,float Radius,float Opacity=1.f);
    void Hero(ADungeonHero* H,float HS=1.375f);
    void Enemy(ADungeonEnemy* E);
    void Rustblade(ADungeonEnemy* E);
    void ExpansionEnemy(ADungeonEnemy* E);
    void ExpansionReview(int32 Species,float Age);
    float Scale=1;
    FVector2D Offset=FVector2D::ZeroVector;
    UPROPERTY() TMap<FString,TObjectPtr<UTexture2D>> Textures;
    UPROPERTY() TMap<FString,TObjectPtr<UMaterialInstanceDynamic>> ArmorMaterials;
};
