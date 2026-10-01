#include "DungeonActors.h"
#include "DungeonInventoryLayout.h"
#include "DungeonItemComparison.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformMisc.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"

bool ADungeonHero::DiscardItem(int32 Index)
{
    if(!Inventory.IsValidIndex(Index))return false;
    auto* G=Cast<ADungeonGameMode>(UGameplayStatics::GetGameMode(this));
    if(!G||!G->DropInventoryItem(this,Inventory[Index].Item))return false;
    Inventory.RemoveAt(Index);SelectedItem=INDEX_NONE;InventoryMessage.Empty();return true;
}
bool ADungeonGameMode::SellTraderItem(ADungeonHero* H,int32 Index)
{
    if(!bTraderOpen||!H||!H->Inventory.IsValidIndex(Index))return false;
    const int64 Value=TraderSellPrice(H->Inventory[Index].Item);
    if(H->Coins>MAX_int64-Value)return false;
    BalanceEvent(TEXT("sold"),float(Value),H->Inventory[Index].Item.CatalogId);
    H->Coins+=Value;H->Inventory.RemoveAt(Index);H->SelectedItem=INDEX_NONE;
    H->InventoryMessage.Empty();PlaySound(TEXT("Equip"),.5f);return true;
}
TArray<FDungeonItem> ADungeonGameMode::CreateTraderStock(int32 Level)
{
    TArray<FDungeonItem> Stock;TSet<int32> Definitions;
    const int Count=FMath::RandRange(3,5);
    while(Stock.Num()<Count) {
        const int ID=FMath::RandRange(0,DungeonLootCatalog::Count-1);
        if(Definitions.Contains(ID))continue;
        Definitions.Add(ID);
        const float Roll=FMath::FRand();
        const int Tier=DungeonBalance::TraderRarity(Roll,Level,Stock.IsEmpty());
        Stock.Add(RollItem(ID,Tier,FMath::Min(25,Level+1)));
    }
    return Stock;
}
void ADungeonGameMode::OpenTrader()
{
    if(bAtlasActive){
        if(bTraderOpen||AtlasRooms[AtlasCurrent].Type!=EAtlasRoom::Trader)return;
        bTraderOpen=true;TraderMessage.Empty();PlaySound(TEXT("UI"));
        if(auto* PC=GetWorld()->GetFirstPlayerController())if(auto* HUD=Cast<ADungeonHUD>(PC->GetHUD())){HUD->TraderSelection=INDEX_NONE;HUD->CancelInventoryGesture();}
        return;
    }
    if(bTraderOpen||bTraderVisited)return;
    bTraderOpen=bTraderVisited=true;TraderMessage.Empty();TraderStock.Empty();
    TraderStock=CreateTraderStock(LootLevel());
    for(const auto& Item:TraderStock)BalanceEvent(TEXT("trader_offer"),Item.Rarity,Item.CatalogId);
    if(auto* PC=GetWorld()->GetFirstPlayerController())if(auto* HUD=Cast<ADungeonHUD>(PC->GetHUD())) {HUD->TraderSelection=INDEX_NONE;HUD->CancelInventoryGesture();}
    PlaySound(TEXT("UI"));
}
void ADungeonGameMode::ContinueFromTrader()
{
    if(!bTraderOpen)return;
    if(auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0)))if(H->IsInventoryOpen())H->ToggleInventory();
    bTraderOpen=false;
    if(bAtlasActive){
        SaveAtlasRoom();TraderMessage.Empty();
        // The shop is a terminal branch, not a playable chamber. Return through
        // the same doorway in its parent, restoring that room's exact state.
        for(int D=0;D<4;++D){const int Parent=AtlasRooms[AtlasCurrent].Links[D];if(Parent>=0){EnterAtlasRoom(Parent,(D+2)%4);break;}}
        return;
    }
    TraderStock.Empty();TraderMessage.Empty();NextRoom();
}
bool ADungeonGameMode::BuyTraderItem(ADungeonHero* H,int32 Index)
{
    if(!bTraderOpen||!H||!TraderStock.IsValidIndex(Index)||TraderStock[Index].IsEmpty())return false;
    const auto Item=TraderStock[Index];const int64 Price=TraderPrice(Item);
    if(H->Coins<Price){TraderMessage=TEXT("Not enough coins.");return false;}
    // Insert before charging, so a full bag never spends coins or loses stock.
    if(!H->AddToInventory(Item)){TraderMessage=TEXT("Your satchel is full. Manage your inventory first.");return false;}
    H->Coins-=Price;BalanceEvent(TEXT("purchased"),float(Price),Item.CatalogId);TraderStock[Index]=FDungeonItem();TraderMessage=TEXT("Purchased - added to your satchel.");PlaySound(TEXT("UI"));return true;
}
void ADungeonHUD::DrawTrader(ADungeonGameMode* G,ADungeonHero* H)
{
    const FLinearColor Gold(.94f,.69f,.30f),Pale(.85f,.9f,.86f);
    Sprite(TEXT("InventoryFrame"),10,40,1260,720);
    Sprite(TEXT("InventoryCard"),24,102,400,594);
    CardText(TEXT("THE TRADER"),92,146,Gold,28,270);
    // Gentle breathing and upper-body sway, anchored at the feet. Adjacent
    // strips share their boundaries so the existing artwork stays seamless.
    if(auto* Portrait=Texture(TEXT("TraderPortrait"))) {
        const float T=GetWorld()->GetRealTimeSeconds();
        const float Breath=.5f-.5f*FMath::Cos(T*2*PI/3.8f);
        auto Upper=[](float V){return 1-FMath::SmoothStep(.35f,.95f,V);};
        auto Y=[&](float V){return 185+430*V-2.5f*Breath*Upper(V);};
        for(int I=0;I<128;++I) {
            const float V=I/128.f,Next=(I+1)/128.f,Mid=(V+Next)*.5f;
            const float Chest=FMath::Max(0.f,1-FMath::Abs(Mid-.43f)/.24f);
            const float W=300*(1+.008f*Breath*Chest);
            const float X=224-W*.5f+2.2f*FMath::Sin(T*2*PI/6.2f)*Upper(Mid);
            DrawTexture(Portrait,Offset.X+X*Scale,Offset.Y+Y(V)*Scale,W*Scale,(Y(Next)-Y(V))*Scale,
                0,V,1,Next-V,FLinearColor::White,BLEND_Translucent);
        }
    }
    CardText(TEXT("A fair trade before the descent."),78,625,Pale,17,292);
    Sprite(TEXT("CurrencyCoin"),456,108,38,38);
    CardText(FString::Printf(TEXT("%lld"),H->Coins),506,117,Gold,25,310);
    CardText(TEXT("Rare finds for the road ahead"),456,151,Pale,18,360);
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
        const TCHAR* Tiers[]={TEXT("Common"),TEXT("Uncommon"),TEXT("Rare"),TEXT("Epic"),TEXT("Legendary")};
        const FLinearColor TierColor=Item.Rarity==4?Gold:Item.Rarity==3?FLinearColor(.78f,.58f,1):FLinearColor(.42f,.74f,1);
        CardText(FString::Printf(TEXT("%s / LV %d"),Tiers[Item.Rarity],Item.ItemLevel),530,Top+53,TierColor,14,165,18);
        const int64 Price=G->TraderPrice(Item);
        Sprite(TEXT("InventoryFrame"),704,Top+12,109,52);
        CardText(TEXT("BUY"),725,Top+16,Gold,18,80,20);
        CardText(FString::Printf(TEXT("%lld coins"),Price),715,Top+39,H->Coins>=Price?Pale:FLinearColor(.9f,.4f,.3f),16,96,20);
        if(DungeonInventoryLayout::In(Mouse,450,Top,374,76))Inspect=I;
    }
    if(Stock.IsValidIndex(Inspect)&&!Stock[Inspect].IsEmpty())DrawItemCard(Stock[Inspect],H,true);
    else Sprite(TEXT("InventoryCard"),842,104,400,628);
    CardText(G->TraderMessage,456,607,Pale,18,360,40);
    Sprite(TEXT("InventoryFrame"),456,653,174,42);CardText(TEXT("INVENTORY / SELL"),467,663,Gold,16,158);
    Sprite(TEXT("InventoryFrame"),642,653,174,42);CardText(G->IsAtlasFloor()?TEXT("LEAVE SHOP"):FString::Printf(TEXT("ENTER ROOM %d"),G->GetRoom()+1),661,663,Gold,18,145);
}
void ADungeonHUD::DrawTraderComparison(const FDungeonItem& Offered,const FDungeonItem& Equipped)
{
    const FLinearColor Gold(.94f,.69f,.30f),Pale(.85f,.9f,.86f),Green(.4f,1,.58f),Red(1,.4f,.3f);
    const auto Rows=DungeonItemComparison::Rows(Equipped,Offered);
    Box(884,337,322,1,Gold);
    CardText(TEXT("EQUIPPED: ")+(Equipped.IsEmpty()?FString(TEXT("NONE")):Equipped.Name),892,347,Gold,17,304,38);
    CardText(TEXT("STAT"),892,391,Pale,14,112,18);
    CardText(TEXT("GEAR"),1007,391,Gold,14,57,18);
    CardText(TEXT("OFFER"),1070,391,Pale,14,57,18);
    CardText(TEXT("DIFF"),1134,391,Gold,14,62,18);
    const int Effects=(Equipped.Effect?1:0)+(Offered.Effect&&Offered.Effect!=Equipped.Effect?1:0);
    const float Pitch=FMath::Min(22.f,(206.f-Effects*40.f)/FMath::Max(1,Rows.Num()));
    float Y=416;
    for(const auto& Row:Rows) {
        const auto C=Row.Direction()>0?Green:Row.Direction()<0?Red:Pale;
        auto Value=[&](float V,bool Signed) {
            const float Rounded=FMath::RoundToFloat(V*(Row.Percent?1000.f:10.f))/10.f;
            return Signed?FString::Printf(TEXT("%+.1f%s"),Rounded,Row.Percent?TEXT("%"):TEXT("")):FString::Printf(TEXT("%.1f%s"),Rounded,Row.Percent?TEXT("%"):TEXT(""));
        };
        CardText(Row.Label,892,Y,Pale,15,112,Pitch);
        CardText(Value(Row.Equipped,false),1007,Y,Pale,15,58,Pitch);
        CardText(Value(Row.Offered,false),1070,Y,C,15,58,Pitch);
        CardText(Value(Row.Difference(),true),1134,Y,C,15,67,Pitch);
        Y+=Pitch;
    }
    if(Rows.IsEmpty()){CardText(TEXT("No stat modifiers on either item."),892,Y,Pale,16,304,22);Y+=22;}
    // Different signatures are not universally better or worse. Show their full
    // descriptions neutrally; color a signature only when the other item has none.
    if(Equipped.Effect) {
        CardText(TEXT("Gear: ")+Equipped.EffectText(),892,Y,Offered.Effect?Pale:Red,14,304,40);Y+=40;
    }
    if(Offered.Effect&&Offered.Effect!=Equipped.Effect)
        CardText(TEXT("Offer: ")+Offered.EffectText(),892,Y,Equipped.Effect?Pale:Green,14,304,40);
    Box(884,625,322,1,Gold);
    CardText(TEXT("Diff = offer - gear. Green: offer is better."),892,632,Pale,13,304,18);
    CardText(FString::Printf(TEXT("VALUE  %d GOLD"),Offered.CoinValue),892,652,Gold,22,304,26);
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
    H->SelectedItem=H->Inventory.Num()-1;const int64 Value=G->TraderSellPrice(H->Inventory[H->SelectedItem].Item);
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
    {
        FDungeonItem Gear,Offer;Gear.Name=TEXT("Equipped");Offer.Name=TEXT("Offer");
        Gear.Movement=.15f;Offer.Movement=.10f;Gear.Defense=4;Offer.Defense=8;
        Gear.CritChance=Offer.CritChance=.05f;Gear.Speed=-.1f;Offer.Vitality=12;
        const auto Rows=DungeonItemComparison::Rows(Gear,Offer);
        Check(Rows.Num()==5,TEXT("Comparison includes union of all stats, including missing bonuses"));
        for(const auto& Row:Rows) {
            const FString Label=Row.Label;
            if(Label==TEXT("Move speed"))Check(Row.Direction()==-1&&FMath::IsNearlyEqual(Row.Difference(),-.05f),TEXT("Offer downgrade is red"));
            if(Label==TEXT("Armor")||Label==TEXT("Health")||Label==TEXT("Attack speed"))Check(Row.Direction()==1,TEXT("Offer upgrade is green"));
            if(Label==TEXT("Crit chance"))Check(Row.Direction()==0,TEXT("Equal stats are neutral"));
        }
        const auto EmptyRows=DungeonItemComparison::Rows(FDungeonItem(),Offer);
        for(const auto& Row:EmptyRows)Check(Row.Equipped==0,TEXT("Empty slot compares against zero, not hero base stats"));
    }
    Check(H->DiscardItem(0)&&H->Coins==0,TEXT("Discard drops item without coin credit"));
    Check(!H->DiscardItem(0)&&H->Coins==0,TEXT("No duplicate discard"));
    for(int Chapter=0;Chapter<DungeonProgression::Chapters;++Chapter) {
        InitializeAtlasFloor(4201,Chapter);EnterAtlasRoom(1,-1);EnterAtlasRoom(6,-1);
        Check(IsTraderOpen()&&TraderStock.Num()>=3&&TraderStock.Num()<=5,TEXT("Shop branch on each theme"));
        if(TraderStock.IsEmpty())continue; // Report a failed fixture without crashing the verifier.
        const auto Stock=TraderStock;OpenTrader();Check(Stock[0].Stats()==TraderStock[0].Stats(),TEXT("Opening shop cannot reroll stock"));
        const auto Offer=TraderStock[0];const int64 Price=TraderPrice(Offer);Check(Price>Offer.CoinValue,TEXT("Positive shop markup"));
        H->Coins=Price-1;Check(!BuyTraderItem(H,0)&&H->Coins==Price-1,TEXT("Unaffordable purchase leaves balance intact"));
        H->Coins=Price;H->Inventory.Empty();for(int I=0;I<36;++I)H->AddToInventory(RollItem(36,0));
        Check(!BuyTraderItem(H,0)&&H->Coins==Price&&!TraderStock[0].IsEmpty(),TEXT("Full bag preserves money and offer"));
        H->Inventory.Empty();Check(BuyTraderItem(H,0)&&H->Coins==0&&H->Inventory.Num()==1&&H->Inventory[0].Item.Stats()==Offer.Stats(),TEXT("Purchase exact offer"));
        Check(!BuyTraderItem(H,0)&&H->Inventory.Num()==1,TEXT("Sold item cannot be bought twice"));
        H->Coins=777;H->ToggleInventory();Check(H->IsInventoryOpen(),TEXT("Manage gear while shopping"));H->ToggleInventory();
        ContinueFromTrader();Check(AtlasCurrent==1&&!IsTraderOpen()&&H->Coins==777,TEXT("Continue preserves wallet and returns to parent room"));
        ContinueFromTrader();Check(AtlasCurrent==1,TEXT("Continue cannot travel twice"));
        TickAtlasTravel(1);
        PendingSpawns=0;
    }
    RestartRun();Check(H->Coins==0&&!IsTraderOpen(),TEXT("New run resets wallet and shop"));
    UE_LOG(LogTemp,Display,TEXT("TRADER_VERIFY_COMPLETE errors=%d"),Errors);FPlatformMisc::RequestExitWithStatus(false,Errors?1:0);
#endif
}
