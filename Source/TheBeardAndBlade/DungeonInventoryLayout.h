#pragma once
#include "CoreMinimal.h"
namespace DungeonInventoryLayout {
constexpr float BagX=456,BagY=178,CellSize=60;
inline FVector2D Gear(int S) { return {S%2==0?40.:320.,S>=4?(S<6?166.:282.):(S<2?430.:560.)}; }
inline bool In(FVector2D P,float X,float Y,float W,float H) {return P.X>=X&&P.X<X+W&&P.Y>=Y&&P.Y<Y+H;}
inline int GearAt(FVector2D P) {for(int S=0;S<8;++S){auto G=Gear(S);if(In(P,G.X,G.Y,80,80))return S;}return INDEX_NONE;}
inline FIntPoint BagCell(FVector2D P) {return FIntPoint(FMath::FloorToInt((P.X-BagX)/CellSize),FMath::FloorToInt((P.Y-BagY)/CellSize));}
}
