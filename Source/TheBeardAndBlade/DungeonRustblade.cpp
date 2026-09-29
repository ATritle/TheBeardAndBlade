#include "DungeonActors.h"
#include "DungeonRoster.h"
#include "DungeonCombatBalance.h"
#include "RustbladeSquire.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformMisc.h"

void ADungeonEnemy::TickRustblade(float Dt,ADungeonHero* H,ADungeonGameMode* G)
{
    using namespace RustbladeSquire;
    if(SpawnTime>0) { SpawnTime=FMath::Max(0.f,SpawnTime-Dt);return; }
    if(RustHurtAge>=0) {
        RustHurtAge+=Dt;
        if(RustHurtAge>=HurtDuration)RustHurtAge=-1;
        return;
    }
    const auto P=DungeonView::Project(GetActorLocation());
    const auto Target=DungeonView::Project(H->GetActorLocation());
    const auto& Profile=DungeonRoster::Get(Species);
    if(RustAttackAge>=0) {
        const float Before=RustAttackAge;
        RustAttackAge+=Dt;
        Windup=FMath::Max(0.f,StrikeTime-RustAttackAge);
        if(!bRustStrikeFired&&CrossedStrike(Before,RustAttackAge)) {
            bRustStrikeFired=true;
            G->PlaySound(TEXT("Sword"),.42f);
            // Facing/aim are locked for the whole swing. ReceiveHit owns dodge,
            // armor, damage reduction and invulnerability, exactly as other melee.
            if(Hits(Target-P,ChargeAim))H->ReceiveHit(Profile.Damage);
        }
        if(RustAttackAge>=AttackDuration) {
            RustAttackAge=-1;Windup=0;Recovery=.32f;
        }
        return;
    }
    if(Recovery>0) { Recovery=FMath::Max(0.f,Recovery-Dt);return; }
    const auto Delta=Target-P;
    if(!Delta.IsNearlyZero())Facing=Direction(Delta);
    if(Delta.Size()<=Profile.Range) {
        RustAttackAge=0;bRustStrikeFired=false;Windup=StrikeTime;
        ChargeAim=Aim(Facing);AttackTarget=Target;
        return;
    }
    FVector2D Velocity=Delta.GetSafeNormal()*Profile.Speed*(SlowTime>0?.65f:1.f);
    for(const auto& Other:G->GetEnemies())if(IsValid(Other)&&Other!=this) {
        const auto Apart=P-DungeonView::Project(Other->GetActorLocation());
        if(Apart.SizeSquared()>1&&Apart.SizeSquared()<1600)Velocity+=Apart.GetSafeNormal()*26;
    }
    const auto Next=DungeonView::Clamp(P+Velocity*Dt);
    const auto Travel=Next-P;
    bWalking=Travel.SizeSquared()>.0001;
    if(bWalking) { Facing=Direction(Travel);WalkDistance+=Travel.Size(); }
    SetActorLocation(DungeonView::Unproject(Next));
}

void ADungeonHUD::Rustblade(ADungeonEnemy* E)
{
    using namespace RustbladeSquire;
    const auto P=DungeonView::Project(E->GetActorLocation());
    const TCHAR* State=TEXT("idle");
    static const int32 Idle[]={0,1,2,3,2,1};
    int32 Frame=Idle[int(E->MotionClock*4)%6];
    if(E->RustHurtAge>=0) {State=TEXT("hurt");Frame=HurtFrame(E->RustHurtAge);}
    else if(E->RustAttackAge>=0) {State=TEXT("attack");Frame=AttackFrame(E->RustAttackAge);}
    else if(E->bWalking) {State=TEXT("walk");Frame=WalkFrame(E->WalkDistance);}
    Shadow(P,24);
    if(E->SpawnTime>0)Ring(P,30+E->SpawnTime*20,FLinearColor(.55f,.35f,.8f,.7f),2);
    const auto Tint=E->IsHurt()?FLinearColor(1,.75f,.68f):E->PoisonTime>0?FLinearColor(.7f,1,.65f):E->SlowTime>0?FLinearColor(.65f,.85f,1):FLinearColor::White;
    // Fixed canvas/root: no runtime mirroring, bobbing or per-state fit-to-box.
    Sprite(Art(E->Facing,State,Frame),P.X-RenderSize*.5f,P.Y-RenderSize*RootY,RenderSize,RenderSize,Tint);
    DrawStatus(P-FVector2D(0,137),0,E->SlowTime,E->PoisonTime,E->BleedTime,false,E->MotionClock);
    Box(P.X-24,P.Y-130,48,4,FLinearColor(.12f,.02f,.025f));
    Box(P.X-24,P.Y-130,48*E->Health/E->MaxHealth,4,FLinearColor(.8f,.15f,.12f));
}

void ADungeonGameMode::VerifyRustblade()
{
#if !UE_BUILD_SHIPPING
    using namespace RustbladeSquire;
    int32 Failures=0,Checks=0;
    auto Check=[&](bool OK,const TCHAR* Message) { ++Checks;if(!OK){++Failures;UE_LOG(LogTemp,Error,TEXT("RUSTBLADE FAIL %s"),Message);} };
    for(int32 D=0;D<8;++D) {
        Check(Direction(Aim(D))==D,TEXT("eight independent directions"));
        Check(Hits(Aim(D)*75,Aim(D)),TEXT("forward reach"));
        Check(!Hits(-Aim(D)*45,Aim(D)),TEXT("behind attacker misses"));
        Check(!Hits(Aim(D)*105,Aim(D)),TEXT("out of reach misses"));
        const auto A=Aim(D);Check(!Hits(A*65+FVector2D(-A.Y,A.X)*60,A),TEXT("side dodge misses"));
        const TCHAR* States[]={TEXT("idle"),TEXT("walk"),TEXT("attack"),TEXT("hurt"),TEXT("death")};
        const int32 Counts[]={4,8,AttackFrames,4,8};
        for(int32 S=0;S<5;++S)for(int32 F=0;F<Counts[S];++F) {
            const auto Name=Art(D,States[S],F);
            const auto Path=FString::Printf(TEXT("/Game/Art/EnemyExpansion/RustbladeSquire/%s.%s"),*Name,*Name);
            auto* T=LoadObject<UTexture2D>(nullptr,*Path);
            Check(T!=nullptr,*Name);
            if(T)Check(T->Filter==TF_Nearest&&T->NeverStream,TEXT("sharp non-streaming texture"));
        }
    }
    for(float FPS:{30.f,60.f,120.f}) {
        float Age=0;int HitsCount=0;
        while(Age<AttackDuration){const float Previous=Age;Age+=1.f/FPS;if(CrossedStrike(Previous,Age))++HitsCount;}
        Check(HitsCount==1,TEXT("one strike at every framerate"));
    }
    Check(CrossedStrike(0,AttackDuration),TEXT("long frame cannot skip strike"));
    Check(!CrossedStrike(StrikeTime,StrikeTime+.1f),TEXT("strike not repeated"));
    Check(AttackFrame(StrikeTime+.0001f)==6,TEXT("damage aligns with selected downstroke"));
    Check(DungeonCombatBalance::FreedomDamage(25,100,false)==18.75f,TEXT("exact threshold survives"));
    Check(DungeonCombatBalance::FreedomDamage(20,100,false)==20.f,TEXT("low health execution"));
    Check(DungeonCombatBalance::FreedomDamage(100,100,true)==0.f,TEXT("boss immunity unchanged"));
    Check(FMath::IsNearlyEqual(DungeonCombatBalance::SpawnHealth(65,1,false),93.4375f),TEXT("base health scales once"));
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));
    Check(H!=nullptr,TEXT("player exists for integration checks"));
    if(H) {
        StartGame();PendingSpawns=1;
        for(auto& E:Enemies)if(IsValid(E))E->Destroy();Enemies.Empty();
        auto* E=GetWorld()->SpawnActor<ADungeonEnemy>();
        E->Species=RustbladeSquire::Species;E->SpawnTime=0;E->Health=E->MaxHealth=100;Enemies.Add(E);
        const FVector2D Origin(640,450);
        auto Reset=[&](int32 D) {
            H->Restart();H->Armor=0;H->DamageReduction=0;
            E->SetActorLocation(DungeonView::Unproject(Origin));
            H->SetActorLocation(DungeonView::Unproject(Origin+Aim(D)*75));
            E->RustAttackAge=E->RustHurtAge=-1;E->Recovery=E->Windup=0;
            E->bRustStrikeFired=false;E->Facing=D;E->SlowTime=0;E->HurtTime=0;
        };
        for(int32 D=0;D<8;++D)for(float FPS:{30.f,60.f,120.f}) {
            Reset(D);const float Initial=H->Health;
            E->TickRustblade(1.f/FPS,H,this);
            Check(E->RustAttackAge==0&&H->Health==Initial,TEXT("attack begins without contact damage"));
            float Age=0;
            while(Age<AttackDuration) {E->TickRustblade(1.f/FPS,H,this);Age+=1.f/FPS;}
            Check(FMath::IsNearlyEqual(H->Health,Initial-14),TEXT("one actual 14-damage hit in each direction/FPS"));
            Check(E->Facing==D,TEXT("facing remains locked throughout attack"));
            Reset(D);E->TickRustblade(.016f,H,this);E->TickRustblade(StrikeTime-.03f,H,this);
            H->Dodge();const float BeforeDodge=H->Health;E->TickRustblade(.05f,H,this);
            Check(H->Health==BeforeDodge&&E->bRustStrikeFired,TEXT("timed dodge consumes strike without damage"));
            Reset(D);E->TickRustblade(.016f,H,this);
            const auto A=Aim(D);
            H->SetActorLocation(DungeonView::Unproject(Origin-A*75));
            E->TickRustblade(StrikeTime+.01f,H,this);
            Check(H->Health==H->MaxHealth&&E->Facing==D,TEXT("attack cannot turn or reacquire behind"));
        }
        Reset(2);E->TickRustblade(.016f,H,this);E->TickRustblade(.3f,H,this);
        E->TakeDungeonDamage(1);const float BeforeInterrupt=H->Health;
        Check(E->RustAttackAge<0&&E->RustHurtAge==0,TEXT("hurt cancels pending strike"));
        E->TickRustblade(.3f,H,this);E->TickRustblade(.2f,H,this);
        Check(H->Health==BeforeInterrupt,TEXT("cancelled strike cannot fire late"));
        Reset(2);H->SetActorLocation(DungeonView::Unproject(Origin+FVector2D(300,0)));
        E->WalkDistance=0;E->TickRustblade(.1f,H,this);const float NormalDistance=E->WalkDistance;
        E->SetActorLocation(DungeonView::Unproject(Origin));E->WalkDistance=0;E->SlowTime=1;
        E->TickRustblade(.1f,H,this);
        Check(FMath::IsNearlyEqual(E->WalkDistance,NormalDistance*.65f,.001f),TEXT("slow reduces travel and gait together"));
        E->SetActorLocation(DungeonView::Unproject(FVector2D(1170,450)));
        H->SetActorLocation(DungeonView::Unproject(FVector2D(1400,450)));
        const float DistanceAtWall=E->WalkDistance;E->TickRustblade(.2f,H,this);
        Check(!E->bWalking&&E->WalkDistance==DistanceAtWall,TEXT("wall cannot advance walk phase"));
        Reset(2);E->RustAttackAge=.3f;const float AgeBefore=E->RustAttackAge;
        bMenu=true;E->Tick(.5f);bMenu=false;
        Check(E->RustAttackAge==AgeBefore,TEXT("pause freezes action time"));
        const int32 BloodBefore=Blood.Num();E->TakeDungeonDamage(1000);
        Check(!Enemies.Contains(E)&&E->IsActorBeingDestroyed(),TEXT("death removes combat actor immediately"));
        Check(Blood.Num()>BloodBefore&&Blood.Last().RustDirection==2,TEXT("death preserves facing for authored collapse"));
        Check(PendingSpawns==1&&!bChest,TEXT("death does not bypass progression"));
    }
    UE_LOG(LogTemp,Display,TEXT("RUSTBLADE_VERIFY checks=%d failures=%d"),Checks,Failures);
    FPlatformMisc::RequestExitWithStatus(true,Failures?1:0);
#endif
}
