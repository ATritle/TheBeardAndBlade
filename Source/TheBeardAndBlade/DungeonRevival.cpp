#include "DungeonActors.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "CanvasItem.h"
#include "GlobalRenderResources.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

void ADungeonHero::RestoreAfterDeath()
{
    CancelCombatActions();RebuildStats();Health=MaxHealth;Stamina=MaxStamina;
    StunTime=SlowTime=FlashBlindTime=HurtTime=StaminaDelay=0;
    Invulnerable=1.5f;bExhausted=false;Block={};GuardBlend=0;
    WalkDistance=FootstepDistance=MoveBlend=RunBlend=0;GaitTravel=FVector2D::ZeroVector;
    GearHealBudget=MaxHealth*DungeonBalance::GearHealingPerSecond;
    // Preserve charge and cooldowns: paying to revive must not recharge skills.
    if(TeaSpirit.Active>0)TeaSpirit.Cooldown=FMath::Max(TeaSpirit.Cooldown,FDungeonTeaSpirit::Recharge);
    TeaSpirit.Active=TeaSpirit.Sip=0;
}

int64 ADungeonGameMode::RevivalCost() const
{
    const auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));
    return H?DungeonRevival::Cost(H->Coins):0;
}
bool ADungeonGameMode::CanReviveFromDeath() const
{
    const auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));
    return IsDeathSequence()&&H&&H->Health<=0&&H->Coins>0&&bAtlasActive&&
        AtlasRooms.IsValidIndex(DeathReturnRoom)&&AtlasRooms[DeathReturnRoom].Cleared&&
        AtlasRooms[DeathReturnRoom].Type!=EAtlasRoom::Trader;
}
bool ADungeonGameMode::ReviveFromDeath()
{
    if(EndTime<DungeonRevival::ChoiceDelay||!CanReviveFromDeath())return false;
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));
    const int Return=DeathReturnRoom,Failed=AtlasCurrent;
    const int64 Cost=RevivalCost();
    // Commit the payment only after every destination/state check succeeds.
    H->Coins-=Cost;BalanceEvent(TEXT("revived_gold_cost"),float(Cost));
    if(AtlasRooms.IsValidIndex(Failed)&&!AtlasRooms[Failed].Cleared)
        AtlasRooms[Failed].RetryEncounter=true;
    int Door=-1;
    for(int D=0;D<4;++D)if(AtlasRooms[Return].Links[D]==Failed){Door=D;break;}
    EndState=0;EndTime=0;bEndingCleaned=false;DeathReturnRoom=INDEX_NONE;
    bMenu=bShowControls=bAtlasMap=bTraderOpen=false;
    AtlasTravelTime=AtlasArrivalTime=TransitionTime=0;
    H->RestoreAfterDeath();EnterAtlasRoom(Return,Door);
    if(auto* PC=GetWorld()->GetFirstPlayerController())if(auto* HUD=Cast<ADungeonHUD>(PC->GetHUD()))HUD->CancelInventoryGesture();
    PlaySound(TEXT("Magic"),.4f,.8f);
    UE_LOG(LogTemp,Display,TEXT("REVIVAL floor=%d from=%d to=%d cost=%lld remaining=%lld"),AtlasChapter+1,Failed,Return,Cost,H->Coins);
    return true;
}

void ADungeonHUD::DrawDeathSequence(ADungeonGameMode* G,ADungeonHero* H)
{
    const float T=G->GetEndingTime(),In=FMath::Clamp(T/.65f,0.f,1.f);
    const auto P=DungeonView::Project(H->GetActorLocation());
    // Keep the actual dungeon and enemies visible beneath a cold, translucent veil.
    Box(0,0,1280,800,FLinearColor(.015f,.035f,.05f,.64f*In));
    for(int I=0;I<14;++I){
        const float A=.008f*In;
        Box(I*9,0,9,800,FLinearColor(0,0,0,A*(14-I)));
        Box(1280-(I+1)*9,0,9,800,FLinearColor(0,0,0,A*(14-I)));
    }
    Shadow(P,43,.8f);
    // An existing tucked roll pose forms the fallen body; no gameplay sprites change.
    Sprite(TEXT("Athletic_Roll_2_3"),P.X-72,P.Y-63,144,90,FLinearColor(.36f,.43f,.46f,.85f));
    if(!H->Equipment.IsEmpty()&&!H->Equipment[0].IsEmpty())
        Sprite(H->Equipment[0].Art(),P.X+25,P.Y-56,74,74,FLinearColor(.55f,.65f,.69f,.7f),65);

    TArray<FCanvasUVTri> Triangles;
    auto Glow=[&](FVector2D C,float RX,float RY,float Alpha){
        for(int I=0;I<32;++I){
            const float A=2*PI*I/32,B=2*PI*(I+1)/32;
            FCanvasUVTri V;V.V0_Pos=Offset+C*Scale;
            V.V1_Pos=Offset+(C+FVector2D(FMath::Cos(A)*RX,FMath::Sin(A)*RY))*Scale;
            V.V2_Pos=Offset+(C+FVector2D(FMath::Cos(B)*RX,FMath::Sin(B)*RY))*Scale;
            V.V0_UV=V.V1_UV=V.V2_UV=FVector2D::ZeroVector;
            V.V0_Color=FLinearColor(.38f,.82f,1,Alpha);
            V.V1_Color=V.V2_Color=FLinearColor(.38f,.82f,1,0);Triangles.Add(V);
        }
    };
    const float Rise=1-FMath::Exp(-FMath::Max(0.f,T-.25f)*1.25f);
    const auto Soul=P-FVector2D(0,15+68*Rise+FMath::Sin(T*1.8f)*4);
    Glow(Soul-FVector2D(0,68),76,116,.15f*Rise);
    Glow(P-FVector2D(0,5),67,17,.18f*In);
    for(int I=0;I<7;++I){
        const float Age=FMath::Fmod(T*.26f+I/7.f,1.f);
        const auto Q=P+FVector2D(FMath::Sin(Age*5+I*2)* (18+Age*34),-Age*175);
        for(int J=0;J<6;++J)Glow(Q+FVector2D(FMath::Sin(Age*8+J*.4f)*10,J*5),10+J*1.5f,14,.045f*(1-J/6.f)*FMath::Sin(Age*PI)*In);
    }
    FCanvasTriangleItem Mist(Triangles,GWhiteTexture);Mist.BlendMode=SE_BLEND_Translucent;Canvas->DrawItem(Mist);
    // Strip deformation gives the spirit a drifting, tapering lower body, not
    // a rigid transparent copy. Native texture alpha keeps the silhouette clean.
    const FString BodyName=FString::Printf(TEXT("Locomotion_Walk_%d_2"),H->GetFacingDirection());
    UMaterialInstanceDynamic* Spirit=nullptr;
    if(auto* Cached=ArmorMaterials.Find(TEXT("DeathSpirit")))Spirit=*Cached;
    else if(auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/V2/M_DeathSpirit.M_DeathSpirit"))){
        Spirit=UMaterialInstanceDynamic::Create(Base,this);ArmorMaterials.Add(TEXT("DeathSpirit"),Spirit);
    }
    if(Spirit){
        Spirit->SetTextureParameterValue(TEXT("SpriteTexture"),Texture(BodyName));
        Spirit->SetScalarParameterValue(TEXT("SpiritOpacity"),Rise*(.68f+.08f*FMath::Sin(T*2.3f)));
        const float K=1.375f;
        for(int Y=0;Y<128;Y+=2){
            const float Tail=FMath::Clamp((Y-68)/52.f,0.f,1.f);
            const float Drift=FMath::Sin(T*2.2f-Y*.065f)*(1.5f+Tail*8);
            const float Width=128*K*(1-.20f*Tail);
            DrawMaterial(Spirit,Offset.X+(Soul.X-Width/2+Drift)*Scale,
                Offset.Y+(Soul.Y+(Y-116)*K)*Scale,Width*Scale,2*K*Scale,0,Y/128.f,1,2.f/128);
        }
    }
    const float PanelAlpha=FMath::Clamp((T-.65f)/.65f,0.f,1.f);
    if(PanelAlpha<=0)return;
    const FLinearColor Pale(.88f,.95f,1,PanelAlpha),Gold(.92f,.72f,.37f,PanelAlpha),Muted(.53f,.63f,.68f,PanelAlpha);
    Box(350,568,580,193,FLinearColor(.008f,.017f,.023f,.9f*PanelAlpha));
    Sprite(TEXT("InventoryFrame"),340,556,600,218,FLinearColor(.7f,.87f,.9f,PanelAlpha));
    CardText(TEXT("YOU HAVE FALLEN"),376,586,Pale,30,528,40,true);
    CardText(TEXT("The dungeon took your breath. Not your last chance."),375,626,Muted,15,530,24,true);
    const bool Can=G->CanReviveFromDeath(),Ready=T>=DungeonRevival::ChoiceDelay;
    const FString Price=Can?FString::Printf(TEXT("Revive for %lld gold  |  %lld remaining"),G->RevivalCost(),H->Coins-G->RevivalCost()):
        H->Coins<=0?TEXT("Your purse is empty. No revival remains."):TEXT("No safe room is available for revival.");
    CardText(Price,375,654,Can?Gold:FLinearColor(.84f,.49f,.44f,PanelAlpha),18,530,26,true);
    CardText(Can?TEXT("Return to the previous safe room with your equipment."):TEXT("Begin a new descent to try again."),375,679,Muted,13,530,20,true);
    float MX=-100,MY=-100;if(auto* PC=GetOwningPlayerController())PC->GetMousePosition(MX,MY);
    const auto Mouse=(FVector2D(MX,MY)-Offset)/Scale;
    for(int I=0;I<2;++I){
        const float X=377+I*271;const bool Enabled=Ready&&(I||Can);
        const bool Hover=Enabled&&Mouse.X>=X&&Mouse.X<=X+255&&Mouse.Y>=701&&Mouse.Y<=747;
        Box(X+5,705,245,38,FLinearColor(.03f,Hover?.16f:.08f,Hover?.18f:.09f,.95f*PanelAlpha));
        Sprite(TEXT("InventoryFrame"),X,699,255,50,Enabled?(Hover?Pale:Gold):Muted*.45f);
        CardText(I?TEXT("END RUN / NEW DESCENT"):TEXT("REVIVE  [ENTER]"),X+12,715,Enabled?Pale:Muted,15,231,26,true);
    }
}

void ADungeonGameMode::ReviewRevival(float Dt)
{
#if WITH_EDITOR
    if(!FParse::Param(FCommandLine::Get(),TEXT("RevivalReview")))return;
    static float Age=0;static int Frame=-1;static bool Prepared=false,Dead=false,Revived=false;
    if(GetWorld()->GetTimeSeconds()<1)return;
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));if(!H)return;
    if(!Prepared){
        Prepared=true;StartGame();EnterAtlasRoom(1,-1);PendingSpawns=0;
        for(int I=0;I<2;++I){SpawnOneEnemy();Enemies.Last()->SpawnTime=0;Enemies.Last()->SetActorLocation(DungeonView::Unproject({780.+I*110,410.}));}
        H->Coins=6001;H->SetActorLocation(DungeonView::Unproject({570,490}));
    }
    Age+=Dt;
    if(Age>=1&&!Dead){Dead=true;H->Health=0;FinishRun(false);}
    if(Age>=7&&!Revived){Revived=true;ReviveFromDeath();}
    const int N=int(Age*20);
    if(N!=Frame){Frame=N;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("RevivalReview")/FString::Printf(TEXT("frame-%04d.png"),N),true,false);}
    if(Age>=9)FPlatformMisc::RequestExit(false);
#endif
}

void ADungeonGameMode::VerifyRevival()
{
#if WITH_EDITOR
    int Checks=0,Errors=0;auto Check=[&](bool OK,const TCHAR* Message){++Checks;if(!OK){++Errors;UE_LOG(LogTemp,Error,TEXT("REVIVAL FAIL %s"),Message);}};
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));if(!H){FPlatformMisc::RequestExitWithStatus(false,1);return;}
    Check(DungeonRevival::Cost(2499)==2499&&DungeonRevival::Cost(2500)==1250&&DungeonRevival::Cost(2501)==1251,TEXT("explicit threshold examples"));
    Check(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/V2/M_DeathSpirit.M_DeathSpirit"))!=nullptr,TEXT("alpha-safe spectral material available"));
    for(int64 Gold:{int64(0),int64(1),int64(2499),int64(2500),int64(2501),int64(6000),MAX_int64}){
        StartGame();EnterAtlasRoom(1,-1);PendingSpawns=0;SpawnOneEnemy();Enemies.Last()->SpawnTime=0;
        H->Coins=Gold;H->AddToInventory(RollItem(48,2,3));H->Equip(RollItem(0,2,3));
        const auto Gear=H->Equipment[0].Stats();const auto Bag=H->Inventory.Num();
        Breakables[0].BrokenAge=2;Breakables[0].Loot=RollItem(48,2,3);
        CoinDrops.Add({{200,400},17,1});SaveAtlasRoom();
        H->Health=0;FinishRun(false);
        const int64 Expected=Gold<2500?Gold:Gold/2+Gold%2;
        Check(RevivalCost()==Expected,TEXT("threshold and integer rounding"));
        Check(!ReviveFromDeath()&&H->Coins==Gold,TEXT("held input cannot skip death animation or spend early"));
        UpdateEnding(3);Check(Enemies.Num()==1&&IsGameplayBlocked(),TEXT("encounter frozen for death presentation"));
        if(Gold==0){Check(!CanReviveFromDeath()&&!ReviveFromDeath()&&H->Coins==0,TEXT("empty wallet denies revival"));continue;}
        const int Destination=DeathReturnRoom;DeathReturnRoom=INDEX_NONE;
        Check(!ReviveFromDeath()&&H->Coins==Gold,TEXT("invalid destination never spends gold"));DeathReturnRoom=Destination;
        Check(CanReviveFromDeath()&&ReviveFromDeath(),TEXT("funded revival succeeds"));
        Check(H->Coins==Gold-Expected&&H->Coins>=0,TEXT("gold charged exactly once"));
        Check(!ReviveFromDeath()&&H->Coins==Gold-Expected,TEXT("duplicate input cannot charge again"));
        Check(AtlasCurrent==0&&!HasEnding()&&H->Health==H->MaxHealth&&H->Stamina==H->MaxStamina,TEXT("safe room and restored vitals"));
        Check(H->Equipment[0].Stats()==Gear&&H->Inventory.Num()==Bag,TEXT("gear and bag retained"));
        Check(AtlasRooms[1].Visited&&AtlasRooms[1].RetryEncounter&&!AtlasRooms[1].Cleared,TEXT("failed room stays explored and uncleared"));
        TickAtlasTravel(1);EnterAtlasRoom(1,-1);
        Check(PendingSpawns>0&&!AreDoorsOpen(),TEXT("failed encounter restarts, cannot bypass combat"));
        Check(Breakables[0].BrokenAge==2&&Breakables[0].Loot.CatalogId==48&&CoinDrops.Num()==1&&CoinDrops[0].Amount==17,TEXT("props and floor drops persist without reroll"));
        Check(!AtlasRooms[1].RetryEncounter,TEXT("retry consumed once"));
    }
    for(int Floor=0;Floor<DungeonProgression::Chapters;++Floor){
        StartGame();InitializeAtlasFloor(9123,Floor);EnterAtlasRoom(11,-1);
        for(auto& E:Enemies)if(IsValid(E))E->Destroy();Enemies.Empty();PendingSpawns=0;
        AtlasRooms[11].Cleared=true;SaveAtlasRoom();EnterAtlasRoom(5,-1);PendingSpawns=0;SpawnOneEnemy();
        CancelBossIntro();DialogueLines.Empty();BossGrace=0;Enemies.Last()->Health=1;
        H->Coins=8000;H->Health=0;FinishRun(false);UpdateEnding(3);
        Check(ReviveFromDeath()&&AtlasCurrent==11&&AtlasChapter==Floor,TEXT("boss death returns to actual previous room on every floor"));
        TickAtlasTravel(1);EnterAtlasRoom(5,-1);PendingSpawns=0;SpawnOneEnemy();
        Check(Enemies.Num()==1&&Enemies[0]->Health==Enemies[0]->MaxHealth,TEXT("boss retries at full health"));
    }
    StartGame();EnterAtlasRoom(1,-1);PendingSpawns=0;AtlasRooms[1].Cleared=true;SaveAtlasRoom();
    EnterAtlasRoom(6,-1);H->Coins=100000;H->Inventory.Empty();BuyTraderItem(H,0);ContinueFromTrader();TickAtlasTravel(1);
    EnterAtlasRoom(2,-1);FreedomKills=7;H->DrinkTea();H->Tick(FDungeonTeaSpirit::Duration+1);const float Cooldown=H->GetTeaSpiritCooldown();
    H->Coins=2500;H->Health=0;FinishRun(false);UpdateEnding(3);ReviveFromDeath();
    Check(AtlasCurrent==1&&FreedomKills==7&&H->GetTeaSpiritCooldown()==Cooldown,TEXT("previous room and skill progress retained"));
    Check(AtlasRooms[6].StockMade&&AtlasRooms[6].Stock[0].IsEmpty(),TEXT("sold trader stock remains sold after death"));
    StartGame();EnterAtlasRoom(1,-1);H->Coins=1;H->Health=0;FinishRun(false);UpdateEnding(3);ReviveFromDeath();TickAtlasTravel(1);
    EnterAtlasRoom(1,-1);H->Health=0;FinishRun(false);UpdateEnding(3);
    Check(!CanReviveFromDeath(),TEXT("last coin permits only one revival"));
    H->Confirm();Check(HasEnding(),TEXT("enter cannot erase an unaffordable run"));
    RestartFromEnding();Check(!HasEnding()&&H->Coins==0&&H->Inventory.IsEmpty()&&AtlasCurrent==0,TEXT("explicit new run resets progress"));
    H->Health=H->MaxHealth;FinishRun(true);UpdateEnding(3);
    Check(IsVictory()&&!CanReviveFromDeath()&&!ReviveFromDeath(),TEXT("victory flow unchanged"));
    UE_LOG(LogTemp,Display,TEXT("REVIVAL_VERIFY checks=%d errors=%d"),Checks,Errors);
    FPlatformMisc::RequestExitWithStatus(false,Errors?1:0);
#endif
}
