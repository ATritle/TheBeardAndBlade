#include "DungeonBlock.h"
#include "DungeonActors.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    bool CombatAvailable(const ADungeonHero* H)
    {
        const auto* G=Cast<ADungeonGameMode>(UGameplayStatics::GetGameMode(H));
        return G&&!G->IsGameplayBlocked()&&!H->IsInventoryOpen()&&H->Health>0&&H->StunTime<=0;
    }
}
bool ADungeonHero::CanStrike() const
{
    return CombatAvailable(this)&&!IsBlocking()&&!IsRolling()&&!IsCasting()&&!IsAttacking()&&!IsDrinkingTea();
}
bool ADungeonHero::CanUseTea() const
{
    return CanStrike()&&PowerCooldown<=0;
}
bool ADungeonHero::CanUseFreedom() const
{
    const auto* G=Cast<ADungeonGameMode>(UGameplayStatics::GetGameMode(this));
    return CombatAvailable(this)&&!IsBlocking()&&!IsDrinkingTea()&&G&&G->IsFreedomReady();
}
bool ADungeonHero::CanStartBlock() const
{
    return CanStrike()&&!Block.Exhausted&&Block.Cooldown<FDungeonBlock::Limit&&!Block.Held;
}
void ADungeonHero::BlockPressed()
{
    Block.Press(CanStartBlock());
    if(Block.Active)Facing=DungeonView::Direction(Aim);
}
bool ADungeonHero::TryBlockDamage(FVector2D ToSource)
{
    // Unlocalized damage (or an explosion centered exactly on the hero) has no
    // rear direction. Directional attacks still require facing the source.
    if(CombatAvailable(this)&&IsBlocking()&&(ToSource.IsNearlyZero()||FDungeonBlock::InFront(GetVisualFacing(),ToSource))){
        if(Block.Impact<=0)if(auto* G=Cast<ADungeonGameMode>(UGameplayStatics::GetGameMode(this)))G->PlaySound(TEXT("Hit"),.3f,1.15f);
        Block.Impact=.18f;
        return true;
    }
    return false;
}
bool ADungeonHero::ReceiveMeleeHit(float Damage,FVector2D Attacker,bool BossAttack)
{
    const bool Guarded=TryBlockDamage(Attacker-DungeonView::Project(GetActorLocation()));
    ReceiveHit(Damage,false,Attacker,BossAttack);
    return Guarded;
}
