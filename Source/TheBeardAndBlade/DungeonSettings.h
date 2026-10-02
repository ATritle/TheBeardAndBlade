#pragma once
#include "CoreMinimal.h"
#include "InputCoreTypes.h"

namespace DungeonKeys
{
enum Action { Up,Down,Left,Right,Attack,Throw,Block,Freedom,Tea,Potion,Interact,Inventory,Dodge,Sprint,Map,Pause,Count };
const TCHAR* Name(int Action);
FKey Key(int Action);
FString Label(int Action);
void Load(bool Force=false);
bool Set(int Action,FKey Key,FString& Error,bool Save=true);
void Reset(bool Save=true);
}
