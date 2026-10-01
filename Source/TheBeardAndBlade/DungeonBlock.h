#pragma once
#include "CoreMinimal.h"

// Shared five-second reserve: spent time refills one-for-one while released.
// Partial reserve can be reused immediately; exhaustion requires a full refill.
// A held button is latched even when unavailable: recovery cannot auto-start guard.
struct FDungeonBlock
{
    static constexpr float Limit=5.f;
    bool Active=false,Held=false,Exhausted=false;
    float Elapsed=0,Cooldown=0,Impact=0;
    void Press(bool Allowed)
    {
        if(Held)return;
        Held=true;
        if(Allowed&&!Exhausted&&Cooldown<Limit){Active=true;Elapsed=Cooldown;Cooldown=0;}
    }
    void Stop()
    {
        if(!Active)return;
        Active=false;Cooldown=FMath::Clamp(Elapsed,0.f,Limit);Elapsed=0;
    }
    void Release(){Stop();Held=false;}
    void Tick(float Dt)
    {
        Dt=FMath::Max(0.f,Dt);Impact=FMath::Max(0.f,Impact-Dt);
        if(Active){
            const float Step=FMath::Min(Dt,Limit-Elapsed);
            Elapsed+=Step;Dt-=Step;
            if(Elapsed>=Limit-KINDA_SMALL_NUMBER){Elapsed=Limit;Exhausted=true;Stop();}
        }
        if(!Active){
            Cooldown=FMath::Max(0.f,Cooldown-Dt);
            if(Cooldown<=KINDA_SMALL_NUMBER){Cooldown=0;Exhausted=false;}
        }
    }
    float Fraction() const{return 1.f-FMath::Clamp((Active?Elapsed:Cooldown)/Limit,0.f,1.f);}
    static bool InFront(FVector2D Facing,FVector2D ToAttacker)
    {
        // A source at the same anchor has no defensible direction; side-on is not front.
        return !ToAttacker.IsNearlyZero()&&FVector2D::DotProduct(Facing.GetSafeNormal(),ToAttacker.GetSafeNormal())>1.e-6;
    }
};
