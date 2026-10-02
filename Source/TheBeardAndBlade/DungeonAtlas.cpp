#include "DungeonActors.h"
#include "DungeonRoster.h"
#include "DungeonCombatBalance.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "UnrealClient.h"

namespace {
constexpr float TravelDuration=2.2f,ArrivalDuration=.75f;
const FIntPoint Steps[]={{0,1},{1,0},{0,-1},{-1,0}};
ADungeonHero* AtlasHero(UObject* O){return Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(O,0));}
}
FVector2D ADungeonGameMode::AtlasDirection(int32 D) { const FVector2D V[]={{0,-1},{1,0},{0,1},{-1,0}};return V[FMath::Clamp(D,0,3)]; }
FVector2D ADungeonGameMode::AtlasDoor(int32 D) { const FVector2D P[]={{640,192},{1160,465},{640,711},{120,465}};return P[FMath::Clamp(D,0,3)]; }
bool ADungeonGameMode::AtlasIsDescentDoor(int32 D) const
{
    if(!bAtlasActive||AtlasChapter>=DungeonProgression::Chapters-1||!AtlasRooms.IsValidIndex(AtlasCurrent))return false;
    const auto& R=AtlasRooms[AtlasCurrent];if(R.Type!=EAtlasRoom::Boss||!R.Cleared)return false;
    for(int I=0;I<4;++I)if(R.Links[I]<0)return D==I;
    return false;
}
bool ADungeonGameMode::AtlasHasDoor(int32 D) const
{
    return bAtlasActive&&D>=0&&D<4&&AtlasRooms.IsValidIndex(AtlasCurrent)&&(AtlasRooms[AtlasCurrent].Links[D]>=0||AtlasIsDescentDoor(D));
}
void ADungeonGameMode::DisableAtlas()
{
    bAtlasActive=bAtlasMap=bAtlasDescending=false;AtlasTravelTime=AtlasArrivalTime=0;AtlasRooms.Empty();AtlasCurrent=AtlasPreviousSafe=0;
}
void ADungeonGameMode::InitializeAtlasFloor(int32 Seed,int32 Chapter)
{
    DisableAtlas();bAtlasActive=true;AtlasChapter=FMath::Clamp(Chapter,0,DungeonProgression::Chapters-1);
    // Fourteen unique cells: eight combat rooms before the boss, two optional
    // combat rooms, plus entrance, trader and reward. Stable special-room IDs.
    // Each theme gets a fresh rotated/reflected layout and independent discovery/state.
    const FIntPoint Layout[]={{0,0},{0,1},{1,1},{1,2},{0,2},{-2,5},{-1,1},{2,2},
        {-1,2},{-1,3},{-2,3},{-2,4},{2,1},{3,1}};
    const int Depths[]={0,1,2,3,4,9,2,4,5,6,7,8,3,4};
    FRandomStream Random(Seed);const int Rotation=Random.RandRange(0,3);const bool Mirror=Random.RandRange(0,1)!=0;
    AtlasRooms.SetNum(UE_ARRAY_COUNT(Layout));
    for(int I=0;I<AtlasRooms.Num();++I){auto& R=AtlasRooms[I];R.Cell=Layout[I];if(Mirror)R.Cell.X=-R.Cell.X;for(int J=0;J<Rotation;++J)R.Cell=FIntPoint(-R.Cell.Y,R.Cell.X);R.Depth=Depths[I];}
    AtlasRooms[0].Type=EAtlasRoom::Entrance;AtlasRooms[5].Type=EAtlasRoom::Boss;
    AtlasRooms[6].Type=EAtlasRoom::Trader;AtlasRooms[7].Type=EAtlasRoom::Reward;
    auto Link=[&](int A,int B){const auto Delta=AtlasRooms[B].Cell-AtlasRooms[A].Cell;for(int D=0;D<4;++D)if(Delta==Steps[D]){AtlasRooms[A].Links[D]=B;AtlasRooms[B].Links[(D+2)%4]=A;}};
    for(int I=0;I<4;++I)Link(I,I+1);
    Link(4,8);Link(8,9);Link(9,10);Link(10,11);Link(11,5);
    Link(1,6);Link(3,7);Link(2,12);Link(12,13);
    EnterAtlasRoom(0,-1);
}
void ADungeonGameMode::SaveAtlasRoom()
{
    if(!bAtlasActive||!AtlasRooms.IsValidIndex(AtlasCurrent))return;
    auto& R=AtlasRooms[AtlasCurrent];R.Chest=bChest;R.LootRolled=bLootRolled;R.LootClaimed=bLootClaimed;
    R.Reward=Reward;R.Loot=Loot;R.Props=Breakables;R.Potions=Potions;R.Coins=CoinDrops;
    for(int I=0;I<3;++I){R.ChestLoot[I]=ChestLoot[I];R.ChestRolled[I]=ChestRolled[I];}
    if(R.Type==EAtlasRoom::Trader&&R.StockMade)R.Stock=TraderStock;
}
void ADungeonGameMode::EnterAtlasRoom(int32 Index,int32 EntryDoor)
{
    if(!AtlasRooms.IsValidIndex(Index))return;
    if(Index!=AtlasCurrent&&AtlasRooms.IsValidIndex(AtlasCurrent)&&AtlasRooms[AtlasCurrent].Visited&&
        AtlasRooms[AtlasCurrent].Cleared&&AtlasRooms[AtlasCurrent].Type!=EAtlasRoom::Trader)
        AtlasPreviousSafe=AtlasCurrent;
    EndBalanceRoom(TEXT("left"));
    for(auto& E:Enemies)if(IsValid(E))E->Destroy();Enemies.Empty();PendingSpawns=0;
    FreedomTime=0;bFreedomResolved=false;
    Shots.Empty();Splashes.Empty();Impacts.Empty();Blood.Empty();CancelBossIntro();DialogueLines.Empty();DialogueIndex=0;BossGrace=0;
    bTraderOpen=bTraderVisited=false;bAtlasMap=false;AtlasArrivalTime=0;TraderStock.Empty();TraderMessage.Empty();
    AtlasCurrent=Index;auto& R=AtlasRooms[Index];
    Room=AtlasChapter*DungeonProgression::RoomsPerChapter+(R.Type==EAtlasRoom::Boss?4:FMath::Clamp(R.Depth,1,3));Wave=1;LootTimer=0;TransitionCooldown=.6f;
    bChest=R.Chest;bLootRolled=R.LootRolled;bLootClaimed=R.LootClaimed;Reward=R.Reward;Loot=R.Loot;Breakables=R.Props;Potions=R.Potions;CoinDrops=R.Coins;
    for(int I=0;I<3;++I){ChestLoot[I]=R.ChestLoot[I];ChestRolled[I]=R.ChestRolled[I];}
    const bool First=!R.Visited;R.Visited=true;
    if(First){
        if(R.Type==EAtlasRoom::Combat||R.Type==EAtlasRoom::Boss)SpawnWave();
        else {R.Cleared=true;if(R.Type!=EAtlasRoom::Trader)SpawnBreakables();if(R.Type==EAtlasRoom::Reward){bChest=true;for(int I=0;I<3;++I){ChestRolled[I]=true;ChestLoot[I]=RollItem(FMath::RandRange(0,DungeonLootCatalog::Count-1),2,LootLevel());}}}
    }
    else if(R.RetryEncounter&&!R.Cleared){
        // Retry combat at full strength, but never reroll crates or duplicate
        // picked-up floor loot. SpawnWave normally creates fresh props.
        R.RetryEncounter=false;SpawnWave();Breakables=R.Props;
    }
    if(R.Type==EAtlasRoom::Trader){
        if(!R.StockMade){R.Stock=CreateTraderStock(LootLevel());R.StockMade=true;for(const auto& Item:R.Stock)BalanceEvent(TEXT("trader_offer"),Item.Rarity,Item.CatalogId);}
        TraderStock=R.Stock;
    }
    if(auto* H=AtlasHero(this)){
        H->CancelCombatActions();H->StunTime=H->SlowTime=H->FlashBlindTime=0;
        if(EntryDoor>=0&&R.Type!=EAtlasRoom::Trader){AtlasArrivalFrom=AtlasDoor(EntryDoor);AtlasArrivalTo=AtlasArrivalFrom-AtlasDirection(EntryDoor)*75;AtlasArrivalTime=ArrivalDuration;H->TransitionWalk(AtlasArrivalFrom,AtlasArrivalTo,0);}
        else H->SetActorLocation(DungeonView::Unproject(FVector2D(640,560)));
    }
    if(R.Type==EAtlasRoom::Trader)OpenTrader();
}
void ADungeonHero::ToggleAtlas(){if(auto* G=Cast<ADungeonGameMode>(UGameplayStatics::GetGameMode(this)))G->ToggleAtlasMap();}
void ADungeonGameMode::ToggleAtlasMap()
{
    if(bAtlasMap){bAtlasMap=false;PlaySound(TEXT("Paper"),.5f);return;}
    auto* H=AtlasHero(this);if(!bAtlasActive||!H||H->Health<=0||H->IsInventoryOpen()||IsGameplayBlocked())return;
    H->StopBlock();SaveAtlasRoom();bAtlasMap=true;PlaySound(TEXT("Paper"),.5f);
}
bool ADungeonGameMode::AtlasInteract(ADungeonHero* H)
{
    if(!bAtlasActive||!H)return false;
    const auto P=DungeonView::Project(H->GetActorLocation());
    if(AreDoorsOpen()&&TransitionCooldown<=0)for(int D=0;D<4;++D)if(AtlasHasDoor(D)&&FVector2D::Distance(P,AtlasDoor(D))<95){StartAtlasTravel(D);return true;}
    return false;
}
void ADungeonGameMode::StartAtlasTravel(int32 D)
{
    if(!AtlasHasDoor(D)||!AreDoorsOpen()||IsGameplayBlocked()||!Enemies.IsEmpty()||PendingSpawns>0)return;
    SaveAtlasRoom();bAtlasDescending=AtlasIsDescentDoor(D);AtlasTravelDoor=D;
    AtlasTravelTime=bAtlasDescending?DungeonDescent::Duration:TravelDuration;
    if(auto* H=AtlasHero(this)){H->CancelCombatActions();AtlasTravelFrom=DungeonView::Project(H->GetActorLocation());}
    Shots.Empty();Splashes.Empty();PlaySound(TEXT("Door"),.65f);
}
float ADungeonGameMode::AtlasHeroOpacity() const
{
    if(AtlasTravelTime>0){float T=1-AtlasTravelTime/(bAtlasDescending?DungeonDescent::Duration:TravelDuration);return 1-DungeonDescent::Ease(T,.28f,.76f);}
    return AtlasArrivalTime>0?DungeonDescent::Ease(1-AtlasArrivalTime/ArrivalDuration,0,.8f):1;
}
float ADungeonGameMode::AtlasBlackout() const
{
    if(AtlasTravelTime>0)return DungeonDescent::Ease(1-AtlasTravelTime/(bAtlasDescending?DungeonDescent::Duration:TravelDuration),.80f,1);
    return AtlasArrivalTime>0?1-DungeonDescent::Ease(1-AtlasArrivalTime/ArrivalDuration,0,.55f):0;
}
void ADungeonGameMode::TickAtlasTravel(float Dt)
{
    auto* H=AtlasHero(this);
    if(AtlasTravelTime>0){
        AtlasTravelTime=FMath::Max(0.f,AtlasTravelTime-Dt);
        float T=1-AtlasTravelTime/(bAtlasDescending?DungeonDescent::Duration:TravelDuration);
        const auto Door=AtlasDoor(AtlasTravelDoor),Beyond=Door+AtlasDirection(AtlasTravelDoor)*35;
        if(H){if(T<.35f)H->TransitionWalk(AtlasTravelFrom,Door,T/.35f);else H->TransitionWalk(Door,Beyond,FMath::Clamp((T-.35f)/.43f,0.f,1.f),2.f);}
        if(AtlasTravelTime<=0){
            if(bAtlasDescending){
                const int NextChapter=AtlasChapter+1;
                InitializeAtlasFloor(FMath::Rand(),NextChapter);
                AtlasArrivalFrom=AtlasDoor(2);AtlasArrivalTo=AtlasArrivalFrom-AtlasDirection(2)*75;AtlasArrivalTime=ArrivalDuration;
                if(H)H->TransitionWalk(AtlasArrivalFrom,AtlasArrivalTo,0);
            }else {const int Next=AtlasRooms[AtlasCurrent].Links[AtlasTravelDoor];EnterAtlasRoom(Next,(AtlasTravelDoor+2)%4);}
        }
    }else if(AtlasArrivalTime>0){AtlasArrivalTime=FMath::Max(0.f,AtlasArrivalTime-Dt);if(H)H->TransitionWalk(AtlasArrivalFrom,AtlasArrivalTo,1-AtlasArrivalTime/ArrivalDuration);}
}

void ADungeonHUD::DrawAtlas(ADungeonGameMode* G,ADungeonHero* H)
{
    const FLinearColor Gold(.94f,.69f,.30f),Pale(.85f,.9f,.86f),Emerald(.12f,.85f,.67f),Dim(.3f,.36f,.34f);
    Box(0,0,1280,800,FLinearColor(0,0,0,.78f));Sprite(TEXT("InventoryFrame"),150,35,980,730);
    Sprite(TEXT("AtlasMapIcon"),215,95,70,70);
    // Trim transparent title padding at draw time; keep the source artwork intact.
    if(auto* Title=Texture(TEXT("AtlasTitle")))DrawTexture(Title,Offset.X+300*Scale,Offset.Y+96*Scale,420*Scale,68*Scale,0,.24f,1,.48f,FLinearColor::White,BLEND_Translucent);
    CardText(FString::Printf(TEXT("Floor %d  /  %s"),G->GetAtlasChapter()+1,DungeonRoster::Biome(G->GetBiome())),300,158,Pale,16,620);
    CardText(TEXT("M  /  CLOSE"),940,112,Gold,17,145);
    Box(220,183,850,1,FLinearColor(.5f,.38f,.18f));
    const auto& Rooms=G->GetAtlasRooms();FIntPoint Min(999,999),Max(-999,-999);
    for(const auto& R:Rooms)if(R.Visited){Min.X=FMath::Min(Min.X,R.Cell.X);Min.Y=FMath::Min(Min.Y,R.Cell.Y);Max.X=FMath::Max(Max.X,R.Cell.X);Max.Y=FMath::Max(Max.Y,R.Cell.Y);}
    const float Step=FMath::Min(110.f,FMath::Min(700.f/(Max.X-Min.X+1),365.f/(Max.Y-Min.Y+1)));
    const float Size=Step*.6f;const FVector2D Mid((Min.X+Max.X)*.5,(Min.Y+Max.Y)*.5);
    auto Position=[&](const FAtlasRoom& R){return FVector2D(640+(R.Cell.X-Mid.X)*Step,403-(R.Cell.Y-Mid.Y)*Step);};
    // Only connect two discovered rooms. Open stubs disclose a seen doorway, not a hidden room.
    for(int I=0;I<Rooms.Num();++I)if(Rooms[I].Visited)for(int D=0;D<4;++D){int J=Rooms[I].Links[D];if(J>I&&Rooms.IsValidIndex(J)&&Rooms[J].Visited){auto A=Position(Rooms[I]),B=Position(Rooms[J]);DrawLine(Offset.X+A.X*Scale,Offset.Y+A.Y*Scale,Offset.X+B.X*Scale,Offset.Y+B.Y*Scale,Gold,3*Scale);}}
    for(int I=0;I<Rooms.Num();++I){const auto& R=Rooms[I];if(!R.Visited)continue;auto P=Position(R);const bool Current=I==G->GetAtlasRoom();
        Box(P.X-Size/2,P.Y-Size/2,Size,Size,Current?Emerald:R.Cleared?Gold:Dim);
        Box(P.X-Size/2+2,P.Y-Size/2+2,Size-4,Size-4,FLinearColor(.015f,.023f,.025f));
        for(int D=0;D<4;++D){int J=R.Links[D];if(J>=0&&!Rooms[J].Visited){auto Edge=P+ADungeonGameMode::AtlasDirection(D)*(Size/2);if(D%2==0){Box(Edge.X-8,Edge.Y-3,16,6,Emerald);Box(Edge.X-5,Edge.Y-3,10,6,FLinearColor(.01f,.04f,.04f));}else {Box(Edge.X-3,Edge.Y-8,6,16,Emerald);Box(Edge.X-3,Edge.Y-5,6,10,FLinearColor(.01f,.04f,.04f));}}}
        if(Current){const float A=H->GetFacingDirection()*PI/4;const FVector2D F(FMath::Sin(A),-FMath::Cos(A)),Side(-F.Y,F.X);auto Tip=P+F*13,Left=P-F*9+Side*9,Right=P-F*9-Side*9;DrawLine(Offset.X+Tip.X*Scale,Offset.Y+Tip.Y*Scale,Offset.X+Left.X*Scale,Offset.Y+Left.Y*Scale,Emerald,4*Scale);DrawLine(Offset.X+Tip.X*Scale,Offset.Y+Tip.Y*Scale,Offset.X+Right.X*Scale,Offset.Y+Right.Y*Scale,Emerald,4*Scale);}
        else if(R.Type==EAtlasRoom::Trader)Sprite(TEXT("InventorySatchel"),P.X-18,P.Y-19,36,36);
        else if(R.Type==EAtlasRoom::Boss)Sprite(FString::Printf(TEXT("BossPortrait_%d"),DungeonProgression::Bosses[G->GetAtlasChapter()]-24),P.X-22,P.Y-22,44,44);
        else if(R.Type==EAtlasRoom::Entrance){for(int S=0;S<4;++S)Box(P.X-14+S*3,P.Y-12+S*7,28-S*6,2,Gold);}
        else if((R.Chest&&!R.LootClaimed)||R.Props.ContainsByPredicate([](const FDungeonBreakable& B){return B.BrokenAge>=0&&!B.Collected&&!B.Loot.IsEmpty();}))Sprite(TEXT("RewardChest_0"),P.X-23,P.Y-20,46,40);
        else if(R.Cleared){DrawLine(Offset.X+(P.X-9)*Scale,Offset.Y+P.Y*Scale,Offset.X+(P.X-2)*Scale,Offset.Y+(P.Y+7)*Scale,Pale,2*Scale);DrawLine(Offset.X+(P.X-2)*Scale,Offset.Y+(P.Y+7)*Scale,Offset.X+(P.X+12)*Scale,Offset.Y+(P.Y-9)*Scale,Pale,2*Scale);}
    }
    CardText(TEXT("N"),1021,208,Gold,18,40);Box(1030,238,2,26,Gold);
    DrawLine(Offset.X+1024*Scale,Offset.Y+244*Scale,Offset.X+1031*Scale,Offset.Y+237*Scale,Gold,2*Scale);
    DrawLine(Offset.X+1031*Scale,Offset.Y+237*Scale,Offset.X+1038*Scale,Offset.Y+244*Scale,Gold,2*Scale);
    Box(220,620,850,1,FLinearColor(.5f,.38f,.18f));
    auto Stroke=[&](float X,float Y,float XX,float YY,FLinearColor C){DrawLine(Offset.X+X*Scale,Offset.Y+Y*Scale,Offset.X+XX*Scale,Offset.Y+YY*Scale,C,2*Scale);};
    Stroke(226,650,233,640,Emerald);Stroke(233,640,240,650,Emerald);CardText(TEXT("You"),251,639,Emerald,17,75);
    Stroke(353,647,358,652,Pale);Stroke(358,652,370,640,Pale);CardText(TEXT("Cleared"),383,639,Pale,17,110);
    Sprite(TEXT("InventorySatchel"),511,632,28,28);CardText(TEXT("Trader"),549,639,Gold,17,100);
    Box(697,639,3,14,Emerald);Box(707,639,3,14,Emerald);CardText(TEXT("Unexplored exit"),727,639,Emerald,17,220);
    CardText(TEXT("Only explored rooms are recorded."),220,674,Pale,15,730);
}

void ADungeonHUD::DrawAtlasDoors(ADungeonGameMode* G,ADungeonHero* H)
{
    const FLinearColor Gold(.94f,.69f,.30f),Emerald(.16f,.82f,.62f);
    auto P=DungeonView::Project(H->GetActorLocation());
    for(int D=0;D<4;++D){const auto Door=G->AtlasDoor(D);bool Exists=G->AtlasHasDoor(D);bool Open=Exists&&G->AreDoorsOpen();
        const float W=D%2?78:104,HH=D%2?90:104;
        const FVector2D Centers[]={{640,75},{1235,392},{640,740},{45,392}};
        const FVector2D Center=Centers[D];
        if(!Open){
            // Portcullis bars visually close unused arches and lock active combat exits.
            Box(Center.X-W/2,Center.Y-HH/2,W,HH,FLinearColor(.013f,.02f,.023f,Exists?.62f:.90f));
            for(int I=0;I<7;++I){const float X=Center.X-W/2+8+I*(W-16)/6;Box(X-1,Center.Y-HH/2,5,HH,FLinearColor(.035f,.04f,.04f));Box(X,Center.Y-HH/2,3,HH,FLinearColor(.11f,.14f,.14f));Box(X+1,Center.Y-HH/2,1,HH,FLinearColor(.25f,.24f,.20f));}
            for(float Rail:{-.24f,.25f}){Box(Center.X-W/2,Center.Y+HH*Rail,W,5,FLinearColor(.09f,.11f,.11f));Box(Center.X-W/2,Center.Y+HH*Rail,W,1,FLinearColor(.28f,.24f,.16f));for(int I=0;I<4;++I)Box(Center.X-W/2+9+I*(W-18)/3,Center.Y+HH*Rail+1,2,2,FLinearColor(.35f,.29f,.18f));}
        }else{
            const int J=G->GetAtlasRooms()[G->GetAtlasRoom()].Links[D];
            const bool Trader=J>=0&&G->GetAtlasRooms()[J].Type==EAtlasRoom::Trader;
            const bool Treasure=J>=0&&G->GetAtlasRooms()[J].Type==EAtlasRoom::Reward;
            // A boss entrance is indistinguishable from an ordinary combat door.
            const auto C=Trader?Gold:Emerald;
            const FVector2D Thresholds[]={{640,135},{1195,411},{640,686},{85,411}};const auto Threshold=Thresholds[D];
            // Soft, layered threshold light sits inside the opening rather than on the room floor.
            for(int I=3;I>0;--I)Box(Threshold.X-(D%2?I:35),Threshold.Y-(D%2?24:I),D%2?I*2:70,D%2?48:I*2,FLinearColor(C.R,C.G,C.B,.08f));
            if(Trader)Sprite(TEXT("InventorySatchel"),Door.X-18,Door.Y-70,36,36);
            if(Treasure)Sprite(TEXT("RewardChest_0"),Door.X-22,Door.Y-70,44,38);
            if(G->AtlasIsDescentDoor(D)){for(int S=0;S<5;++S)Box(Door.X-35+S*5,Door.Y-46+S*8,70-S*10,3,Gold);}
        }
        if(Exists&&FVector2D::Distance(P,Door)<105&&!G->IsAtlasTravel()){
            FString Text=Open?(G->AtlasIsDescentDoor(D)?TEXT("E  Descend to the next floor"):TEXT("E  Enter doorway")):TEXT("Clear this room to open the gate");
            CardText(Text,D==1?875:D==3?145:480,D==2?662:D==0?205:520,Gold,18,325,28);
        }
    }
}

void ADungeonGameMode::VerifyAtlas()
{
#if !UE_BUILD_SHIPPING
    int Errors=0;auto Check=[&](bool B,const TCHAR* Why){if(!B){++Errors;UE_LOG(LogTemp,Error,TEXT("ATLAS: %s"),Why);}};
    auto* H=AtlasHero(this);if(!H){FPlatformMisc::RequestExitWithStatus(false,1);return;}
    bMenu=false;bHasRun=true;
    for(int Chapter=0;Chapter<DungeonProgression::Chapters;++Chapter)for(int Seed=0;Seed<128;++Seed){
        InitializeAtlasFloor(Seed,Chapter);TSet<FIntPoint> Cells;int Traders=0,Rewards=0,Bosses=0,Edges=0;
        Check(GetBiome()==DungeonProgression::Themes[Chapter]&&GetBossSpecies()==DungeonProgression::Bosses[Chapter],TEXT("Floor retains theme and boss order"));
        TArray<int> Distance;Distance.Init(-1,AtlasRooms.Num());Distance[0]=0;TArray<int> Queue;Queue.Add(0);
        for(int K=0;K<Queue.Num();++K){int I=Queue[K];for(int D=0;D<4;++D){int J=AtlasRooms[I].Links[D];if(J>=0){Check(AtlasRooms[J].Links[(D+2)%4]==I,TEXT("Reciprocal doorway"));Check(AtlasRooms[J].Cell-AtlasRooms[I].Cell==Steps[D],TEXT("Physical adjacency"));if(Distance[J]<0){Distance[J]=Distance[I]+1;Queue.Add(J);}}}}
        for(int I=0;I<AtlasRooms.Num();++I){const auto& R=AtlasRooms[I];Cells.Add(R.Cell);Traders+=R.Type==EAtlasRoom::Trader;Rewards+=R.Type==EAtlasRoom::Reward;Bosses+=R.Type==EAtlasRoom::Boss;int Degree=0;for(int D=0;D<4;++D)Degree+=R.Links[D]>=0;Edges+=Degree;Check(R.Depth==Distance[I],TEXT("Encounter depth equals shortest route"));if(R.Type==EAtlasRoom::Boss||R.Type==EAtlasRoom::Trader||R.Type==EAtlasRoom::Reward)Check(Degree==1,TEXT("Special rooms are branch endpoints"));}
        Check(Cells.Num()==14&&Queue.Num()==14&&Edges==26,TEXT("Unique connected non-crossing expanded tree"));
        Check(Traders==1&&Rewards==1&&Bosses==1&&Distance[5]==9,TEXT("One trader, reward, distant boss"));
        Check(AtlasRooms.FilterByPredicate([](const FAtlasRoom& R){return R.Visited;}).Num()==1,TEXT("Only entrance revealed initially"));
    }
    // Exercise every new campaign encounter through real spawn/clear/backtrack.
    TSet<int32> SeenExpansion;
    for(int Chapter=0;Chapter<DungeonProgression::Chapters;++Chapter){
        InitializeAtlasFloor(37,Chapter);
        for(int Index:{11,13,10,9,8,12,4,3,2,1}){
            EnterAtlasRoom(Index,-1);
            const int Depth=BalanceDepth();
            for(int ExpectedWave=1;ExpectedWave<=2;++ExpectedWave){
                const auto Plan=DungeonRoster::AtlasEncounter(GetBiome(),Depth,ExpectedWave);
                Check(Wave==ExpectedWave&&PendingSpawns==Plan.Num(),TEXT("Expanded encounter wave budget"));
                int Elites=0;
                for(int Species:Plan){
                    SpawnOneEnemy();--PendingSpawns;
                    Check(!Enemies.IsEmpty()&&Enemies.Last()->Species==Species,TEXT("Expanded theme roster spawns exactly"));
                    if(DungeonExpansionV2::Is(Species)){
                        SeenExpansion.Add(Species);
                        Check(DungeonExpansionV2::Enabled(Species),TEXT("Removed species never enter campaign"));
                        const bool Elite=DungeonExpansionV2::Get(Species).Elite||Species==68;
                        Elites+=Elite;
                        Check(!Elite||Depth>=6,TEXT("Heavy elites reserved for late rooms"));
                    }
                    if(!Enemies.IsEmpty())Check(FMath::IsNearlyEqual(Enemies.Last()->MaxHealth,DungeonBalance::EnemyHealth(DungeonRoster::BalanceRole(Species),BalanceFloor(),BalanceDepth())),TEXT("Expanded health uses role floor and depth once"));
                }
                Check(Elites<=1,TEXT("No stacked elites in encounter wave"));
                const auto Defeated=Enemies;for(const auto& E:Defeated)EnemyDefeated(E.Get());
            }
            Check(AtlasRooms[Index].Cleared&&AreDoorsOpen(),TEXT("Expanded room clear unlocks exits"));
            SaveAtlasRoom();EnterAtlasRoom(0,-1);EnterAtlasRoom(Index,-1);
            Check(Enemies.IsEmpty()&&PendingSpawns==0,TEXT("Expanded room remains cleared on backtrack"));
        }
    }
    for(int Species:DungeonExpansionV2::Active)Check(SeenExpansion.Contains(Species),TEXT("Every approved new enemy appears in campaign"));
    H->Restart();
    // Integrated Keep: room depth controls both waves regardless of visit order.
    // Exercise real spawn/defeat paths, not just the encounter table.
    for(int Seed=0;Seed<8;++Seed){
        InitializeAtlasFloor(Seed,0);
        for(int Index:{4,2,1,3}){
            EnterAtlasRoom(Index,-1);
            int Lancers=0,Bailiffs=0,Hexers=0;
            for(int ExpectedWave=1;ExpectedWave<=2;++ExpectedWave){
                const auto Plan=DungeonRoster::KeepEncounter(AtlasRooms[Index].Depth,ExpectedWave);
                Check(Wave==ExpectedWave&&PendingSpawns==Plan.Num(),TEXT("Keep wave budget by depth"));
                for(int Species:Plan){
                    SpawnOneEnemy();--PendingSpawns;
                    Check(!Enemies.IsEmpty()&&Enemies.Last()->Species==Species,TEXT("Keep depth composition independent of visit order"));
                    if(!Enemies.IsEmpty()){
                        auto* E=Enemies.Last().Get();
                        Check(FMath::IsNearlyEqual(E->MaxHealth,DungeonBalance::EnemyHealth(DungeonRoster::BalanceRole(Species),BalanceFloor(),BalanceDepth())),TEXT("Keep health scaling applied once"));
                        Lancers+=E->Species==53;Bailiffs+=E->Species==51;Hexers+=E->Species==52;
                    }
                }
                const auto Defeated=Enemies;
                for(const auto& E:Defeated)EnemyDefeated(E.Get());
            }
            Check(Lancers==0&&Bailiffs==(Index==2?1:0)&&Hexers==((Index==3||Index==4)?1:0),TEXT("Introduce specialists before late-room elites"));
            Check(AtlasRooms[Index].Cleared&&AreDoorsOpen()&&PendingSpawns==0,TEXT("Both Keep waves clear before exit"));
            SaveAtlasRoom();EnterAtlasRoom(0,-1);EnterAtlasRoom(Index,-1);
            Check(Enemies.IsEmpty()&&PendingSpawns==0&&AreDoorsOpen(),TEXT("Cleared Keep room never respawns"));
        }
        for(int Index:{6,7}){
            EnterAtlasRoom(Index,-1);
            Check(Enemies.IsEmpty()&&PendingSpawns==0,TEXT("Keep trader and reward remain peaceful"));
        }
    }
    // Every cardinal doorway is exercised using an existing reciprocal link.
    for(int Chapter=0;Chapter<DungeonProgression::Chapters;++Chapter){
    InitializeAtlasFloor(17,Chapter);
    for(int D=0;D<4;++D){
        int Source=-1;for(int I=0;I<8;++I)if(AtlasRooms[I].Type!=EAtlasRoom::Trader&&AtlasRooms[I].Links[D]>=0&&AtlasRooms[AtlasRooms[I].Links[D]].Type!=EAtlasRoom::Trader){Source=I;break;}
        Check(Source>=0,TEXT("All cardinal directions represented"));if(Source<0)continue;
        EnterAtlasRoom(Source,-1);for(auto& E:Enemies)if(IsValid(E))E->Destroy();Enemies.Empty();PendingSpawns=0;AtlasRooms[Source].Cleared=true;
        int Dest=AtlasRooms[Source].Links[D];H->SetActorLocation(DungeonView::Unproject(AtlasDoor(D)-AtlasDirection(D)*60));
        StartAtlasTravel(D);Check(IsAtlasTravel()&&AtlasHeroOpacity()==1,TEXT("Begin doorway fully visible"));
        TickAtlasTravel(TravelDuration*.5f);Check(AtlasHeroOpacity()>0&&AtlasHeroOpacity()<1,TEXT("Gradual fade through doorway"));
        Check(H->GetFacingDirection()==D*2,TEXT("Hero faces chosen exit"));
        TickAtlasTravel(TravelDuration);Check(AtlasCurrent==Dest&&AtlasArrivalTime>0,TEXT("Adjacent room with arrival fade"));
        TickAtlasTravel(ArrivalDuration);Check(AtlasHeroOpacity()==1,TEXT("Arrival restores opacity"));
        Check(DungeonView::Project(H->GetActorLocation()).Equals(AtlasDoor((D+2)%4)-AtlasDirection((D+2)%4)*75,.01),TEXT("Opposite doorway arrival"));
    }
    }
    InitializeAtlasFloor(23);EnterAtlasRoom(1,-1);Check(!AreDoorsOpen(),TEXT("Uncleared combat exit locked"));
    FreedomTime=2.f;EnterAtlasRoom(0,-1);Check(FreedomTime==0,TEXT("Room changes clear transient attacks"));EnterAtlasRoom(1,-1);
    StartAtlasTravel(0);Check(!IsAtlasTravel(),TEXT("Cannot bypass queued combat"));
    const auto Pos=H->GetActorLocation();const int Pending=PendingSpawns;ToggleAtlasMap();Check(bAtlasMap,TEXT("M opens map"));H->MoveRight(1);H->Tick(.2f);Tick(.2f);H->MoveRight(0);Check(Pos.Equals(H->GetActorLocation())&&PendingSpawns==Pending,TEXT("Map pauses simulation"));ToggleAtlasMap();
    // Reward remains on the floor, even if a doorway is taken during ejection.
    EnterAtlasRoom(7,-1);Check(bChest&&AreDoorsOpen(),TEXT("Reward dead end is safe"));
    H->SetActorLocation(DungeonView::Unproject(ChestPosition(1)));InteractReward(H);const auto Rolled=Loot;UpdateReward(10);
    Breakables[0].BrokenAge=2;Breakables[0].Loot=RollItem(48,3,2);FDungeonPotion Potion;Potion.Position=FVector2D(450,500);Potions.Add(Potion);
    SaveAtlasRoom();EnterAtlasRoom(3,-1);EnterAtlasRoom(7,-1);
    Check(Reward.Phase==ERewardPhase::Available&&Loot.Stats()==Rolled.Stats()&&Loot.CatalogId==Rolled.CatalogId,TEXT("Exact floor reward persists"));
    Check(Breakables[0].BrokenAge==2&&Breakables[0].Loot.CatalogId==48&&Potions.Num()==1,TEXT("Props and potions persist"));
    H->Inventory.Empty();H->SetActorLocation(DungeonView::Unproject(Reward.Landing));InteractReward(H);SaveAtlasRoom();EnterAtlasRoom(0,-1);EnterAtlasRoom(7,-1);
    Check(bLootClaimed&&Reward.Phase==ERewardPhase::Collected&&H->Inventory.Num()==1,TEXT("Collected reward cannot duplicate"));
    // Persistent stock, sold entries, close/reopen, and physical return to trader.
    EnterAtlasRoom(1,-1);AtlasRooms[1].Cleared=true;PendingSpawns=0;for(auto& E:Enemies)if(IsValid(E))E->Destroy();Enemies.Empty();
    int ShopDoor=0;while(ShopDoor<4&&AtlasRooms[1].Links[ShopDoor]!=6)++ShopDoor;
    const int ParentProps=Breakables.Num();SaveAtlasRoom();StartAtlasTravel(ShopDoor);TickAtlasTravel(TravelDuration);
    Check(AtlasCurrent==6&&bTraderOpen&&AtlasArrivalTime==0&&Breakables.IsEmpty(),TEXT("Door opens shop directly without playable chamber"));H->Coins=100000;
    Check(BuyTraderItem(H,0),TEXT("Trader purchase"));const int64 WalletAfter=H->Coins;ContinueFromTrader();
    Check(AtlasCurrent==1&&!bTraderOpen&&AtlasArrivalTime>0&&Breakables.Num()==ParentProps,TEXT("Leave shop restores parent room and fades in"));
    Check(AtlasArrivalFrom.Equals(AtlasDoor(ShopDoor)),TEXT("Return through same parent doorway"));
    ContinueFromTrader();Check(AtlasCurrent==1,TEXT("Repeated close cannot travel twice"));TickAtlasTravel(ArrivalDuration);
    StartAtlasTravel(ShopDoor);TickAtlasTravel(TravelDuration);Check(bTraderOpen&&TraderStock[0].IsEmpty()&&H->Coins==WalletAfter,TEXT("Backtracking keeps stock and wallet"));ContinueFromTrader();TickAtlasTravel(ArrivalDuration);
    // Boss only spawns at endpoint; return visits cannot respawn it.
    EnterAtlasRoom(5,-1);Check(IsBossRoom()&&PendingSpawns==1,TEXT("Boss endpoint queues Finance"));
    SpawnOneEnemy();PendingSpawns=0;Check(Enemies.Num()==1&&Enemies[0]->Species==24,TEXT("Finance Guy encounter"));
    CancelBossIntro();DialogueLines.Empty();DialogueIndex=0;BossGrace=0;if(!Enemies.IsEmpty())EnemyDefeated(Enemies[0]);
    Check(AtlasRooms[5].Cleared&&AreDoorsOpen(),TEXT("Boss defeat opens descent"));SaveAtlasRoom();EnterAtlasRoom(4,-1);EnterAtlasRoom(5,-1);Check(Enemies.IsEmpty()&&PendingSpawns==0,TEXT("Defeated boss stays dead"));
    int Exit=0;while(Exit<4&&!AtlasIsDescentDoor(Exit))++Exit;Check(Exit<4,TEXT("Descent has free doorway"));
    StartAtlasTravel(Exit);TickAtlasTravel(5);Check(bAtlasActive&&AtlasChapter==1&&AtlasCurrent==0&&Room==5&&GetBiome()==6,TEXT("Descent starts second theme atlas"));TickAtlasTravel(1);
    // Walk the full campaign's systems: level-scaled roster/rewards/stock,
    // persistent branch purchases, non-respawning clears and final victory.
    RestartRun();H->Coins=100000;
    for(int Chapter=0;Chapter<DungeonProgression::Chapters;++Chapter){
        Check(AtlasChapter==Chapter&&AtlasCurrent==0&&!HasEnding(),TEXT("Sequential floor arrival"));
        Check(AtlasRooms.FilterByPredicate([](const FAtlasRoom& R){return R.Visited;}).Num()==1,TEXT("New floor starts undiscovered"));
        const FString Art=Chapter==0?TEXT("AtlasChamber"):FString::Printf(TEXT("AtlasChamber%d"),GetBiome());
        Check(LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Art/V2/%s.%s"),*Art,*Art))!=nullptr,TEXT("Themed four-way background imported"));
        const int Portrait=DungeonProgression::Bosses[Chapter]-24;
        Check(LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Art/September/BossPortrait_%d.BossPortrait_%d"),Portrait,Portrait))!=nullptr,TEXT("Boss portrait available for explored map rooms only"));
        TickAtlasTravel(ArrivalDuration);
        EnterAtlasRoom(1,-1);SpawnOneEnemy();--PendingSpawns;
        Check(!Enemies.IsEmpty()&&(Chapter==0?Enemies[0]->Species==49:Enemies[0]->Species>=DungeonProgression::RosterBase(GetBiome())&&Enemies[0]->Species<DungeonProgression::RosterBase(GetBiome())+6),TEXT("Theme-specific regular enemies"));
        for(auto& E:Enemies)if(IsValid(E))E->Destroy();Enemies.Empty();PendingSpawns=0;CompleteRoom();
        Check(AtlasRooms[1].Cleared&&AreDoorsOpen(),TEXT("Normal clear unlocks floor exits"));
        Breakables[0].BrokenAge=2;Breakables[0].Loot=RollItem(48,3,Room);const auto Drop=Breakables[0].Loot;SaveAtlasRoom();
        EnterAtlasRoom(6,-1);Check(bTraderOpen&&TraderStock.Num()>=3&&TraderStock.Num()<=5&&TraderStock[0].ItemLevel==FMath::Min(25,2+Chapter*3),TEXT("Each floor has premium level-scaled shop"));
        H->Inventory.Empty();Check(BuyTraderItem(H,0),TEXT("Purchase on each floor"));ContinueFromTrader();TickAtlasTravel(ArrivalDuration);
        Check(AtlasCurrent==1&&Enemies.IsEmpty()&&PendingSpawns==0&&Breakables[0].BrokenAge==2&&Breakables[0].Loot.Stats()==Drop.Stats(),TEXT("Parent clear and floor loot survive shop visit"));
        SaveAtlasRoom();EnterAtlasRoom(6,-1);Check(TraderStock[0].IsEmpty(),TEXT("Each floor preserves sold stock"));ContinueFromTrader();TickAtlasTravel(ArrivalDuration);
        EnterAtlasRoom(7,-1);Check(bChest&&ChestRolled[0]&&ChestLoot[0].ItemLevel==LootLevel(),TEXT("Reward endpoint scales with floor"));
        EnterAtlasRoom(5,-1);SpawnOneEnemy();PendingSpawns=0;
        Check(Enemies.Num()==1&&Enemies[0]->Species==DungeonProgression::Bosses[Chapter],TEXT("Campaign boss progression unchanged"));
        CancelBossIntro();DialogueLines.Empty();DialogueIndex=0;BossGrace=0;
        if(!Enemies.IsEmpty())EnemyDefeated(Enemies[0]);
        if(Chapter==DungeonProgression::Chapters-1){Check(IsVictory(),TEXT("Twister defeat ends campaign"));for(int D=0;D<4;++D)Check(!AtlasIsDescentDoor(D),TEXT("No eighth-floor exit"));}
        else{
            Check(!HasEnding()&&AtlasRooms[5].Cleared,TEXT("Earlier bosses do not end run"));SaveAtlasRoom();EnterAtlasRoom(4,-1);EnterAtlasRoom(5,-1);
            Check(Enemies.IsEmpty()&&PendingSpawns==0,TEXT("Every defeated boss stays defeated"));
            const auto Health=H->Health,Stamina=H->Stamina;const int64 Coins=H->Coins;const int Items=H->Inventory.Num();
            int D=0;while(D<4&&!AtlasIsDescentDoor(D))++D;Check(D<4,TEXT("Next floor descent available"));StartAtlasTravel(D);TickAtlasTravel(5);
            Check(bAtlasActive&&AtlasChapter==Chapter+1&&H->Health==Health&&H->Stamina==Stamina&&H->Coins==Coins&&H->Inventory.Num()==Items,TEXT("Descent preserves hero and inventory"));
        }
    }
    RestartRun();Check(bAtlasActive&&AtlasCurrent==0&&H->Coins==0,TEXT("New run resets floor and wallet"));
    Check(LoadObject<UTexture2D>(nullptr,TEXT("/Game/Art/V2/AtlasChamber.AtlasChamber"))!=nullptr,TEXT("Four-way art imported"));
    UE_LOG(LogTemp,Display,TEXT("ATLAS_VERIFY_COMPLETE errors=%d"),Errors);FPlatformMisc::RequestExitWithStatus(false,Errors?1:0);
#endif
}
void ADungeonGameMode::RunAtlasReview()
{
#if !UE_BUILD_SHIPPING
    if(!FParse::Param(FCommandLine::Get(),TEXT("AtlasReview")))return;
    static int Step=0;const float Time=GetWorld()->GetTimeSeconds();auto* H=AtlasHero(this);if(!H)return;
    if(FParse::Param(FCommandLine::Get(),TEXT("AtlasExpandedMapReview"))){
        if(Step==0&&Time>2){StartGame();InitializeAtlasFloor(12);for(auto& R:AtlasRooms){R.Visited=true;R.Cleared=true;}ToggleAtlasMap();++Step;}
        if(Step==1&&Time>4){FScreenshotRequest::RequestScreenshot(FPaths::ProjectDir()/TEXT("Saved/AtlasExpandedMap.png"),false,false);++Step;}
        if(Step==2&&Time>5)FPlatformMisc::RequestExit(false);
        return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("AtlasCurrencyReview"))){
        auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectDir()/TEXT("ArtSource/")+Name+TEXT(".png"),false,false);};
        if(Step==0&&Time>2){StartGame();InitializeAtlasFloor(12);AtlasRooms[4].Visited=AtlasRooms[4].Cleared=true;EnterAtlasRoom(4,-1);H->Coins=12345;++Step;}
        if(Step==1&&Time>3.5f){Shot(TEXT("MysteryDoorReview"));++Step;}
        if(Step==2&&Time>4.2f){ToggleAtlasMap();++Step;}
        if(Step==3&&Time>4.8f){Shot(TEXT("MysteryMapReview"));++Step;}
        if(Step==4&&Time>5.2f){ToggleAtlasMap();H->ToggleInventory();++Step;}
        if(Step==5&&Time>6.3f){Shot(TEXT("CurrencyInventoryReview"));++Step;}
        if(Step==6&&Time>7.2f)FPlatformMisc::RequestExit(false);
        return;
    }
    int ReviewDoor=-1,ReviewChapter=0;FParse::Value(FCommandLine::Get(),TEXT("AtlasReviewDoor="),ReviewDoor);FParse::Value(FCommandLine::Get(),TEXT("AtlasReviewChapter="),ReviewChapter);
    auto Capture=[&](const TCHAR* Name){const FString Suffix=FString::Printf(TEXT("_Floor%d_Door%d"),ReviewChapter+1,ReviewDoor);FScreenshotRequest::RequestScreenshot(FPaths::ProjectDir()/TEXT("ArtSource/")+Name+Suffix+TEXT(".png"),false,false);};
    if(Step==0&&Time>2){
        StartGame();InitializeAtlasFloor(12,ReviewChapter);
        for(int I:{0,1,2,3,6,7}){AtlasRooms[I].Visited=true;AtlasRooms[I].Cleared=true;}
        AtlasRooms[7].Chest=true;
        EnterAtlasRoom(3,-1);PendingSpawns=0;AtlasRooms[3].Cleared=true;
        if(ReviewDoor>=0&&ReviewDoor<4){for(int I=0;I<AtlasRooms.Num();++I)if(AtlasRooms[I].Links[ReviewDoor]>=0){AtlasRooms[I].Visited=AtlasRooms[I].Cleared=true;EnterAtlasRoom(I,-1);PendingSpawns=0;break;}}
        H->SetActorLocation(DungeonView::Unproject(FVector2D(640,540)));
        if(FParse::Param(FCommandLine::Get(),TEXT("AtlasMapReview")))ToggleAtlasMap();
        ++Step;
    }
    if(Step==1&&Time>3.5f){Capture(TEXT("AtlasRoomReview"));++Step;}
    if(Step==2&&Time>4.2f){
        ToggleAtlasMap();
        ++Step;
    }
    if(Step==3&&Time>4.8f){Capture(TEXT("AtlasMapReview"));++Step;}
    if(Step==4&&Time>5.2f){if(bAtlasMap)ToggleAtlasMap();int D=ReviewDoor>=0&&ReviewDoor<4?ReviewDoor:0;while(D<4&&!AtlasHasDoor(D))++D;H->SetActorLocation(DungeonView::Unproject(AtlasDoor(D)-AtlasDirection(D)*60));StartAtlasTravel(D);++Step;}
    if(Step==5&&Time>6.3f){Capture(TEXT("AtlasFadeReview"));++Step;}
    if(Step==6&&Time>8.5f){Capture(TEXT("AtlasArrivalReview"));++Step;}
    if(Step==7&&Time>9.2f)FPlatformMisc::RequestExit(false);
#endif
}
