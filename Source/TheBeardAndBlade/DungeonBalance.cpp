#include "DungeonActors.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "DungeonRoster.h"

namespace {
void AppendReport(const FString& Run,const TCHAR* Kind,const FString& Header,const FString& Row)
{
    const FString Dir=FPaths::ProjectSavedDir()/TEXT("Balance");
    IFileManager::Get().MakeDirectory(*Dir,true);
    const FString Path=Dir/(Run+TEXT("-")+Kind+TEXT(".csv"));
    const bool Exists=IFileManager::Get().FileExists(*Path);
    FFileHelper::SaveStringToFile((Exists?FString():Header+TEXT("\n"))+Row+TEXT("\n"),
        *Path,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,&IFileManager::Get(),FILEWRITE_Append);
}
}
int ADungeonGameMode::BalanceFloor() const
{
    return bAtlasActive?AtlasChapter:DungeonProgression::Chapter(Room);
}
int ADungeonGameMode::BalanceDepth() const
{
    if(bAtlasActive&&AtlasRooms.IsValidIndex(AtlasCurrent)){
        // Optional combat branch has a little extra risk, not visit-based scaling.
        return FMath::Clamp(AtlasRooms[AtlasCurrent].Depth+(AtlasCurrent>=12?2:0),1,9);
    }
    return IsBossRoom()?9:1+(Room-1)%DungeonProgression::RoomsPerChapter;
}
int ADungeonGameMode::LootLevel() const
{
    return DungeonBalance::LootLevel(BalanceFloor(),BalanceDepth());
}
float ADungeonGameMode::IncomingDamageScale(bool Boss) const
{
    return DungeonBalance::IncomingScale(BalanceFloor(),BalanceDepth(),Boss);
}
bool ADungeonGameMode::PermitEnemyAttack(ADungeonEnemy* Enemy)
{
    if(!Enemy)return false;
    if(Enemy->bBoss)return true;
    const float Now=GetWorld()->GetTimeSeconds();
    if(Now<NextEnemyAttack)return false;
    int Active=0;
    for(const auto& E:Enemies)if(IsValid(E)&&E!=Enemy&&E->Health>0&&(E->Windup>0||E->ChargeTime>0))++Active;
    if(Active>=DungeonBalance::AttackSlots(BalanceFloor(),BalanceDepth()))return false;
    NextEnemyAttack=Now+DungeonBalance::AttackStartGap(BalanceFloor());
    if(Balance.Active)++Balance.Attacks;
    return true;
}
void ADungeonHero::RestoreHealth(float Amount,bool FromGear)
{
    if(Health<=0||Amount<=0)return;
    const float Missing=FMath::Max(0.f,MaxHealth-Health);
    const float Restored=FMath::Min(Missing,FromGear?FMath::Min(Amount,GearHealBudget):Amount);
    if(FromGear)GearHealBudget=FMath::Max(0.f,GearHealBudget-Restored);
    Health+=Restored;
    if(auto* G=Cast<ADungeonGameMode>(UGameplayStatics::GetGameMode(this)))
        if(G->Balance.Active)G->Balance.Healing+=Restored;
}
void ADungeonGameMode::BeginBalanceRoom()
{
    EndBalanceRoom(TEXT("replaced"));
    if(BalanceRun.IsEmpty())BalanceRun=FGuid::NewGuid().ToString(EGuidFormats::Digits);
    Balance=FBalanceReport();Balance.Active=true;Balance.Floor=BalanceFloor()+1;
    Balance.Depth=BalanceDepth();Balance.Node=bAtlasActive?AtlasCurrent:Room;
    NextEnemyAttack=0;
    if(auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0)))Balance.StartHealth=H->Health;
    BalanceEvent(TEXT("room_start"));
}
void ADungeonGameMode::EndBalanceRoom(const TCHAR* Outcome)
{
    if(!Balance.Active)return;
    Balance.Active=false;
    const auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));
    const float DPS=H?H->AttackPower*H->AttackSpeed/.48f*(1+H->CritChance*(H->CritMultiplier-1)):0;
    AppendReport(BalanceRun,TEXT("rooms"),
        TEXT("floor,depth,node,outcome,active_seconds,kills,regular_attack_starts,landed_contacts,blocked_contacts,invulnerable_contacts,player_melee_hits,damage_taken,healing,start_hp,end_hp,max_hp,sheet_dps,armor"),
        FString::Printf(TEXT("%d,%d,%d,%s,%.2f,%d,%d,%d,%d,%d,%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f"),
        Balance.Floor,Balance.Depth,Balance.Node,Outcome,Balance.Seconds,Balance.Kills,Balance.Attacks,
        Balance.Landed,Balance.Blocked,Balance.Avoided,Balance.MeleeHits,Balance.DamageTaken,Balance.Healing,
        Balance.StartHealth,H?H->Health:0,H?H->MaxHealth:0,DPS,H?H->Armor:0));
}
void ADungeonGameMode::BalanceEvent(const TCHAR* Event,float Value,int Species)
{
    if(BalanceRun.IsEmpty())return;
    const auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));
    const float DPS=H?H->AttackPower*H->AttackSpeed/.48f*(1+H->CritChance*(H->CritMultiplier-1)):0;
    AppendReport(BalanceRun,TEXT("events"),TEXT("floor,depth,node,event,value,item_or_species,sheet_dps,max_hp,armor"),
        FString::Printf(TEXT("%d,%d,%d,%s,%.3f,%d,%.2f,%.2f,%.2f"),BalanceFloor()+1,BalanceDepth(),
        bAtlasActive?AtlasCurrent:Room,Event,Value,Species,DPS,H?H->MaxHealth:0,H?H->Armor:0));
}

// Run with -BalanceVerify -game after the host's UE startup issue is resolved.
// Tests actual runtime equipment, spawning, damage and healing, not just projections.
void ADungeonGameMode::VerifyBalance()
{
#if WITH_EDITOR
    int Checks=0,Errors=0;
    auto Check=[&](bool OK,const TCHAR* Why){++Checks;if(!OK){++Errors;UE_LOG(LogTemp,Error,TEXT("BALANCE FAIL: %s"),Why);}};
    StartGame();
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));
    if(!H){FPlatformMisc::RequestExitWithStatus(false,1);return;}
    for(int F=0;F<8;++F){
        InitializeAtlasFloor(4201,F);
        EnterAtlasRoom(1,-1);SpawnOneEnemy();
        Check(BalanceFloor()==F&&BalanceDepth()==1,TEXT("floor and initial depth"));
        Check(Enemies.Num()==1,TEXT("one scheduled spawn"));
        if(!Enemies.IsEmpty()){
            auto* E=Enemies.Last().Get();
            Check(FMath::IsNearlyEqual(E->Health,DungeonBalance::EnemyHealth(DungeonRoster::BalanceRole(E->Species),F,1)),TEXT("runtime role HP"));
        }
        EnterAtlasRoom(11,-1);SpawnOneEnemy();
        Check(BalanceDepth()==8,TEXT("late combat uses depth eight, not clamped room three"));
        Check(LootLevel()==DungeonBalance::LootLevel(F,8),TEXT("loot level independent of legacy room counter"));
        const int Level=LootLevel();
        for(int R=0;R<5;++R)for(int ID=0;ID<DungeonLootCatalog::Count;++ID){
            const auto I=RollItem(ID,R,Level);
            Check(I.CoinValue>0&&I.ItemLevel==Level&&I.Slot>=0&&I.Slot<8,TEXT("all loot definitions retain valid identity and value"));
        }
        const auto Stock=CreateTraderStock(Level);
        Check(Stock.Num()>=3&&Stock.Num()<=5&&Stock[0].Rarity>=2,TEXT("trader has a featured choice"));
        for(const auto& I:Stock)Check(I.ItemLevel==FMath::Min(25,Level+1)&&TraderPrice(I)>TraderSellPrice(I),TEXT("shop progression and no buy/sell arbitrage"));
        // Equipped gear never influences the fixed encounter health target.
        const float FixedHP=Enemies.Last()->MaxHealth;
        for(int ID:{23,30,44,58,62,65,68,71})H->Equip(RollItem(ID,4,25));
        Check(Enemies.Last()->MaxHealth==FixedHP,TEXT("no rubber-band scaling to gear"));
    }
    StartGame();EnterAtlasRoom(1,-1);PendingSpawns=0;
    for(auto& E:Enemies)if(IsValid(E))E->Destroy();Enemies.Empty();
    H->Restart();H->Health=50;H->GearHealBudget=H->MaxHealth*DungeonBalance::GearHealingPerSecond;
    H->RestoreHealth(100,true);const float Healed=H->Health;
    H->RestoreHealth(100,true);
    Check(FMath::IsNearlyEqual(Healed,52.25f)&&H->Health==Healed,TEXT("all gear healing shares a single budget"));
    H->RestoreHealth(H->MaxHealth*DungeonBalance::PotionFraction);
    Check(FMath::IsNearlyEqual(H->Health,85.25f),TEXT("potions exempt from gear cap"));
    H->Health=0;H->RestoreHealth(100);
    Check(H->Health==0,TEXT("healing cannot resurrect"));
    H->Restart();H->Armor=85;H->ReceiveHit(40,true);
    Check(FMath::IsNearlyEqual(H->Health,130),TEXT("85 armor halves incoming damage, not flat subtraction"));
    H->Restart();H->Armor=0;H->SetFlashReviewAim({0,-1});H->BlockPressed();
    const auto P=DungeonView::Project(H->GetActorLocation());
    H->ReceiveHit(40,true,P+FVector2D(0,-60),true);
    Check(FMath::IsNearlyEqual(H->Health,120),TEXT("boss block preserves 75 percent damage"));
    H->BlockReleased();
    FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* E=GetWorld()->SpawnActor<ADungeonEnemy>(DungeonView::Unproject(P+FVector2D(0,-80)),FRotator::ZeroRotator,Params);
    E->Species=49;E->SpawnTime=0;E->Health=E->MaxHealth=1000;Enemies.Add(E);
    E->TakeDungeonDamage(1);E->RustHurtAge=-1;E->RustAttackAge=.1f;E->Windup=.4f;E->Recovery=0;
    E->TakeDungeonDamage(1);
    Check(E->RustAttackAge==.1f&&E->Windup==.4f,TEXT("repeated hits do not repeatedly cancel attack"));
    E->StaggerGuard=0;E->TakeDungeonDamage(1);
    Check(E->RustAttackAge<0&&E->StaggerGuard>0,TEXT("interrupt available after stagger guard expires"));
    E->Windup=0;E->Recovery=0;NextEnemyAttack=0;
    Check(PermitEnemyAttack(E)&&!PermitEnemyAttack(E),TEXT("same-frame attacks are serialized"));
    const int Node=AtlasCurrent;AtlasRooms[Node].Cleared=true;SaveAtlasRoom();
    EnterAtlasRoom(0,-1);EnterAtlasRoom(Node,-1);
    Check(Enemies.IsEmpty()&&PendingSpawns==0,TEXT("backtracking cannot farm respawns"));
    EndBalanceRoom(TEXT("test_end"));
    UE_LOG(LogTemp,Display,TEXT("BALANCE_VERIFY checks=%d errors=%d"),Checks,Errors);
    FPlatformMisc::RequestExitWithStatus(false,Errors?1:0);
#endif
}
