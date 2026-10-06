#include "DungeonActors.h"
#include "RustbladeSquire.h"
#include "DungeonExpansion.h"
#include "DungeonCombatBalance.h"
#include "Kismet/GameplayStatics.h"

void ADungeonHero::Freedom()
{
    if(auto* G=Cast<ADungeonGameMode>(UGameplayStatics::GetGameMode(this))) G->ActivateFreedom(this);
}

bool ADungeonGameMode::ActivateFreedom(ADungeonHero* H)
{
    if(!H||!H->CanUseFreedom()) return false;
    FreedomKills=0; FreedomTime=3.f; bFreedomResolved=false;
    H->CancelCombatActions();
    Shots.Empty();
    PlaySound(TEXT("EagleScreech"),1.f);
    // Voiceover intentionally deferred: the local speech engine denied synthesis.
    return true;
}

void ADungeonGameMode::UpdateFreedom(float Dt)
{
    if(!IsFreedomActive()) return;
    const float Remaining=FMath::Max(0.f,FreedomTime-Dt);
    // Resolve once, during the sweep. Snapshot protects against death callbacks mutating Enemies.
    if(!bFreedomResolved&&Remaining<=1.65f)
    {
        bFreedomResolved=true; Shots.Empty();
        PlaySound(TEXT("Explosion"),1.f);
        const auto Victims=Enemies;
        for(auto& E:Victims) if(IsValid(E))
        {
            if(E->bBoss) { E->FreedomImmuneTime=2.5f; continue; }
            const float Damage=DungeonCombatBalance::FreedomDamage(E->Health,E->MaxHealth,false);
            E->SpawnTime=0;
            E->TakeDungeonDamage(Damage);
        }
        CompleteRoom();
    }
    FreedomTime=Remaining;
}

void ADungeonHUD::DrawCombatFX(ADungeonGameMode* G,bool Foreground)
{
    for(const auto& B:G->Blood)
    {
        if(DungeonExpansion::Is(B.ExpansionSpecies)){
            if(!Foreground&&B.Age<5){
                const float S=DungeonExpansion::Size(B.ExpansionSpecies),Alpha=FMath::Clamp((5-B.Age)/1.4f,0.f,1.f);
                Sprite(DungeonExpansion::Art(B.ExpansionSpecies,B.ExpansionDirection,TEXT("death"),FMath::Clamp(int(B.Age/.14f),0,DungeonExpansion::Count(B.ExpansionSpecies,TEXT("death"))-1)),B.Position.X-S*.5f,B.Position.Y-S*DungeonExpansion::Root(B.ExpansionSpecies),S,S,FLinearColor(1,1,1,Alpha));
            }continue;
        }
        if(B.RustDirection>=0) {
            if(!Foreground&&B.Age<5.f) {
                const float Size=RustbladeSquire::RenderSize;
                const float Alpha=FMath::Clamp((5.f-B.Age)/1.4f,0.f,1.f);
                Sprite(RustbladeSquire::Art(B.RustDirection,TEXT("death"),RustbladeSquire::DeathFrame(B.Age)),B.Position.X-Size*.5f,B.Position.Y-Size*RustbladeSquire::RootY,Size,Size,FLinearColor(1,1,1,Alpha));
            }
            continue;
        }
        if(!Foreground)
        {
            const float Alpha=FMath::Min(1.f,B.Age/.3f)*FMath::Clamp((14.f-B.Age)/4.f,0.f,1.f);
            const FString Art=B.bRemains?FString::Printf(TEXT("Remains_%d"),B.Variant):FString::Printf(TEXT("Blood_%d"),4+B.Variant);
            Sprite(Art,B.Position.X-B.Size*.5f,B.Position.Y-B.Size*.5f,B.Size,B.Size,FLinearColor(1,1,1,Alpha));
        }
        else if(!B.bRemains&&B.Age<.4f)
            Sprite(FString::Printf(TEXT("Blood_%d"),FMath::Min(3,int(B.Age*10))),B.Position.X-B.Size*.5f,B.Position.Y-B.Size,B.Size,B.Size,FLinearColor::White);
    }
    if(Foreground&&G->IsFreedomActive())
    {
        const float T=G->FreedomProgress(),X=FMath::Lerp(-220.f,1500.f,T),Y=280.f+FMath::Sin(T*PI*2)*55.f;
        Sprite(FString::Printf(TEXT("Eagle_%d"),int(T*36)%8),X-180,Y-150,360,300);
    }
}
