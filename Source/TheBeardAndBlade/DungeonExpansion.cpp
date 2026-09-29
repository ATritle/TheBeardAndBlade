#include "DungeonExpansion.h"
#include "DungeonActors.h"
#include "DungeonRoster.h"
#include "DungeonCombatBalance.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "HAL/PlatformMisc.h"

void ADungeonEnemy::TickExpansion(float Dt,ADungeonHero* H,ADungeonGameMode* G)
{
    using namespace DungeonExpansion;
    if(SpawnTime>0){SpawnTime=FMath::Max(0.f,SpawnTime-Dt);return;}
    if(ExpansionHurtAge>=0){ExpansionHurtAge+=Dt;if(ExpansionHurtAge>=.3f)ExpansionHurtAge=-1;return;}
    const auto P=DungeonView::Project(GetActorLocation()),Target=DungeonView::Project(H->GetActorLocation());
    const auto& Profile=DungeonRoster::Get(Species);
    if(ExpansionAttackAge>=0){
        const float Before=ExpansionAttackAge;ExpansionAttackAge+=Dt;
        Windup=FMath::Max(0.f,DungeonExpansion::Windup(Species)-ExpansionAttackAge);
        if(!bExpansionReleased&&Crossed(Species,Before,ExpansionAttackAge)){
            bExpansionReleased=true;G->ReleaseExpansion(this,H);
        }
        if(ExpansionAttackAge>=Duration(Species)){ExpansionAttackAge=-1;Windup=0;Recovery=.15f;}
        return;
    }
    if(Recovery>0){Recovery=FMath::Max(0.f,Recovery-Dt);return;}
    const auto Delta=Target-P;
    if(!Delta.IsNearlyZero())Facing=RustbladeSquire::Direction(Delta);
    if(Delta.Size()<=Profile.Range){
        ExpansionAttackAge=0;bExpansionReleased=false;Windup=DungeonExpansion::Windup(Species);
        ChargeAim=RustbladeSquire::Aim(Facing);AttackTarget=Target;
        bExpansionRanged=Species!=51&&(Species!=53||Delta.Size()>115.f);
        return;
    }
    auto Velocity=Delta.GetSafeNormal()*Profile.Speed*(SlowTime>0?.65f:1.f);
    for(const auto& Other:G->GetEnemies())if(IsValid(Other)&&Other!=this){
        const auto Apart=P-DungeonView::Project(Other->GetActorLocation());
        if(Apart.SizeSquared()>1&&Apart.SizeSquared()<1600)Velocity+=Apart.GetSafeNormal()*26;
    }
    const auto Next=DungeonView::Clamp(P+Velocity*Dt),Travel=Next-P;
    bWalking=Travel.SizeSquared()>.0001;
    if(bWalking){Facing=RustbladeSquire::Direction(Travel);WalkDistance+=Travel.Size();}
    SetActorLocation(DungeonView::Unproject(Next));
}

void ADungeonGameMode::ReleaseExpansion(ADungeonEnemy* E,ADungeonHero* H)
{
    if(!IsValid(E)||E->Health<=0||E->ExpansionHurtAge>=0||!H||H->Health<=0)return;
    const auto P=DungeonView::Project(E->GetActorLocation()),Target=DungeonView::Project(H->GetActorLocation());
    const float Damage=DungeonRoster::Get(E->Species).Damage;
    if(!E->bExpansionRanged){
        PlaySound(TEXT("Sword"),.42f);
        if(DungeonExpansion::MeleeHit(E->Species,Target-P,E->ChargeAim)&&DungeonExpansion::WallHit(P,Target)>1)H->ReceiveHit(Damage);
        return;
    }
    FDungeonShot S;S.ExpansionSpecies=E->Species;S.SourceEnemy=E;
    S.Position=S.Origin=P;S.Target=Target;S.Life=S.FlightTime=2.6f;S.Damage=Damage;S.Radius=8;
    const float Speed=E->Species==50?280.f:E->Species==52?230.f:270.f;
    auto Aim=(Target-P).GetSafeNormal();if(Aim.IsNearlyZero())Aim=E->ChargeAim;
    S.Velocity=Aim*Speed;S.bMotionTuned=true;S.bTrackingStopped=E->Species!=52;
    Shots.Add(S);PlaySound(E->Species==50?TEXT("Throw"):TEXT("Magic"),.35f);
}

void ADungeonGameMode::UpdateExpansionShot(FDungeonShot& S,float Dt,ADungeonHero* H)
{
    using namespace DungeonExpansion;
    const float Step=FMath::Min(Dt,FMath::Max(0.f,S.Life));
    const auto Target=DungeonView::Project(H->GetActorLocation());
    const auto ToHero=Target-S.Position;
    if(S.ExpansionSpecies==52&&!S.bTrackingStopped){
        if(H->IsRolling()||S.Age>=.65f||ToHero.Size()<=100||FVector2D::DotProduct(ToHero,S.Velocity)<=0)S.bTrackingStopped=true;
        else{
            const float Heading=FMath::Atan2(S.Velocity.Y,S.Velocity.X),Desired=FMath::Atan2(ToHero.Y,ToHero.X);
            const float Limit=PI/3.f*FMath::Min(Step,.65f-S.Age);
            const float Turn=FMath::Clamp(FMath::FindDeltaAngleRadians(Heading,Desired),-Limit,Limit);
            S.Velocity=FVector2D(FMath::Cos(Heading+Turn),FMath::Sin(Heading+Turn))*230.f;
        }
    }
    const auto Before=S.Position,After=Before+S.Velocity*Step;
    const float Wall=WallHit(Before,After);
    const float Player=CircleHit(Before,After,Target,S.Radius+17);
    S.Age+=Step;S.Life-=Dt;S.Position=After;
    if(Wall<=1||Player<=1){
        const bool HitPlayer=Player<Wall;
        S.Position=FMath::Lerp(Before,After,FMath::Min(Wall,Player));
        const float HealthBefore=H->Health;
        if(HitPlayer)H->ReceiveHit(S.Damage);
        // Dodge/invulnerability consumes the shot without a fake impact on the player.
        if(!HitPlayer||H->Health<HealthBefore){
            FDungeonSplash FX;FX.Position=S.Position;FX.ExpansionSpecies=S.ExpansionSpecies;
            FX.bExpansionPlayerHit=HitPlayer;FX.Life=.84f;FX.Radius=45;Splashes.Add(FX);
            PlaySound(S.ExpansionSpecies==50?TEXT("TeaSplash"):TEXT("Magic"),.35f);
        }
        S.Life=0;
    }
    // Expiry is harmless: no ResolveProjectile/splash damage on timeout.
}

void ADungeonHUD::ExpansionEnemy(ADungeonEnemy* E)
{
    using namespace DungeonExpansion;
    const auto P=DungeonView::Project(E->GetActorLocation());
    const TCHAR* State=TEXT("idle");static const int Idle[]={0,1,2,3,2,1};int Frame=Idle[int(E->MotionClock*4)%6];
    if(E->ExpansionHurtAge>=0){State=TEXT("hurt");Frame=FMath::Clamp(int(E->ExpansionHurtAge/.3f*4),0,3);}
    else if(E->ExpansionAttackAge>=0){State=TEXT("attack");Frame=AttackFrame(E->Species,E->ExpansionAttackAge);}
    else if(E->bWalking){State=TEXT("walk");Frame=int(E->WalkDistance/78.f*8)%8;}
    Shadow(P,24);
    if(E->SpawnTime>0)Ring(P,30+E->SpawnTime*20,FLinearColor(.55f,.35f,.8f,.7f),2);
    const auto Tint=E->IsHurt()?FLinearColor(1,.75f,.68f):E->PoisonTime>0?FLinearColor(.7f,1,.65f):E->SlowTime>0?FLinearColor(.65f,.85f,1):FLinearColor::White;
    Sprite(Art(E->Species,E->Facing,State,Frame),P.X-RenderSize*.5f,P.Y-RenderSize*RootY,RenderSize,RenderSize,Tint);
    DrawStatus(P-FVector2D(0,137),0,E->SlowTime,E->PoisonTime,E->BleedTime,false,E->MotionClock);
    Box(P.X-24,P.Y-130,48,4,FLinearColor(.12f,.02f,.025f));Box(P.X-24,P.Y-130,48*E->Health/E->MaxHealth,4,FLinearColor(.8f,.15f,.12f));
}

void ADungeonHUD::ExpansionReview(int32 S,float Age)
{
    using namespace DungeonExpansion;
    S=FMath::Clamp(S,First,Last);
    Box(0,0,1280,800,FLinearColor(.025f,.032f,.029f));
    Label(FString::Printf(TEXT("%s / eight authored views"),DungeonRoster::Get(S).Name),20,10,FLinearColor(.9f,.75f,.4f),.9f);
    const TCHAR* States[]={TEXT("idle"),TEXT("walk"),TEXT("attack"),TEXT("hurt"),TEXT("death")};
    const int Frames[]={int(Age*4)%4,int(Age*61/78*8)%8,AttackFrame(S,FMath::Fmod(Age,Duration(S)+.15f)),FMath::Clamp(int(FMath::Fmod(Age,.8f)/.3f*4),0,3),FMath::Clamp(int(FMath::Fmod(Age,2.5f)/.14f),0,7)};
    for(int D=0;D<8;++D)for(int A=0;A<5;++A){
        float X=80+D*160,Y=181+A*148;
        Sprite(Art(S,D,States[A],Frames[A]),X-RenderSize*.5f,Y-RenderSize*RootY,RenderSize,RenderSize);
        Label(FString::Printf(TEXT("%s %s %d"),RustbladeSquire::Directions[D],States[A],Frames[A]),X-60,Y+4,FLinearColor(.85f,.85f,.8f),.6f);
    }
}

void ADungeonGameMode::VerifyExpansion()
{
#if !UE_BUILD_SHIPPING
    using namespace DungeonExpansion;
    int Checks=0,Errors=0;auto Check=[&](bool OK,const TCHAR* M){++Checks;if(!OK){++Errors;UE_LOG(LogTemp,Error,TEXT("EXPANSION FAIL %s"),M);}};
    for(int S=First;S<=Last;++S){
        const TCHAR* States[]={TEXT("idle"),TEXT("walk"),TEXT("attack"),TEXT("hurt"),TEXT("death")};
        const int Counts[]={4,8,16,4,8};
        for(int D=0;D<8;++D)for(int A=0;A<5;++A)for(int F=0;F<Counts[A];++F){
            const auto Name=Art(S,D,States[A],F);auto* T=LoadObject<UTexture2D>(nullptr,*Path(Name));
            Check(T!=nullptr,*Name);if(T)Check(T->Filter==TF_Nearest&&T->NeverStream,TEXT("sharp non-streaming"));
        }
        for(float FPS:{30.f,60.f,120.f}){int Events=0;float Age=0;while(Age<Duration(S)){float Previous=Age;Age+=1/FPS;if(Crossed(S,Previous,Age))++Events;}Check(Events==1,TEXT("single release across frame rates"));}
        Check(AttackFrame(S,Windup(S)+.0001f)==8,TEXT("release aligns pose eight"));
        Check(Crossed(S,0,Duration(S)),TEXT("long tick release"));
    }
    Check(FMath::IsNearlyEqual(CircleHit({0,0},{200,0},{100,0},20),.4f),TEXT("swept first contact"));
    Check(CircleHit({0,40},{200,40},{100,0},20)>1,TEXT("side dodge misses"));
    Check(FMath::IsNearlyEqual(WallHit({1200,400},{1250,400}),.3f),TEXT("wall contact"));
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));Check(H!=nullptr,TEXT("hero"));
    if(H){
        StartGame();PendingSpawns=1;for(auto& E:Enemies)if(IsValid(E))E->Destroy();Enemies.Empty();
        const FVector2D P(640,450);
        for(int S=First;S<=Last;++S){
            auto* E=GetWorld()->SpawnActor<ADungeonEnemy>();E->Species=S;E->SpawnTime=0;E->Health=E->MaxHealth=100;Enemies.Add(E);
            for(int D=0;D<8;++D)for(float FPS:{30.f,60.f,120.f}){
                H->Restart();H->Armor=H->DamageReduction=0;Shots.Empty();E->ExpansionAttackAge=E->ExpansionHurtAge=-1;E->Recovery=0;
                E->SetActorLocation(DungeonView::Unproject(P));H->SetActorLocation(DungeonView::Unproject(P+RustbladeSquire::Aim(D)*75));
                E->TickExpansion(1/FPS,H,this);Check(H->Health==H->MaxHealth,TEXT("no contact damage"));
                float Age=0;while(Age<Duration(S)){E->TickExpansion(1/FPS,H,this);Age+=1/FPS;}
                Check(E->Facing==D,TEXT("locked facing"));
                if(S==51||S==53)Check(FMath::IsNearlyEqual(H->Health,H->MaxHealth-DungeonRoster::Get(S).Damage),TEXT("one raw melee hit"));
                else Check(Shots.Num()==1&&Shots[0].Damage==DungeonRoster::Get(S).Damage,TEXT("one unmultiplied projectile"));
            }
            H->Restart();E->ExpansionAttackAge=.1f;E->ExpansionHurtAge=-1;E->TakeDungeonDamage(1);
            Check(E->ExpansionAttackAge<0&&E->ExpansionHurtAge==0,TEXT("stagger cancels"));
            E->TakeDungeonDamage(1000);Check(!Enemies.Contains(E)&&E->IsActorBeingDestroyed(),TEXT("death clears actor"));
            Check(Blood.Last().ExpansionSpecies==S,TEXT("corpse keeps species"));
        }
        for(int S:{50,52,53}){
            H->Restart();H->Armor=H->DamageReduction=0;H->SetActorLocation(DungeonView::Unproject({640,450}));
            FDungeonShot Shot;Shot.ExpansionSpecies=S;Shot.Position={540,450};Shot.Velocity={270,0};Shot.Damage=11;Shot.bTrackingStopped=true;
            UpdateExpansionShot(Shot,.8f,H);Check(Shot.Life==0&&H->Health==H->MaxHealth-11,TEXT("swept projectile damages once"));
            H->Restart();H->SetActorLocation(DungeonView::Unproject({640,450}));H->Dodge();Shot.Position={540,450};Shot.Life=2.6f;UpdateExpansionShot(Shot,.8f,H);
            Check(Shot.Life==0&&H->Health==H->MaxHealth,TEXT("dodge consumes projectile harmlessly"));
            H->Restart();Shot.Position={200,250};Shot.Velocity={270,0};Shot.Life=.01f;UpdateExpansionShot(Shot,.5f,H);
            Check(H->Health==H->MaxHealth,TEXT("expiry harmless"));
        }
        H->Restart();H->SetActorLocation(DungeonView::Unproject({900,450}));H->Dodge();
        FDungeonShot Wax;Wax.ExpansionSpecies=52;Wax.Position={640,450};Wax.Velocity={230,0};UpdateExpansionShot(Wax,.01f,H);
        Check(Wax.bTrackingStopped,TEXT("dodge permanently breaks tracking"));
        H->Restart();H->SetActorLocation(DungeonView::Unproject({640,250}));const auto Velocity=Wax.Velocity;UpdateExpansionShot(Wax,.1f,H);Check(Wax.Velocity==Velocity,TEXT("no reacquisition"));
    }
    Check(DungeonCombatBalance::FreedomDamage(25,100,false)==18.75f,TEXT("FREEDOM exact threshold"));
    Check(DungeonCombatBalance::FreedomDamage(20,100,false)==20,TEXT("FREEDOM executes"));
    Check(DungeonCombatBalance::FreedomDamage(100,100,true)==0,TEXT("boss immunity"));
    UE_LOG(LogTemp,Display,TEXT("EXPANSION_VERIFY checks=%d failures=%d"),Checks,Errors);FPlatformMisc::RequestExitWithStatus(true,Errors?1:0);
#endif
}
