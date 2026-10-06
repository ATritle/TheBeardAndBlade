#pragma once
#include "CoreMinimal.h"
namespace DungeonProgression
{
    inline constexpr int RoomsPerChapter=4, Chapters=8, CampaignRooms=RoomsPerChapter*Chapters;
    inline constexpr int Bosses[]={24,28,30,25,26,27,29,79};
    inline constexpr int Themes[]={0,6,5,1,2,3,4,7};
    // Keep authored chapter IDs/assets stable. Grease is parked for rework;
    // editor playtests may still address it directly. Toggle to restore it.
    inline constexpr bool IncludeGrease=false;
    inline bool IsEnabled(int Index) { return Index>=0&&Index<Chapters&&(IncludeGrease||Index!=1); }
    inline int NextChapter(int Index) { do { ++Index; } while(Index<Chapters&&!IsEnabled(Index));return Index; }
    inline int ActiveFloor(int Index) { int Floor=0;for(int I=0;I<Index;++I)if(IsEnabled(I))++Floor;return Floor; }
    inline int Chapter(int Room) { return ((FMath::Max(1,Room)-1)/RoomsPerChapter)%Chapters; }
    inline int NextRoom(int Room) { return Room%RoomsPerChapter==0?NextChapter(Chapter(Room))*RoomsPerChapter+1:Room+1; }
    inline int BossRoom(int Species) { for(int I=0;I<Chapters;++I)if(Bosses[I]==Species)return (I+1)*RoomsPerChapter;return RoomsPerChapter; }
    inline bool TraderAfter(int Room) { return Room%RoomsPerChapter==2; }
    inline int RosterBase(int Biome) { const int Bases[]={0,6,12,18,43,37,31,37};return Bases[FMath::Clamp(Biome,0,7)]; }
}
