#include "DungeonActors.h"
#include "DungeonCombatBalance.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/PlatformMisc.h"

void ADungeonGameMode::SpawnBreakables()
{
    Breakables.Empty();
    // Side-wall lanes: away from the three north exits, center chests and south HUD.
    // No collision volume is added, so props cannot trap the player or enemies.
    TArray<FVector2D> Sites;
    for(int Side=0;Side<2;++Side)for(int Row=0;Row<4;++Row)
        Sites.Add(FVector2D(Side?1125:155,bAtlasActive?(Row<2?275+Row*60:585+(Row-2)*60):290+Row*100));
    for(int I=Sites.Num()-1;I>0;--I)Sites.Swap(I,FMath::RandRange(0,I));
    const int Count=FMath::RandRange(2,3);
    for(int I=0;I<Count;++I) {
        FDungeonBreakable B;B.Position=Sites[I]+FVector2D(FMath::FRandRange(-12.f,12.f),FMath::FRandRange(-10.f,10.f));
        B.Variant=I%3;Breakables.Add(B);
    }
}
void ADungeonGameMode::StrikeBreakables(ADungeonHero* H)
{
    if(!H||H->Health<=0||IsGameplayBlocked()||H->IsInventoryOpen())return;
    const auto P=DungeonView::Project(H->GetActorLocation());
    for(auto& B:Breakables)if(B.BrokenAge<0&&DungeonCombatBalance::MeleeHits(B.Position-P,H->GetAim(),80,false)) {
        B.BrokenAge=0;
        // Once-only roll, ordinary rarity odds even in a boss room; no kill/proc rewards.
        if(DungeonCombatBalance::PropDropsLoot(FMath::FRand())){
            B.Loot=RollChestLoot(false,LootLevel());
            BalanceEvent(TEXT("prop_offer"),B.Loot.Rarity,B.Loot.CatalogId);
        }
        PlaySound(TEXT("PropBreak"),.6f,FMath::FRandRange(.97f,1.03f));
    }
}
bool ADungeonGameMode::CollectBreakableLoot(ADungeonHero* H)
{
    if(!H||H->Health<=0||IsGameplayBlocked()||H->IsInventoryOpen())return false;
    FDungeonBreakable* Nearest=nullptr;float Distance=85;
    const auto P=DungeonView::Project(H->GetActorLocation());
    for(auto& B:Breakables)if(B.BrokenAge>=.45f&&!B.Collected&&!B.Loot.IsEmpty()) {
        const float D=FVector2D::Distance(B.Position,P);
        if(D<Distance){Distance=D;Nearest=&B;}
    }
    if(!Nearest)return false;
    if(H->AddToInventory(Nearest->Loot)) {Nearest->Collected=true;Nearest->BagFull=false;PlaySound(TEXT("UI"));}
    else Nearest->BagFull=true;
    return true;
}
void ADungeonHUD::DrawBreakables(ADungeonGameMode* G,ADungeonHero* H)
{
    auto* Atlas=Texture(TEXT("DestructiblesAtlas"));if(!Atlas)return;
    // Source sheet rows follow campaign order, whereas biome IDs predate that order.
    const int Rows[]={0,3,4,5,6,2,1,5};const int Row=Rows[G->GetBiome()];
    // Measured gutters prevent neighboring row pixels appearing under a prop.
    const float Tops[]={2,195,377,567,761,958,1143};
    const float Heights[]={189,180,189,193,197,185,197};
    const auto Hero=DungeonView::Project(H->GetActorLocation());
    for(const auto& B:G->GetBreakables()) {
        const auto P=B.Position;
        auto Piece=[&](float X,float Y,float W,float HH,float U,float V,float UW,float VH,float Alpha,float Angle) {
            DrawTexture(Atlas,Offset.X+X*Scale,Offset.Y+Y*Scale,W*Scale,HH*Scale,
                ((B.Variant==0?195.f:B.Variant==1?475.f:755.f)+U*215)/1161.f,(Tops[Row]+V*Heights[Row])/1354.f,UW*215/1161.f,VH*Heights[Row]/1354.f,FLinearColor(1,1,1,Alpha),BLEND_Translucent,1,false,Angle);
        };
        if(B.BrokenAge<0) {Shadow(P,24);Piece(P.X-55,P.Y-80,110,96,0,0,1,1,1,0);}
        else {
            if(B.BrokenAge<1.35f) {
                const float T=B.BrokenAge,Travel=FMath::Min(T/.6f,1.f),Fade=1-FMath::Clamp((T-.65f)/.7f,0.f,1.f);
                // Sixteen textured fragments burst outward, fall, settle, then fade.
                for(int Y=0;Y<4;++Y)for(int X=0;X<4;++X) {
                    const float DX=(X-1.5f)*15*Travel;
                    const float DY=(Y-1.5f)*4*Travel-FMath::Sin(Travel*PI)*(16+(X+Y)%3*7)+Y*7*Travel;
                    Piece(P.X-55+X*27.5f+DX,P.Y-80+Y*24+DY+40*Travel,27.5f,24,X*.25f,Y*.25f,.25f,.25f,Fade,(X-Y)*22*Travel);
                }
            }
            if(B.BrokenAge>=.45f&&!B.Collected&&!B.Loot.IsEmpty()) {
                const FLinearColor Colors[]={FLinearColor(.65f,.69f,.72f),FLinearColor(.3f,.88f,.53f),FLinearColor(.3f,.62f,1),FLinearColor(.8f,.38f,1),FLinearColor(1,.6f,.16f)};
                const auto C=Colors[FMath::Clamp(B.Loot.Rarity,0,4)];
                Shadow(P,17);Sprite(B.Loot.Art(),P.X-24,P.Y-44,48,48);
                Box(P.X-16,P.Y+6,32,2,C);
            }
        }
    }
}
void ADungeonGameMode::VerifyBreakables()
{
#if !UE_BUILD_SHIPPING
    int Errors=0;auto Check=[&](bool OK,const TCHAR* Message){if(!OK){++Errors;UE_LOG(LogTemp,Error,TEXT("BREAKABLES: %s"),Message);}};
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));
    if(!H){FPlatformMisc::RequestExitWithStatus(false,1);return;}
    StartGame();
    Check(DungeonCombatBalance::PropDropsLoot(0.f)&&DungeonCombatBalance::PropDropsLoot(.499f)&&!DungeonCombatBalance::PropDropsLoot(.5f)&&!DungeonCombatBalance::PropDropsLoot(.999f),TEXT("50 percent drop threshold"));
    for(int D=0;D<8;++D){const FVector2D Aim(FMath::Cos(D*PI/4),FMath::Sin(D*PI/4));Check(DungeonCombatBalance::MeleeHits(Aim*65,Aim,80,false),TEXT("Eight-direction prop reach"));Check(!DungeonCombatBalance::MeleeHits(Aim*200,Aim,80,false),TEXT("Distant props remain intact"));}
    for(int Chapter=0;Chapter<7;++Chapter) {
        Room=Chapter*DungeonProgression::RoomsPerChapter+1;Wave=1;SpawnBreakables();
        Check(Breakables.Num()>=2&&Breakables.Num()<=3,TEXT("Halved random prop count"));
        for(const auto& B:Breakables) {
            Check(B.Position.X<180||B.Position.X>1100,TEXT("Wall lane placement"));
            for(int D=0;D<3;++D)Check(FVector2D::Distance(B.Position,DoorPosition(D))>150,TEXT("Door clearance"));
        }
        auto& B=Breakables[0];H->Inventory.Empty();
        H->SetActorLocation(DungeonView::Unproject(B.Position-H->GetAim()*65));
        StrikeBreakables(H);Check(B.BrokenAge==0,TEXT("Melee breaks prop regardless of loot roll"));
        Check(!AreDoorsOpen(),TEXT("Breaking props never unlocks room exits"));
        const auto Rolled=B.Loot;StrikeBreakables(H);Check(B.Loot.Name==Rolled.Name&&B.Loot.Stats()==Rolled.Stats(),TEXT("No reroll"));
        // Explicit fixtures cover both outcomes without probabilistic test failures.
        B.Loot=FDungeonItem();B.BrokenAge=2;H->SetActorLocation(DungeonView::Unproject(B.Position));
        StrikeBreakables(H);
        Check(B.Loot.IsEmpty()&&!CollectBreakableLoot(H)&&H->Inventory.IsEmpty()&&!B.BagFull,TEXT("Empty prop cannot reroll, collect or block interaction"));
        B.Loot=RollItem(36,2,Room);const auto Saved=B.Loot;
        B.BrokenAge=2;H->SetActorLocation(DungeonView::Unproject(B.Position));
        for(int I=0;I<36;++I)H->AddToInventory(RollItem(36,0));
        CollectBreakableLoot(H);Check(B.BagFull&&!B.Collected,TEXT("Full bag preserves loot"));
        H->Inventory.Empty();CollectBreakableLoot(H);CollectBreakableLoot(H);
        Check(B.Collected&&H->Inventory.Num()==1&&H->Inventory[0].Item.Stats()==Saved.Stats(),TEXT("Exact loot picked up once"));
        Wave=2;SpawnWave();Check(Breakables[0].Collected,TEXT("Wave two preserves props"));
        Wave=1;SpawnWave();Check(Breakables[0].BrokenAge<0,TEXT("New room resets props"));
    }
    UE_LOG(LogTemp,Display,TEXT("BREAKABLES_VERIFY_COMPLETE errors=%d"),Errors);
    FPlatformMisc::RequestExitWithStatus(false,Errors?1:0);
#endif
}
