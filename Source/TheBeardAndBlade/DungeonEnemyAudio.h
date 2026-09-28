#pragma once
#include "CoreMinimal.h"

namespace DungeonEnemyAudio
{
inline bool Human(int Species) { return Species==30||(Species>=37&&Species<=42&&Species!=40); }
inline const TCHAR* Spawn(int Species) { return Species==24?TEXT("Paper"):Species==40?TEXT("DroneFlight"):Human(Species)?TEXT("HumanSpawn"):TEXT("Spawn"); }
inline const TCHAR* Hit(int Species,bool Dead)
{
    if(Species==24)return TEXT("Hit");
    if(Species==26)return Dead?TEXT("Hit"):TEXT("RimePain");
    if(Species==40)return Dead?TEXT("DroneDeath"):TEXT("DronePain");
    if(Human(Species))return Dead?TEXT("Hit"):TEXT("HumanPain");
    return Dead?TEXT("EnemyDeath"):TEXT("EnemyPain");
}
}
