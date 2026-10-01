#include "DungeonActors.h"
#include "DungeonRoster.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Texture2D.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "HAL/PlatformMisc.h"

void ADungeonGameMode::VerifyHotbar()
{
#if !UE_BUILD_SHIPPING
    int Checks=0,Errors=0;
    auto Check=[&](bool OK,const TCHAR* Name){++Checks;if(!OK)++Errors;UE_LOG(LogTemp,Display,TEXT("HOTBAR %s %s"),OK?TEXT("PASS"):TEXT("FAIL"),Name);};
    for(float Seconds:{.01f,.5f,1.f,3.f,4.9f,5.f})for(float FPS:{15.f,30.f,60.f,120.f}){
        FDungeonBlock B;B.Press(true);float T=0;
        while(T<Seconds){float Step=FMath::Min(1/FPS,Seconds-T);B.Tick(Step);T+=Step;}
        B.Release();
        Check(FMath::IsNearlyEqual(B.Cooldown,Seconds,.001f),TEXT("hold time equals cooldown at every frame rate"));
        Check(FMath::IsNearlyEqual(B.Fraction(),1-Seconds/5,.001f),TEXT("remaining meter continues into refill without jumping"));
        B.Press(true);Check(B.Active==(Seconds<5),TEXT("partial reserve immediately reusable; exhausted reserve locked"));
        Check(FMath::IsNearlyEqual(B.Fraction(),1-Seconds/5,.001f),TEXT("repress preserves reserve without resetting"));
        B.Stop();
        B.Tick(Seconds+.001f);B.Press(true);Check(!B.Active&&B.Cooldown==0,TEXT("held RMB cannot automatically restart guard"));
        B.Release();B.Press(true);Check(B.Active,TEXT("fresh press after full recovery succeeds"));
    }
    {FDungeonBlock B;B.Press(true);B.Tick(5.25f);Check(!B.Active&&FMath::IsNearlyEqual(B.Cooldown,4.75f),TEXT("five second exhaustion handles frame overshoot"));}
    for(float FPS:{15.f,30.f,60.f,120.f}){
        FDungeonBlock B;auto Advance=[&](float Seconds){float T=0;while(T<Seconds){const float Dt=FMath::Min(1/FPS,Seconds-T);B.Tick(Dt);T+=Dt;}};
        B.Press(true);Advance(2);B.Release();B.Press(true);
        Check(B.Active&&FMath::IsNearlyEqual(B.Elapsed,2,.001f),TEXT("two seconds then immediate repress retains three seconds"));
        Advance(2.9f);Check(B.Active,TEXT("remaining reserve works without refill wait"));Advance(.1f);
        Check(!B.Active&&B.Exhausted&&FMath::IsNearlyEqual(B.Cooldown,5,.002f),TEXT("two plus three exhausts shared reserve"));
        B.Release();Advance(1);B.Press(true);Check(!B.Active&&B.Exhausted,TEXT("exhaustion still locks partial refill"));
        B.Release();Advance(4);B.Press(true);Check(B.Active,TEXT("exhausted reserve unlocks after full refill"));
        B=FDungeonBlock();B.Press(true);Advance(2);B.Release();Advance(1);B.Press(true);
        Check(B.Active&&FMath::IsNearlyEqual(B.Elapsed,1,.001f)&&FMath::IsNearlyEqual(B.Fraction(),.8f,.001f),TEXT("one second rest restores one second of reserve"));
    }
    for(int D=0;D<8;++D){const float A=D*PI/4;const FVector2D Aim(FMath::Sin(A),-FMath::Cos(A));
        Check(FDungeonBlock::InFront(Aim,Aim*70),TEXT("all eight front directions block"));
        Check(!FDungeonBlock::InFront(Aim,-Aim*70),TEXT("all eight rear directions bypass"));
        Check(!FDungeonBlock::InFront(Aim,FVector2D(-Aim.Y,Aim.X)*70),TEXT("side boundary is not in front"));
    }
    Check(!FDungeonBlock::InFront({0,1},{0,0}),TEXT("overlapped source has no front direction"));
    StartGame();CancelBossIntro();DialogueLines.Empty();BossGrace=0;AtlasArrivalTime=AtlasTravelTime=0;bAtlasMap=bTraderOpen=false;
    PendingSpawns=0;bChest=bLootClaimed=false;
    for(auto& E:Enemies)if(IsValid(E))E->Destroy();Enemies.Empty();
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));Check(H!=nullptr,TEXT("hero exists"));
    if(H){
        const FVector2D P(640,500);
        auto Reset=[&](){H->Restart();H->Armor=H->DamageReduction=0;H->SetActorLocation(DungeonView::Unproject(P));H->SetFlashReviewAim({0,-1});FreedomTime=0;FreedomKills=15;};
        for(int D=0;D<8;++D){
            Reset();const float A=D*PI/4;const FVector2D Aim(FMath::Sin(A),-FMath::Cos(A));H->SetFlashReviewAim(Aim);H->BlockPressed();
            Check(H->ReceiveMeleeHit(35,P+Aim*60)&&H->Health==150&&H->Stamina==100,TEXT("live melee guard absorbs all damage without stamina"));
            Check(!H->ReceiveMeleeHit(35,P-Aim*60)&&H->Health==115,TEXT("live rear melee still damages"));
            Reset();H->SetFlashReviewAim(Aim);H->BlockPressed();H->ReceiveHit(35,true,P+Aim*60);
            Check(H->Health==150&&H->Stamina==100,TEXT("all eight front projectile directions block without stamina"));
            H->ReceiveHit(35,true,P-Aim*60);Check(H->Health==115,TEXT("all eight rear projectile directions bypass"));
        }
        Reset();H->BlockPressed();H->ReceiveHit(35,true,P+FVector2D(0,-60));Check(H->Health==150,TEXT("front bullets are blocked"));
        Reset();H->BlockPressed();H->ReceiveHit(35,true,P+FVector2D(0,60));Check(H->Health==115,TEXT("rear bullets still damage"));
        Reset();H->BlockPressed();FDungeonShot Shot;Shot.Damage=35;Shot.BlastRadius=100;ResolveProjectile(Shot,P);Check(H->Health==150,TEXT("centered explosions are blocked"));
        Reset();H->BlockPressed();ResolveProjectile(Shot,P+FVector2D(0,-40));Check(H->Health==150,TEXT("front explosion blocked"));
        Reset();H->BlockPressed();ResolveProjectile(Shot,P+FVector2D(0,40));Check(H->Health==115,TEXT("rear explosion still damages"));
        for(int Style:{10,11,14,15}){
            Reset();H->BlockPressed();FDungeonShot S;S.Style=Style;S.Damage=35;S.Radius=100;S.BlastRadius=100;S.Origin=P+FVector2D(0,-100);S.Velocity={0,100};S.bPlayerBodyHit=true;
            ResolveProjectile(S,P);Check(H->Health==150&&H->StunTime==0,TEXT("projectile styles block damage and on-hit status"));
        }
        Reset();H->BlockPressed();Check(!H->ApplyFlashBang(P+FVector2D(0,-40),175)&&H->Health==150&&H->StunTime==0&&H->FlashBlindTime==0,TEXT("guard stops flash damage blind and stun"));
        Reset();H->BlockPressed();H->ReceiveFlashStab(P+FVector2D(0,-40));Check(H->Health==121.875f,TEXT("front boss percent stab reduced by 25 percent"));
        Reset();H->BlockPressed();H->ReceiveFlashStab(P+FVector2D(0,40));Check(H->Health==112.5f,TEXT("rear percent stab preserved"));
        Reset();H->BlockPressed();Shot.bBossAttack=true;Shot.bMelee=true;Shot.MeleeOrigin=P+FVector2D(0,-60);ResolveProjectile(Shot,P);Check(H->Health==123.75f,TEXT("boss close slash loses only 25 percent damage"));
        for(int Style:{1,10,11,14,15}){
            Reset();H->BlockPressed();FDungeonShot S;S.bBossAttack=true;S.Style=Style;S.Damage=40;S.Radius=S.BlastRadius=100;S.Origin=P+FVector2D(0,-100);S.Velocity={0,100};S.bPlayerBodyHit=true;
            ResolveProjectile(S,P);Check(H->Health==120&&H->Stamina==100,TEXT("boss projectile and blast guard passes 75 percent"));
        }
        Reset();H->BlockPressed();H->ApplyFlashBang(P+FVector2D(0,-40),175,true);
        Check(H->Health<150&&H->FlashBlindTime==0,TEXT("boss flash guard takes damage but prevents blind"));
        Reset();H->Armor=20;H->DamageReduction=.2f;H->BlockPressed();H->ReceiveHit(40,true,P+FVector2D(0,-40),true);
        Check(FMath::IsNearlyEqual(H->Health,150-(40-6)*.8f*.75f),TEXT("boss block removes exactly one quarter after gear mitigation"));
        Reset();H->BlockPressed();H->Attack();H->PowerMove();H->Freedom();Check(!H->IsAttacking()&&!H->IsCasting()&&FreedomTime==0,TEXT("guard rejects conflicting offensive actions"));
        Reset();H->Attack();H->BlockPressed();Check(!H->IsBlocking(),TEXT("sword recovery prevents guard"));
        Reset();H->PowerMove();H->BlockPressed();Check(!H->IsBlocking(),TEXT("tea cast prevents guard"));
        Reset();H->BlockPressed();H->Tick(3);H->BlockReleased();Check(FMath::IsNearlyEqual(H->GetBlockCooldown(),3)&&H->Stamina==100,TEXT("live three second release; no stamina charge"));
        H->Tick(2);Check(FMath::IsNearlyEqual(H->GetBlockCooldown(),1),TEXT("live recovery counts down"));H->Tick(1);Check(H->CanStartBlock(),TEXT("recovery enables block exactly at zero"));
        Reset();H->BlockPressed();H->Tick(2);H->BlockReleased();Check(H->CanStartBlock(),TEXT("live partial meter immediately available"));
        H->BlockPressed();Check(H->IsBlocking()&&FMath::IsNearlyEqual(H->GetBlockCountdown(),3),TEXT("live repress starts with three seconds remaining"));
        H->Tick(3);Check(!H->IsBlocking()&&FMath::IsNearlyEqual(H->GetBlockCooldown(),5),TEXT("live combined holds exhaust reserve"));
        Reset();H->BlockPressed();H->Tick(5);Check(!H->IsBlocking()&&FMath::IsNearlyEqual(H->GetBlockCooldown(),5),TEXT("live five second cap"));
        H->Tick(5);H->BlockPressed();Check(!H->IsBlocking(),TEXT("live full hold requires release"));
        Reset();H->BlockPressed();H->Tick(.5f);H->ToggleInventory();Check(!H->IsBlocking()&&H->GetBlockCooldown()==.5f,TEXT("inventory cancels block immediately"));H->Tick(3);Check(H->GetBlockCooldown()==.5f,TEXT("inventory pauses cooldown"));H->ToggleInventory();
        Reset();H->BlockPressed();H->Tick(.5f);H->Dodge();Check(!H->IsBlocking()&&H->IsRolling()&&H->GetBlockCooldown()==.5f,TEXT("dodge cancels guard with proportional recovery"));
        Reset();H->BlockPressed();H->Tick(.5f);H->StunTime=1;H->Tick(.1f);Check(!H->IsBlocking()&&H->GetBlockCooldown()>0,TEXT("stun interrupts guard"));
        Reset();H->BlockPressed();H->Tick(.5f);H->ReceiveHit(10000,true,P+FVector2D(0,60));Check(!H->IsBlocking()&&H->Health==0,TEXT("rear lethal damage clears guard"));
        Reset();H->BlockPressed();H->Tick(.2f);Check(H->GetGuardBlend()==1,TEXT("guard reaches braced pose"));H->BlockReleased();H->Tick(.2f);Check(H->GetGuardBlend()==0,TEXT("guard animation returns to locomotion"));
        Reset();H->BlockPressed();H->Tick(.5f);ToggleMenu();Check(!H->IsBlocking(),TEXT("pause cancels guard immediately"));ToggleMenu();
        Reset();H->BlockPressed();H->Tick(.5f);ToggleAtlasMap();Check(!H->IsBlocking(),TEXT("map cancels guard immediately"));ToggleAtlasMap();
        Reset();H->BlockPressed();H->Tick(.5f);H->Restart();Check(H->GetBlockCooldown()==0&&!H->IsBlocking()&&H->CanStartBlock(),TEXT("new run resets guard and latch"));
        // Exercise the real attack dispatchers, not just the helper.
        for(int Species:{0,4,12,31,35,38,41,45,47,49,51,53,68,75}){
            Reset();H->BlockPressed();auto* E=GetWorld()->SpawnActor<ADungeonEnemy>();E->Species=Species;E->SpawnTime=0;E->Health=100;
            E->SetActorLocation(DungeonView::Unproject(P+FVector2D(0,-50)));E->AttackTarget=P;E->ChargeAim={0,1};
            if(Species==49){E->RustAttackAge=RustbladeSquire::StrikeTime-.01f;E->bRustStrikeFired=false;E->TickRustblade(.02f,H,this);}
            else if(Species>=50){E->bExpansionRanged=false;ReleaseExpansion(E,H);}
            else if(Species==38){E->ChargeTime=.1f;E->Tick(.1f);}
            else FireAttack(E);
            Check(H->Health==150&&H->GetBlockImpact()>0,TEXT("enemy melee dispatch reaches block"));E->Destroy();
        }
    }
    for(const TCHAR* Name:{TEXT("OrbFrame"),TEXT("Slot"),TEXT("Attack"),TEXT("Block"),TEXT("Freedom"),TEXT("Wrap"),TEXT("Lightning"),TEXT("Chassis"),TEXT("Gem")}){
        const auto Asset=FString::Printf(TEXT("Hotbar_%s"),Name);
        auto* T=LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Art/UI/Hotbar/%s.%s"),*Asset,*Asset));
        Check(T&&T->NeverStream,TEXT("transparent HUD texture imported and resident"));
    }
    UE_LOG(LogTemp,Display,TEXT("HOTBAR_VERIFY_COMPLETE checks=%d errors=%d"),Checks,Errors);
    FPlatformMisc::RequestExitWithStatus(true,Errors?1:0);
#endif
}

void ADungeonGameMode::UpdateHotbarReview(float Dt)
{
#if !UE_BUILD_SHIPPING
    static bool Started=false;static float Clock=0;static int Capture=-1;
    const bool Auto=FParse::Param(FCommandLine::Get(),TEXT("HotbarReview"));
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));if(!H)return;
    auto* PC=UGameplayStatics::GetPlayerController(this,0);
    if(!Started||(PC&&PC->WasInputKeyJustPressed(EKeys::R))){
        Started=true;Clock=0;StartGame();CancelBossIntro();DialogueLines.Empty();BossGrace=0;AtlasArrivalTime=AtlasTravelTime=0;
        PendingSpawns=0;bChest=bLootClaimed=false;
        for(auto& E:Enemies)if(IsValid(E))E->Destroy();Enemies.Empty();Shots.Empty();Splashes.Empty();
        H->Restart();H->SetActorLocation(DungeonView::Unproject({640,550}));H->SetFlashReviewAim({0,-1});FreedomKills=15;
        for(int I=0;I<(Auto?1:2);++I){auto* E=GetWorld()->SpawnActor<ADungeonEnemy>();E->Species=I?50:49;E->SpawnTime=.5f;E->Health=E->MaxHealth=500;
            E->SetActorLocation(DungeonView::Unproject(I?FVector2D(860,320):FVector2D(640,480)));Enemies.Add(E);}
    }
    if(!Auto)return;
    int32 ChargePreview=-1;
    if(FParse::Value(FCommandLine::Get(),TEXT("HotbarCharge="),ChargePreview))FreedomKills=FMath::Clamp(ChargePreview,0,15);
    const float Before=Clock;Clock+=Dt;
    auto Cross=[&](float T){return Before<T&&Clock>=T;};
    H->SetFlashReviewAim(Clock>=11.2f&&Clock<12.2f?FVector2D(0,1):FVector2D(0,-1));
    if(FParse::Param(FCommandLine::Get(),TEXT("GuardDirections"))){
        const float Angle=(FMath::FloorToInt(Clock*2)%8)*PI/4;
        H->SetFlashReviewAim({FMath::Sin(Angle),-FMath::Cos(Angle)});
    }
    if(Cross(1))H->BlockPressed();
    if(Cross(6.1f))H->BlockReleased();
    if(Cross(11.2f)){H->Health=H->MaxHealth;H->BlockPressed();}
    if(Cross(14.2f))H->BlockReleased();
    if(Cross(17.4f)){H->Health=H->MaxHealth;H->PowerMove();H->SpendStamina(60);}
    if(Cross(19))H->Attack();
    if(Cross(21))H->Dodge();
    if(Cross(23)){H->Freedom();}
    // Keep this automated showcase alive; the manual arena uses normal player health.
    if(H->Health<35)H->Health=H->MaxHealth;
    float HealthPreview=-1;
    if(FParse::Value(FCommandLine::Get(),TEXT("HotbarHealth="),HealthPreview))
        H->Health=H->MaxHealth*FMath::Clamp(HealthPreview,.01f,1.f);
    const int Frame=FMath::FloorToInt(Clock*10);
    FString CaptureFolder=TEXT("Saved/HotbarReviewPolished");FParse::Value(FCommandLine::Get(),TEXT("HotbarCapture="),CaptureFolder);
    const bool Still=FParse::Param(FCommandLine::Get(),TEXT("HotbarStill"));
    if(Frame!=Capture&&Clock>(Still?4.f:.4f)){
        Capture=Frame;FScreenshotRequest::RequestScreenshot(FPaths::ProjectDir()/CaptureFolder/FString::Printf(TEXT("frame-%05d.png"),Frame),true,false);
    }
    if(Clock>(Still?4.5f:27.f))FPlatformMisc::RequestExit(false);
#endif
}
