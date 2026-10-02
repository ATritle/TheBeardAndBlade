#pragma once
#include <algorithm>
#include "DungeonTeaDrink.h"

// Gameplay seconds only: menus, map, trader and transitions pause this clock.
struct FDungeonTeaSpirit
{
    static constexpr float Duration=10.f, Recharge=30.f, Speed=1.5f, SipDuration=DungeonTeaDrink::Duration;
    float Active=0, Cooldown=0, Sip=0;
    bool Ready() const { return Active<=0&&Cooldown<=0; }
    bool Start()
    {
        if(!Ready())return false;
        Active=Duration;Sip=SipDuration;return true;
    }
    void Tick(float Dt)
    {
        Dt=std::max(0.f,Dt);
        Sip=std::max(0.f,Sip-Dt);
        const float Used=std::min(Active,Dt);
        const bool WasActive=Active>0;
        Active=std::max(0.f,Active-Used);
        if(WasActive&&Active<=0)Cooldown=Recharge;
        Cooldown=std::max(0.f,Cooldown-(Dt-Used));
    }
};
