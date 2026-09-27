#include "DungeonActors.h"
#include "DungeonInventoryLayout.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformMisc.h"
#include "Engine/World.h"

bool ADungeonHero::DiscardForCoins(int32 Index)
{
    if(!Inventory.IsValidIndex(Index))return false;
    const int64 Value=FMath::Max(0,Inventory[Index].Item.CoinValue);
    if(Coins>MAX_int64-Value)return false;
    Coins+=Value;Inventory.RemoveAt(Index);SelectedItem=INDEX_NONE;InventoryMessage.Empty();return true;
}
void ADungeonGameMode::OpenTrader()
{
    if(bTraderOpen||bTraderVisited)return;
    bTraderOpen=bTraderVisited=true;TraderMessage.Empty();TraderStock.Empty();
    const int Count=FMath::RandRange(3,5);
    TSet<int32> Definitions;
    while(TraderStock.Num()<Count) {
        const int ID=FMath::RandRange(0,59);if(Definitions.Contains(ID))continue;
        Definitions.Add(ID);auto Item=RollChestLoot(false,Room+1);
        TraderStock.Add(RollItem(ID,Item.Rarity,Room+1));
    }
    if(auto* PC=GetWorld()->GetFirstPlayerController())if(auto* HUD=Cast<ADungeonHUD>(PC->GetHUD())) {HUD->TraderSelection=INDEX_NONE;HUD->CancelInventoryGesture();}
    PlaySound(TEXT("UI"));
}
void ADungeonGameMode::ContinueFromTrader()
{
    if(!bTraderOpen)return;
    if(auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0)))if(H->IsInventoryOpen())H->ToggleInventory();
    bTraderOpen=false;TraderStock.Empty();TraderMessage.Empty();NextRoom();
}
bool ADungeonGameMode::BuyTraderItem(ADungeonHero* H,int32 Index)
{
    if(!bTraderOpen||!H||!TraderStock.IsValidIndex(Index)||TraderStock[Index].IsEmpty())return false;
    const auto Item=TraderStock[Index];const int64 Price=TraderPrice(Item);
    if(H->Coins<Price){TraderMessage=TEXT("Not enough coins.");return false;}
    // Insert before charging, so a full bag never spends coins or loses stock.
    if(!H->AddToInventory(Item)){TraderMessage=TEXT("Your satchel is full. Manage your inventory first.");return false;}
    H->Coins-=Price;TraderStock[Index]=FDungeonItem();TraderMessage=TEXT("Purchased — added to your satchel.");PlaySound(TEXT("UI"));return true;
}
void ADungeonHUD::DrawTrader(ADungeonGameMode* G,ADungeonHero* H)
{
    const FLinearColor Gold(.94f,.69f,.30f),Pale(.85f,.9f,.86f);
    Sprite(TEXT("InventoryFrame"),10,40,1260,720);
    Sprite(TEXT("InventoryCard"),24,102,400,594);
    CardText(TEXT("THE TRADER"),92,146,Gold,28,270);
    Sprite(TEXT("TraderPortrait"),74,185,300,430);
    CardText(TEXT("A fair trade before the descent."),78,625,Pale,17,292);
    CardText(FString::Printf(TEXT("COINS  %lld"),H->Coins),456,117,Gold,25,360);
    CardText(TEXT("Supplies for the road ahead"),456,151,Pale,18,360);
    float X=0,Y=0;FVector2D Mouse(-1,-1);
    if(auto* PC=GetOwningPlayerController())if(PC->GetMousePosition(X,Y)&&Scale>0)Mouse=(FVector2D(X,Y)-Offset)/Scale;
    int Inspect=TraderSelection;
    const auto& Stock=G->GetTraderStock();
    for(int I=0;I<Stock.Num();++I) {
        const float Top=190+I*82;const auto& Item=Stock[I];
        Sprite(TEXT("InventoryFrame"),450,Top,374,76);
        if(Item.IsEmpty()){CardText(TEXT("SOLD"),480,Top+25,Gold,21,300);continue;}
        Sprite(Item.Art(),463,Top+8,58,58);
        CardText(Item.Name,530,Top+10,Pale,18,165,42);
        const int64 Price=G->TraderPrice(Item);
        Sprite(TEXT("InventoryFrame"),704,Top+12,109,52);
        CardText(TEXT("BUY"),725,Top+16,Gold,18,80,20);
        CardText(FString::Printf(TEXT("%lld coins"),Price),715,Top+39,H->Coins>=Price?Pale:FLinearColor(.9f,.4f,.3f),16,96,20);
        if(DungeonInventoryLayout::In(Mouse,450,Top,374,76))Inspect=I;
    }
    if(Stock.IsValidIndex(Inspect)&&!Stock[Inspect].IsEmpty())DrawItemCard(Stock[Inspect],H);
    else Sprite(TEXT("InventoryCard"),842,104,400,628);
    CardText(G->TraderMessage,456,607,Pale,18,360,40);
    Sprite(TEXT("InventoryFrame"),456,653,174,42);CardText(TEXT("INVENTORY"),478,663,Gold,18,145);
    Sprite(TEXT("InventoryFrame"),642,653,174,42);CardText(FString::Printf(TEXT("ENTER ROOM %d"),G->GetRoom()+1),661,663,Gold,18,145);
}
void ADungeonHUD::TraderClick()
{
    auto* G=Cast<ADungeonGameMode>(UGameplayStatics::GetGameMode(this));
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));auto* PC=GetOwningPlayerController();
    float X=0,Y=0;if(!G||!G->IsTraderOpen()||!H||!PC||Scale<=0||!PC->GetMousePosition(X,Y))return;
    const auto P=(FVector2D(X,Y)-Offset)/Scale;
    using namespace DungeonInventoryLayout;
    if(In(P,456,653,174,42)){H->ToggleInventory();return;}
    if(In(P,642,653,174,42)){G->ContinueFromTrader();return;}
    for(int I=0;I<G->GetTraderStock().Num();++I) {
        const float Top=190+I*82;
        if(In(P,450,Top,374,76)){TraderSelection=I;if(In(P,704,Top+12,109,52))G->BuyTraderItem(H,I);return;}
    }
}
int32 ADungeonHUD::VerifyTraderUI()
{
    auto* G=Cast<ADungeonGameMode>(UGameplayStatics::GetGameMode(this));
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));auto* PC=GetOwningPlayerController();
    if(!G||!G->IsTraderOpen()||!H||!PC)return 1;
    int Errors=0;auto Check=[&](bool OK){if(!OK)++Errors;};
    auto Click=[&](float X,float Y){PC->SetMouseLocation(FMath::RoundToInt(Offset.X+X*Scale),FMath::RoundToInt(Offset.Y+Y*Scale));H->Attack();};
    const int64 Price=G->TraderPrice(G->GetTraderStock()[0]),Before=H->Coins;
    const int Count=H->Inventory.Num();Click(748,218);
    Check(H->Coins==Before-Price&&H->Inventory.Num()==Count+1);Click(748,218);Check(H->Coins==Before-Price);
    Click(510,671);Check(H->IsInventoryOpen());
    H->SelectedItem=H->Inventory.Num()-1;const int64 Value=H->Inventory[H->SelectedItem].Item.CoinValue;
    Click(720,582);Check(H->Coins==Before-Price+Value&&H->Inventory.Num()==Count);
    Click(1190,80);Check(!H->IsInventoryOpen()&&G->IsTraderOpen());
    const int Room=G->GetRoom();Click(720,671);Check(!G->IsTraderOpen()&&G->GetRoom()==Room+1);
    return Errors;
}
void ADungeonGameMode::VerifyTrader()
{
#if !UE_BUILD_SHIPPING
    int Errors=0;auto Check=[&](bool OK,const TCHAR* Why){if(!OK){++Errors;UE_LOG(LogTemp,Error,TEXT("TRADER_VERIFY: %s"),Why);}};
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));if(!H){FPlatformMisc::RequestExitWithStatus(false,1);return;}
    StartGame();H->Inventory.Empty();const auto Item=RollItem(48,2,3);H->AddToInventory(Item);
    Check(H->DiscardForCoins(0)&&H->Coins==Item.CoinValue,TEXT("Discard credits full value"));
    Check(!H->DiscardForCoins(0)&&H->Coins==Item.CoinValue,TEXT("No duplicate discard credit"));
    for(int Chapter=0;Chapter<7;++Chapter) {
        Room=Chapter*4+2;Wave=2;PendingSpawns=0;bLootRolled=true;bTraderVisited=false;NextRoom();
        Check(IsTraderOpen()&&Room==Chapter*4+2&&TraderStock.Num()>=3&&TraderStock.Num()<=5,TEXT("Shop halfway through each theme"));
        const auto Stock=TraderStock;OpenTrader();Check(Stock[0].Stats()==TraderStock[0].Stats(),TEXT("Opening shop cannot reroll stock"));
        const auto Offer=TraderStock[0];const int64 Price=TraderPrice(Offer);Check(Price>Offer.CoinValue,TEXT("Positive shop markup"));
        H->Coins=Price-1;Check(!BuyTraderItem(H,0)&&H->Coins==Price-1,TEXT("Unaffordable purchase leaves balance intact"));
        H->Coins=Price;H->Inventory.Empty();for(int I=0;I<36;++I)H->AddToInventory(RollItem(36,0));
        Check(!BuyTraderItem(H,0)&&H->Coins==Price&&!TraderStock[0].IsEmpty(),TEXT("Full bag preserves money and offer"));
        H->Inventory.Empty();Check(BuyTraderItem(H,0)&&H->Coins==0&&H->Inventory.Num()==1&&H->Inventory[0].Item.Stats()==Offer.Stats(),TEXT("Purchase exact offer"));
        Check(!BuyTraderItem(H,0)&&H->Inventory.Num()==1,TEXT("Sold item cannot be bought twice"));
        H->Coins=777;H->ToggleInventory();Check(H->IsInventoryOpen(),TEXT("Manage gear while shopping"));H->ToggleInventory();
        ContinueFromTrader();Check(Room==Chapter*4+3&&!IsTraderOpen()&&H->Coins==777,TEXT("Continue preserves wallet and advances once"));
        ContinueFromTrader();Check(Room==Chapter*4+3,TEXT("Continue cannot advance twice"));
        PendingSpawns=0;
    }
    RestartRun();Check(H->Coins==0&&!IsTraderOpen(),TEXT("New run resets wallet and shop"));
    UE_LOG(LogTemp,Display,TEXT("TRADER_VERIFY_COMPLETE errors=%d"),Errors);FPlatformMisc::RequestExitWithStatus(false,Errors?1:0);
#endif
}
