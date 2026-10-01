// Standalone numeric regression checks, NOT a simulated or recorded playthrough.
// Build with a C++17 compiler; uses exactly the runtime balance formulas.
#include "../Source/TheBeardAndBlade/DungeonBalance.h"
#include <cstdio>
#include <cstdlib>
#include <random>

int main()
{
    using namespace DungeonBalance;
    int Checks=0,Errors=0;
    auto Check=[&](bool OK,const char* Why){++Checks;if(!OK){++Errors;std::fprintf(stderr,"FAIL: %s\n",Why);}};
    const Role Roles[]={Role::Nuisance,Role::Ranged,Role::Melee,Role::Heavy,Role::Elite,Role::Boss};
    for(int F=0;F<8;++F)for(int D=1;D<=9;++D){
        Check(LootLevel(F,D)>=1&&LootLevel(F,D)<=25,"bounded loot level");
        for(auto R:Roles){
            const float HP=EnemyHealth(R,F,D);
            Check(HP>0&&std::isfinite(HP),"finite positive enemy health");
            if(D<9)Check(EnemyHealth(R,F,D+1)>HP,"depth increases health");
            if(F<7)Check(EnemyHealth(R,F+1,D)>HP,"floor increases health");
        }
        Check(EnemyHealth(Role::Melee,F,D)>EnemyHealth(Role::Ranged,F,D),"ranged vulnerable in close combat");
        Check(EnemyHealth(Role::Heavy,F,D)>EnemyHealth(Role::Melee,F,D),"heavy role distinct");
        Check(EnemyHealth(Role::Elite,F,D)>EnemyHealth(Role::Heavy,F,D),"elite role distinct");
        Check(AttackSlots(F,D)>=2&&AttackSlots(F,D)<=3,"bounded simultaneous windups");
        Check(AttackStartGap(F)>=.19f,"attack starts staggered");
    }
    for(int Armor=0;Armor<1000;++Armor){
        const float A=ArmorReduction(float(Armor));
        Check(A>=0&&A<=.6f,"armor bounded");
        Check(ArmorReduction(float(Armor+1))>=A,"more armor never worse");
        if(Armor<100)Check(ArmorReduction(float(Armor+2))-ArmorReduction(float(Armor+1))<=ArmorReduction(float(Armor+1))-A+.000001f,"diminishing returns");
        Check(Mitigated(20,float(Armor),.3f,true)>5.f,"stacked mitigation cannot trivialize damage");
    }
    Check(Mitigated(-20,0,0,false)==0,"negative damage rejected");
    Check(Mitigated(100,-20,-1,false)==100,"negative defense clamped");
    Check(std::abs(Mitigated(100,20,.2f,false)*.75f/Mitigated(100,20,.2f,false)-.75f)<.00001f,"boss guard leaves 75 percent");
    Check(PotionFraction<.35f&&PotionChance<.12f,"less potion sustain");
    Check(GearHealingPerSecond*150<HarvestHealing*10,"mass kills share healing cap");
    Check(StaggerCooldown>1.75f,"stagger guard outlasts hurt recovery and elite windup");
    std::mt19937 Random(4201);std::uniform_real_distribution<float> Unit(0,1);
    for(int F=0;F<8;++F){
        int Rarities[5]={},BossLegendary=0;
        const int Level=LootLevel(F,2);
        for(int I=0;I<10000;++I){
            const float Roll=Unit(Random);
            const int R=ChestRarity(Roll,Level),B=ChestRarity(Roll,Level,true),T=TraderRarity(Roll,Level,true);
            Check(R>=0&&R<=4&&B>=3&&B<=4&&T>=2&&T<=4,"bounded loot tiers");
            ++Rarities[R];BossLegendary+=B==4;
        }
        Check(BossLegendary>1800&&BossLegendary<2200,"boss jackpot about 20 percent");
        if(F<3)Check(Rarities[4]==0,"no early random legendary flood");
        Check(Rarities[0]+Rarities[1]>4000,"ordinary drops remain meaningful baseline");
    }
    // Three explicit hypothetical gear profiles, not a loot/drop simulation:
    // sparse route: uncommon low-power weapon; typical: rare medium weapon;
    // lucky: legendary high-power weapon and more speed/crit affixes.
    std::puts("projection_only,floor,loadout,hit,dps,standard_hits,elite_seconds_50pct_uptime,boss_seconds_50pct_uptime");
    for(int F=0;F<8;++F)for(int Path=0;Path<3;++Path){
        const int Rarity=Path==0?1:Path==1?2:4;
        const int Level=LootLevel(F,6);
        const float Base=Path==0?10.f:Path==1?13.f:20.f;
        const float Hit=24+Base*PrimaryScale(Level,Rarity);
        const float Speed=Path==0?1.05f:Path==1?1.20f:1.45f;
        const float Crit=Path==0?1.04f:Path==1?1.12f:1.25f;
        const float DPS=Hit*Speed/.48f*Crit;
        const float Hits=EnemyHealth(Role::Melee,F,6)/Hit;
        const float Elite=EnemyHealth(Role::Elite,F,6)/(DPS*.5f);
        const float Boss=EnemyHealth(Role::Boss,F,9)/(DPS*.5f);
        Check(Hits>1.5f&&Hits<7,"gear profiles retain regular combat");
        Check(Boss>30&&Boss<240,"gear profiles avoid extreme boss duration");
        std::printf("projection,%d,%s,%.1f,%.1f,%.1f,%.1f,%.1f\n",F+1,
            Path==0?"sparse":Path==1?"typical":"lucky",Hit,DPS,Hits,Elite,Boss);
    }
    std::printf("BALANCE_PROBE checks=%d errors=%d\n",Checks,Errors);
    return Errors?EXIT_FAILURE:EXIT_SUCCESS;
}
