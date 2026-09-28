#include "DungeonActors.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/PlatformMisc.h"

bool ADungeonGameMode::DropInventoryItem(ADungeonHero* H,const FDungeonItem& Item)
{
    if(!H||Item.IsEmpty())return false;
    FDungeonBreakable Drop;Drop.BrokenAge=2;Drop.Loot=Item;
    Drop.Position=DungeonView::Clamp(DungeonView::Project(H->GetActorLocation())+FVector2D(38,15));
    // The shop is not a walkable room: leave dropped gear by its parent doorway.
    if(bTraderOpen&&bAtlasActive) {
        for(int D=0;D<4;++D) {
            const int Parent=AtlasRooms[AtlasCurrent].Links[D];if(Parent<0)continue;
            Drop.Position=AtlasDoor((D+2)%4)-AtlasDirection((D+2)%4)*95;
            AtlasRooms[Parent].Props.Add(Drop);return true;
        }
        return false;
    }
    Breakables.Add(Drop);return true;
}
void ADungeonGameMode::UpdateCoinDrops(float Dt)
{
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));
    if(!H||H->Health<=0||H->IsInventoryOpen()||IsGameplayBlocked())return;
    for(int I=CoinDrops.Num()-1;I>=0;--I) {
        auto& C=CoinDrops[I];C.Age+=Dt;
        if(C.Amount>0&&C.Age>.6f&&FVector2D::Distance(C.Position,DungeonView::Project(H->GetActorLocation()))<38&&H->Coins<=MAX_int64-C.Amount) {
            H->Coins+=C.Amount;CoinDrops.RemoveAt(I);PlaySound(TEXT("Equip"),.35f,1.15f);
        }
    }
}
void ADungeonHUD::DrawCoinDrops(ADungeonGameMode* G)
{
    const float Time=GetWorld()->GetTimeSeconds();
    for(const auto& C:G->GetCoinDrops()) {
        const auto P=C.Position;const float Phase=Time*3.5f+P.X*.05f;
        const float Face=FMath::Abs(FMath::Cos(Phase)),W=FMath::Lerp(3.f,28.f,Face);
        Shadow(P,13);
        const float Y=P.Y-26+FMath::Sin(Phase*.6f)*2;
        Sprite(TEXT("CurrencyCoin"),P.X-W*.5f,Y,W,28,FLinearColor(1, .78f+.22f*Face, .45f+.55f*Face));
        const float Shine=FMath::Pow(FMath::Max(0.f,FMath::Sin(Phase)),12.f);
        if(Shine>.05f) {
            const FLinearColor Gold(1,.95f,.64f,Shine);
            Box(P.X+7,Y+3,1,9,Gold);Box(P.X+3,Y+7,9,1,Gold);
        }
        CardText(FString::Printf(TEXT("%d"),C.Amount),P.X-25,P.Y+7,FLinearColor(1,.78f,.3f),13,50,18,true);
    }
}
void ADungeonGameMode::VerifyEconomy()
{
    int Errors=0;auto Check=[&](bool OK,const TCHAR* Why){if(!OK){++Errors;UE_LOG(LogTemp,Error,TEXT("ECONOMY %s"),Why);}};
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));if(!H){FPlatformMisc::RequestExitWithStatus(false,1);return;}
    StartGame();InitializeAtlasFloor(12);H->Inventory.Empty();H->Coins=0;
    const auto Item=RollItem(48,2,3);H->AddToInventory(Item);
    Check(!SellTraderItem(H,0)&&H->Inventory.Num()==1,TEXT("Cannot sell outside shop"));
    Check(H->DiscardItem(0)&&H->Coins==0&&H->Inventory.IsEmpty(),TEXT("Drop never credits coins"));
    Check(!H->DiscardItem(0),TEXT("Drop once only"));
    const auto P=Breakables.Last().Position;SaveAtlasRoom();EnterAtlasRoom(1,-1);EnterAtlasRoom(0,-1);
    Check(Breakables.Last().Loot.Stats()==Item.Stats()&&Breakables.Last().Loot.CoinValue==Item.CoinValue,TEXT("Dropped exact gear persists"));
    TransitionCooldown=0;H->SetActorLocation(DungeonView::Unproject(P));
    Check(CollectBreakableLoot(H)&&H->Inventory.Num()==1,TEXT("Dropped gear recoverable"));
    FDungeonCoinDrop C;C.Position=P;C.Amount=21;CoinDrops.Add(C);
    UpdateCoinDrops(.2f);Check(H->Coins==0,TEXT("Coin pickup waits for visibility"));
    UpdateCoinDrops(.5f);UpdateCoinDrops(1);Check(H->Coins==21&&CoinDrops.IsEmpty(),TEXT("Coin credits exactly once"));
    C.Position=FVector2D(300,300);CoinDrops.Add(C);SaveAtlasRoom();EnterAtlasRoom(1,-1);EnterAtlasRoom(0,-1);
    Check(CoinDrops.Num()==1&&CoinDrops[0].Amount==21,TEXT("Uncollected coins persist"));
    EnterAtlasRoom(6,-1);const int64 Before=H->Coins,Price=TraderSellPrice(Item);
    Check(Price<Item.CoinValue&&Price<TraderPrice(Item),TEXT("Sell discount and no buy/sell profit loop"));
    Check(SellTraderItem(H,0)&&H->Coins==Before+Price&&H->Inventory.IsEmpty(),TEXT("Sale credits exact discounted amount"));
    Check(!SellTraderItem(H,0)&&H->Coins==Before+Price,TEXT("No duplicate sale"));
    H->AddToInventory(Item);H->Coins=MAX_int64;
    Check(!SellTraderItem(H,0)&&H->Inventory.Num()==1,TEXT("Overflow keeps gear"));
    RestartRun();Check(H->Coins==0&&CoinDrops.IsEmpty(),TEXT("New run clears currency"));
    UE_LOG(LogTemp,Display,TEXT("ECONOMY_VERIFY_COMPLETE errors=%d"),Errors);
    FPlatformMisc::RequestExitWithStatus(false,Errors?1:0);
}
