#pragma once
#include "CoreMinimal.h"

// One result per weapon swing, never one result per enemy in a cleave.
struct FDungeonMeleeCombo
{
    static constexpr float Bonus=1.5f, Window=1.4f, NormalDuration=.48f, FinisherDuration=.66f;
    int32 Landed=0;
    float Remaining=0;
    bool Finisher=false;
    void Reset(){Landed=0;Remaining=0;Finisher=false;}
    void Tick(float Dt,bool Swinging){if(!Swinging){Remaining=FMath::Max(0.f,Remaining-Dt);if(Remaining<=0)Landed=0;}}
    void Begin(){Finisher=Landed==2&&Remaining>0;}
    bool Resolve(bool Hit){
        const bool Completed=Hit&&Finisher;
        Landed=Hit&&!Finisher?FMath::Min(2,Landed+1):0;
        Remaining=Landed?Window:0;
        return Completed;
    }
    float Duration()const{return Finisher?FinisherDuration:NormalDuration;}
    float Impact()const{return Finisher?.56f:1.f-.28f/NormalDuration;}
    float DamageScale()const{return Finisher?Bonus:1.f;}
    // Dedicated eight-key heavy cut: brace, coil, hold overhead, strike, follow-through, recover.
    // Uses the validated right-handed source poses without replacing ordinary attacks.
    static int32 Pose(float T){
        return T<.10f?0:T<.22f?4:T<.52f?1:T<.64f?3:T<.79f?2:T<.90f?4:5;
    }
};
