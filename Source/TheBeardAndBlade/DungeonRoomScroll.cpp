#include "DungeonActors.h"
#include "Engine/Canvas.h"

void ADungeonHUD::DrawAtlasScroll(ADungeonGameMode* G,ADungeonHero* H)
{
    const auto Base=Offset;
    const float T=G->AtlasScrollProgress();
    const auto Span=G->AtlasDirection(G->AtlasTravelDoor)*FVector2D(1280,800);
    const FString Art=G->GetBiome()==0?TEXT("AtlasChamber"):FString::Printf(TEXT("AtlasChamber%d"),G->GetBiome());
    auto Layer=[&]()
    {
        Sprite(Art,0,0,1280,800);
        DrawPolishAtmosphere(G,H);
        DrawAtlasDoors(G,H);DrawBreakables(G,H);DrawReward(G,H);DrawPotions(G);DrawCoinDrops(G);
    };
    // Only completed rooms can be left. Render their saved props/loot without
    // re-entering the room, rerolling rewards, or invoking gameplay callbacks.
    {
        const auto& Old=G->AtlasRooms[G->AtlasScrollSource];
        TGuardValue<int32> Current(G->AtlasCurrent,G->AtlasScrollSource);
        TGuardValue<bool> Chest(G->bChest,Old.Chest);
        TGuardValue<FRewardPresentation> Reward(G->Reward,Old.Reward);
        TGuardValue<FDungeonItem> Loot(G->Loot,Old.Loot);
        TGuardValue<TArray<FDungeonBreakable>> Props(G->Breakables,Old.Props);
        TGuardValue<TArray<FDungeonPotion>> Potions(G->Potions,Old.Potions);
        TGuardValue<TArray<FDungeonCoinDrop>> Coins(G->CoinDrops,Old.Coins);
        Offset=Base-Span*T*Scale;Layer();
    }
    Offset=Base+Span*(1-T)*Scale;Layer();
    for(auto& E:G->GetEnemies())if(IsValid(E)&&E->Health>0)Enemy(E);
    Hero(H);
    Offset=Base;
    // Keep scrolling scenery out of letterbox/pillarbox regions; the HUD and
    // cursor stay in screen coordinates and never travel with the chambers.
    const FLinearColor Black(.005f,.008f,.012f);
    DrawRect(Black,0,0,Canvas->SizeX,Base.Y);
    DrawRect(Black,0,Base.Y+800*Scale,Canvas->SizeX,Canvas->SizeY-(Base.Y+800*Scale));
    DrawRect(Black,0,0,Base.X,Canvas->SizeY);
    DrawRect(Black,Base.X+1280*Scale,0,Canvas->SizeX-(Base.X+1280*Scale),Canvas->SizeY);
}
