#include "../Source/TheBeardAndBlade/DungeonTeaSpirit.h"
#include <cassert>
#include <cmath>
#include <cstdio>
int main()
{
    FDungeonTeaSpirit T;
    assert(T.Ready()&&T.Start()&&!T.Ready()&&!T.Start());
    T.Tick(FDungeonTeaSpirit::SipDuration);assert(T.Sip==0&&std::abs(T.Active-8.8f)<.001f&&T.Cooldown==0);
    T.Tick(8.55f);assert(std::abs(T.Active-.25f)<.001f);
    T.Tick(.5f);assert(T.Active==0&&T.Cooldown==29.75f&&!T.Start());
    T.Tick(29.75f);assert(T.Ready()&&T.Start());
    T.Tick(100);assert(T.Ready());
    for(int FPS:{30,60,120,240}){
        T={};T.Start();
        for(int I=0;I<FPS*9;++I)T.Tick(1.f/FPS);
        assert(std::abs(T.Active-1.f)<.001f&&T.Cooldown==0&&!T.Start());
        for(int I=0;I<FPS*2;++I)T.Tick(1.f/FPS);
        assert(T.Active==0&&std::abs(T.Cooldown-29.f)<.003f);
    }
    T={};T.Start();T.Tick(-1);assert(T.Active==10&&T.Sip==FDungeonTeaSpirit::SipDuration);
    T={};assert(T.Ready()&&T.Active==0&&T.Sip==0);
    static_assert(FDungeonTeaSpirit::Speed==1.5f);
    for(int I=0;I<8;++I){
        const float Start=I?DungeonTeaDrink::Ends[I-1]:0;
        assert(DungeonTeaDrink::Frame(Start+.001f)==I);
        assert(DungeonTeaDrink::Frame(DungeonTeaDrink::Ends[I]-.001f)==I);
        const auto R=DungeonTeaDrink::Regions[I];
        assert(R.X>=0&&R.Y>=0&&R.X+R.W<=1536&&R.Y+R.H<=1024);
    }
    for(int I=0;I<164;++I)
        assert(DungeonTeaDrink::AuraRadius((I+1)*.01f)>DungeonTeaDrink::AuraRadius(I*.01f));
    std::puts("Tea Spirit timing, re-entry, expiry, long-frame and reset tests passed.");
}
