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
        auto& C=CoinDrops[I];
        const auto Target=DungeonView::Project(H->GetActorLocation())-FVector2D(0,30);
        if(C.Collected){C.Position=Target;C.CollectedAge+=Dt;if(C.CollectedAge>=.5f)CoinDrops.RemoveAt(I);continue;}
        const float Before=C.Age;C.Age+=FMath::Max(0.f,Dt);
        const float FlightDt=FMath::Max(0.f,C.Age-.22f)-FMath::Max(0.f,Before-.22f);
        if(FlightDt<=0||C.Amount<=0||H->Coins>MAX_int64-C.Amount)continue;
        const auto Delta=Target-C.Position;
        const float Distance=Delta.Size(),Speed=FMath::Min(1700.f,650.f+FMath::Max(0.f,C.Age-.22f)*1050.f);
        C.Position+=Delta.GetSafeNormal()*FMath::Min(Distance,Speed*FlightDt);
        if(FVector2D::Distance(C.Position,Target)<=8) {
            H->Coins+=C.Amount;C.Collected=true;C.CollectedAge=0;C.Position=Target;
            PlaySound(TEXT("CoinPickup"),.65f);
        }
    }
}
void ADungeonHUD::DrawCoinDrops(ADungeonGameMode* G,bool Foreground)
{
    const float Time=GetWorld()->GetTimeSeconds();
    for(const auto& C:G->GetCoinDrops()) {
        if(C.Collected!=Foreground)continue;
        if(C.Collected){
            const float T=C.CollectedAge/.5f,Fade=1-T;const auto Center=C.Position-FVector2D(0,12);
            SoftEllipse(Center,FVector2D(25+T*12,25+T*12),FLinearColor(1,.72f,.16f,Fade*.3f));
            for(int J=0;J<8;++J){const float A=J*2*PI/8;const auto P=Center+FVector2D(FMath::Cos(A),FMath::Sin(A))*(7+T*30);
                SoftEllipse(P,FVector2D(2,3),FLinearColor(1,.86f,.28f,Fade));}
            CardText(FString::Printf(TEXT("+%d"),C.Amount),Center.X-30,Center.Y-35-T*25,FLinearColor(1,.83f,.3f,Fade),14,60,20,true);
            continue;
        }
        const auto P=C.Position;const float Phase=Time*3.5f+P.X*.05f;
        const float Face=FMath::Abs(FMath::Cos(Phase)),W=FMath::Lerp(3.f,28.f,Face);
        if(C.Age<=.22f)Shadow(P,13);
        else SoftEllipse(P-FVector2D(0,12),FVector2D(16,20),FLinearColor(1,.66f,.14f,.18f));
        const float Y=P.Y-26+FMath::Sin(Phase*.6f)*2;
        Sprite(TEXT("CurrencyCoin"),P.X-W*.5f,Y,W,28,FLinearColor(1, .78f+.22f*Face, .45f+.55f*Face));
        const float Shine=FMath::Pow(FMath::Max(0.f,FMath::Sin(Phase)),12.f);
        if(Shine>.05f) {
            const FLinearColor Gold(1,.95f,.64f,Shine);
            Box(P.X+7,Y+3,1,9,Gold);Box(P.X+3,Y+7,9,1,Gold);
        }
        if(C.Age<=.22f)CardText(FString::Printf(TEXT("%d"),C.Amount),P.X-25,P.Y+7,FLinearColor(1,.78f,.3f),13,50,18,true);
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
    // Vacuum follows a moving player from anywhere in the chamber.
    CoinDrops.Empty();C.Position=FVector2D(100,190);C.Age=0;CoinDrops.Add(C);
    H->SetActorLocation(DungeonView::Unproject(FVector2D(1100,650)));
    const int64 VacuumBefore=H->Coins;
    UpdateCoinDrops(.3f);Check(CoinDrops[0].Position.X>100&&H->Coins==VacuumBefore,TEXT("Far coin animates before credit"));
    const auto PausedPosition=CoinDrops[0].Position;H->ToggleInventory();UpdateCoinDrops(1);
    Check(CoinDrops[0].Position.Equals(PausedPosition)&&H->Coins==VacuumBefore,TEXT("Inventory pauses vacuum"));H->ToggleInventory();
    for(int I=0;I<180;++I){H->SetActorLocation(DungeonView::Unproject(FVector2D(1000-I,600)));UpdateCoinDrops(1.f/60);}
    Check(H->Coins==VacuumBefore+21&&CoinDrops.IsEmpty(),TEXT("Room-wide moving-target vacuum credits once"));
    H->Coins=MAX_int64;CoinDrops.Add(C);UpdateCoinDrops(2);
    Check(H->Coins==MAX_int64&&CoinDrops.Num()==1&&!CoinDrops[0].Collected,TEXT("Full wallet preserves uncredited drop"));
    H->Coins=VacuumBefore+21;CoinDrops.Empty();
    Check(LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/CoinPickup.CoinPickup"))!=nullptr,TEXT("Dedicated coin recording present"));
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
