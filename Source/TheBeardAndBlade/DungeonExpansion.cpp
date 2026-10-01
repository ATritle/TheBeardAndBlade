#include "DungeonExpansion.h"
#include "DungeonActors.h"
#include "DungeonRoster.h"
#include "DungeonCombatBalance.h"
#include "DungeonExpansionAnchors.h"
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
        if(ExpansionAttackAge>=Duration(Species)){
            if(DungeonExpansionV2::Is(Species)&&DungeonExpansionV2::Get(Species).Elite&&bExpansionRanged)ExpansionAdvanceTime=3.f;
            ExpansionAttackAge=-1;Windup=0;Recovery=.15f;
        }
        return;
    }
    if(Recovery>0){Recovery=FMath::Max(0.f,Recovery-Dt);return;}
    const auto Delta=Target-P;
    const bool Advancing=ExpansionAdvanceTime>0&&Delta.Size()>95.f;
    ExpansionAdvanceTime=Delta.Size()<=95.f?0.f:FMath::Max(0.f,ExpansionAdvanceTime-Dt);
    if(!Advancing&&Delta.Size()<=Profile.Range){
        if(!Delta.IsNearlyZero())Facing=RustbladeSquire::Direction(Delta);
        ExpansionAttackAge=0;bExpansionReleased=false;Windup=DungeonExpansion::Windup(Species);
        ChargeAim=RustbladeSquire::Aim(Facing);AttackTarget=Target;
        bExpansionRanged=DungeonExpansionV2::Is(Species)?
            (DungeonExpansionV2::Get(Species).ProjectileSpeed>0&&(!DungeonExpansionV2::Get(Species).Hybrid||Delta.Size()>115.f)):
            Species!=51&&(Species!=53||Delta.Size()>115.f);
        return;
    }
    auto Velocity=Delta.GetSafeNormal()*Profile.Speed*(SlowTime>0?.65f:1.f);
    for(const auto& Other:G->GetEnemies())if(IsValid(Other)&&Other!=this){
        const auto Apart=P-DungeonView::Project(Other->GetActorLocation());
        if(Apart.SizeSquared()>1&&Apart.SizeSquared()<1600)Velocity+=Apart.GetSafeNormal()*26;
    }
    const auto Next=DungeonView::Clamp(P+Velocity*Dt),Travel=Next-P;
    bWalking=Travel.SizeSquared()>.0001;
    if(bWalking){Facing=StableFacing(Facing,Travel);WalkDistance+=Travel.Size();}
    SetActorLocation(DungeonView::Unproject(Next));
}

void ADungeonGameMode::ReleaseExpansion(ADungeonEnemy* E,ADungeonHero* H)
{
    if(!IsValid(E)||E->Health<=0||E->ExpansionHurtAge>=0||!H||H->Health<=0)return;
    const auto P=DungeonView::Project(E->GetActorLocation()),Target=DungeonView::Project(H->GetActorLocation());
    const float Damage=DungeonRoster::Get(E->Species).Damage;
    if(!E->bExpansionRanged){
        PlaySound(TEXT("Sword"),.42f);
        if(DungeonExpansion::MeleeHit(E->Species,Target-P,E->ChargeAim)&&DungeonExpansion::WallHit(P,Target)>1)H->ReceiveMeleeHit(Damage,P);
        return;
    }
    FDungeonShot S;S.ExpansionSpecies=E->Species;S.SourceEnemy=E;
    S.Position=S.Origin=P;S.Target=Target;S.Life=S.FlightTime=2.6f;S.Damage=Damage;S.Radius=8;
    const bool New=DungeonExpansionV2::Is(E->Species);
    const float Speed=New?DungeonExpansionV2::Get(E->Species).ProjectileSpeed:E->Species==50?280.f:E->Species==52?230.f:270.f;
    auto Aim=(Target-P).GetSafeNormal();if(Aim.IsNearlyZero())Aim=E->ChargeAim;
    S.Velocity=Aim*Speed;S.bMotionTuned=true;
    S.bTrackingStopped=New?DungeonExpansionV2::Get(E->Species).TrackingDegrees<=0:E->Species!=52;
    if(New){
        const auto& Profile=DungeonExpansionV2::Get(E->Species);
        S.BlastRadius=Profile.AreaRadius;
        if(Profile.Arc){S.FlightTime=FMath::Clamp((Target-P).Size()/Speed,.45f,1.6f);S.Velocity=(Target-P)/S.FlightTime;S.bTrackingStopped=true;}
    }
    Shots.Add(S);PlaySound(E->Species==50?TEXT("Throw"):TEXT("Magic"),.35f);
}

void ADungeonGameMode::UpdateExpansionShot(FDungeonShot& S,float Dt,ADungeonHero* H)
{
    using namespace DungeonExpansion;
    // Sweep the visible curved trajectory in small steps, including long frames.
    if(Dt>1.f/60.f+.0001f){
        float Remaining=Dt;
        while(Remaining>0&&S.Life>0){const float Part=FMath::Min(Remaining,1.f/60.f);UpdateExpansionShot(S,Part,H);Remaining-=Part;}
        return;
    }
    const bool New=DungeonExpansionV2::Is(S.ExpansionSpecies);
    const bool Arc=New&&DungeonExpansionV2::Get(S.ExpansionSpecies).Arc;
    const float Step=FMath::Min(Dt,FMath::Max(0.f,Arc?FMath::Min(S.Life,S.FlightTime-S.Age):S.Life));
    const auto Target=DungeonView::Project(H->GetActorLocation());
    const auto ToHero=Target-S.Position;
    if((S.ExpansionSpecies==52||New)&&!S.bTrackingStopped){
        if(H->IsRolling()||S.Age>=.65f||ToHero.Size()<=100||FVector2D::DotProduct(ToHero,S.Velocity)<=0)S.bTrackingStopped=true;
        else{
            const float Heading=FMath::Atan2(S.Velocity.Y,S.Velocity.X),Desired=FMath::Atan2(ToHero.Y,ToHero.X);
            const float Limit=FMath::DegreesToRadians(New?DungeonExpansionV2::Get(S.ExpansionSpecies).TrackingDegrees:60.f)*FMath::Min(Step,.65f-S.Age);
            const float Turn=FMath::Clamp(FMath::FindDeltaAngleRadians(Heading,Desired),-Limit,Limit);
            S.Velocity=FVector2D(FMath::Cos(Heading+Turn),FMath::Sin(Heading+Turn))*S.Velocity.Size();
        }
    }
    const auto Before=S.Position,After=Before+S.Velocity*Step;
    const float Wall=WallHit(Before,After);
    const auto VisualBefore=ShotVisualPosition(S.ExpansionSpecies,Before,S.Age,S.FlightTime);
    const auto VisualAfter=ShotVisualPosition(S.ExpansionSpecies,After,S.Age+Step,S.FlightTime);
    const float Player=PlayerBodyHit(VisualBefore,VisualAfter,Target,S.Radius);
    S.Age+=Step;S.Life-=Dt;S.Position=After;
    const bool Land=Arc&&S.Age>=S.FlightTime;
    if(Wall<=1||Player<=1||Land){
        const bool HitPlayer=Player<Wall;
        S.Position=Land&&Wall>1&&Player>1?S.Target:FMath::Lerp(Before,After,FMath::Min(Wall,Player));
        const float HealthBefore=H->Health;
        const bool AreaHit=!HitPlayer&&S.BlastRadius>0&&(Target-S.Position).Size()<=S.BlastRadius&&WallHit(S.Position,Target)>1;
        if(HitPlayer||AreaHit)H->ReceiveHit(S.Damage,false,AreaHit?S.Position:Target-S.Velocity.GetSafeNormal()*60);
        // Dodge/invulnerability consumes the shot without a fake impact on the player.
        if(!HitPlayer||H->Health<HealthBefore){
            FDungeonSplash FX;FX.Position=HitPlayer?FMath::Lerp(VisualBefore,VisualAfter,Player):S.Position;FX.ExpansionSpecies=S.ExpansionSpecies;
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
    const bool New=DungeonExpansionV2::Is(E->Species);
    const TCHAR* State=TEXT("idle");static const int Idle[]={0,1,2,3,2,1};
    int Frame=New?int(E->MotionClock*8)%8:Idle[int(E->MotionClock*4)%6];
    if(E->ExpansionHurtAge>=0){State=TEXT("hurt");const int C=Count(E->Species,State);Frame=FMath::Clamp(int(E->ExpansionHurtAge/.3f*C),0,C-1);}
    else if(E->ExpansionAttackAge>=0){
        State=New&&E->bExpansionRanged&&DungeonExpansionV2::Get(E->Species).Hybrid?TEXT("ranged"):TEXT("attack");
        Frame=New?DungeonExpansionV2::AttackFrame(E->Species,E->Facing,E->bExpansionRanged,E->ExpansionAttackAge):AttackFrame(E->Species,E->ExpansionAttackAge);
    }
    else if(E->bWalking){State=TEXT("walk");const int C=Count(E->Species,State);Frame=int(E->WalkDistance/(New?110.f:78.f)*C)%C;}
    Shadow(P,24);
    if(E->SpawnTime>0)Ring(P,30+E->SpawnTime*20,FLinearColor(.55f,.35f,.8f,.7f),2);
    const auto Tint=E->IsHurt()?FLinearColor(1,.75f,.68f):E->PoisonTime>0?FLinearColor(.7f,1,.65f):E->SlowTime>0?FLinearColor(.65f,.85f,1):FLinearColor::White;
    const float DrawSize=Size(E->Species);
    const auto AnchorCorrection=New?DungeonExpansionAnchors::Get(E->Species,E->Facing,State,Frame)*(DrawSize/384.f):FVector2D::ZeroVector;
    Sprite(Art(E->Species,E->Facing,State,Frame),P.X-DrawSize*.5f+AnchorCorrection.X,P.Y-DrawSize*Root(E->Species)+AnchorCorrection.Y,DrawSize,DrawSize,Tint);
    const float BarHeight=E->Species==68||E->Species==75?190.f:New&&DungeonExpansionV2::Get(E->Species).Elite?190.f:130.f;
    DrawStatus(P-FVector2D(0,BarHeight+7),0,E->SlowTime,E->PoisonTime,E->BleedTime,false,E->MotionClock);
    Box(P.X-24,P.Y-BarHeight,48,4,FLinearColor(.12f,.02f,.025f));Box(P.X-24,P.Y-BarHeight,48*E->Health/E->MaxHealth,4,FLinearColor(.8f,.15f,.12f));
}

void ADungeonHUD::ExpansionReview(int32 S,float Age)
{
    using namespace DungeonExpansion;
    S=FMath::Clamp(S,First,Last);
    Box(0,0,1280,800,FLinearColor(.025f,.032f,.029f));
    Label(FString::Printf(TEXT("%s / eight authored views"),DungeonRoster::Get(S).Name),20,10,FLinearColor(.9f,.75f,.4f),.9f);
    const bool New=DungeonExpansionV2::Is(S),Hybrid=New&&DungeonExpansionV2::Get(S).Hybrid;
    const TCHAR* States[]={TEXT("idle"),TEXT("walk"),TEXT("attack"),TEXT("hurt"),TEXT("death"),TEXT("ranged")};
    for(int D=0;D<8;++D)for(int A=0;A<(Hybrid?6:5);++A){
        const int C=Count(S,States[A]);int Frame=0;
        if(A==0)Frame=int(Age*(New?8:4))%C;
        if(A==1)Frame=int(Age*61/78*C)%C;
        if(A==2||A==5)Frame=New?DungeonExpansionV2::AttackFrame(S,D,A==5,FMath::Fmod(Age,Duration(S)+.15f)):AttackFrame(S,FMath::Fmod(Age,Duration(S)+.15f));
        if(A==3)Frame=FMath::Clamp(int(FMath::Fmod(Age,.8f)/.3f*C),0,C-1);
        if(A==4)Frame=FMath::Clamp(int(FMath::Fmod(Age,2.5f)/.14f),0,C-1);
        const float DrawSize=New?340.f:RenderSize;
        float X=80+D*160,Y=149+A*(Hybrid?124:148);
        const auto AnchorCorrection=New?DungeonExpansionAnchors::Get(S,D,States[A],Frame)*(DrawSize/384.f):FVector2D::ZeroVector;
        Sprite(Art(S,D,States[A],Frame),X-DrawSize*.5f+AnchorCorrection.X,Y-DrawSize*Root(S)+AnchorCorrection.Y,DrawSize,DrawSize);
        Label(FString::Printf(TEXT("%s %s %d"),RustbladeSquire::Directions[D],States[A],Frame),X-60,Y+4,FLinearColor(.85f,.85f,.8f),.6f);
    }
}

void ADungeonGameMode::VerifyExpansion()
{
#if !UE_BUILD_SHIPPING
    using namespace DungeonExpansion;
    int Checks=0,Errors=0;auto Check=[&](bool OK,const TCHAR* M){++Checks;if(!OK){++Errors;UE_LOG(LogTemp,Error,TEXT("EXPANSION FAIL %s"),M);}};
    for(int S=First;S<=Last;++S){
        TArray<const TCHAR*> States={TEXT("idle"),TEXT("walk"),TEXT("attack"),TEXT("hurt"),TEXT("death")};
        if(DungeonExpansionV2::Is(S)&&DungeonExpansionV2::Get(S).Hybrid)States.Add(TEXT("ranged"));
        for(int D=0;D<8;++D)for(int A=0;A<States.Num();++A)for(int F=0;F<Count(S,States[A]);++F){
            const auto Name=Art(S,D,States[A],F);FString Asset=Name;int AtlasFrame,Rows;
            DungeonExpansionV2::Atlas(Name,Asset,AtlasFrame,Rows);
            auto* T=LoadObject<UTexture2D>(nullptr,*Path(Asset));
            Check(T!=nullptr,*Name);if(T){
                Check(T->Filter==TF_Nearest&&T->NeverStream,TEXT("sharp non-streaming"));
                if(S==68||S==75){
                    #if WITH_EDITORONLY_DATA
                    // NullRHI has no GPU resource; inspect the imported source bulk data.
                    Check(T->Source.GetSizeX()==3072&&T->Source.GetSizeY()==Rows*768,TEXT("two-times source-detail atlas imported in engine"));
                    #endif
                }
            }
        }
        for(float FPS:{30.f,60.f,120.f}){int Events=0;float Age=0;while(Age<Duration(S)){float Previous=Age;Age+=1/FPS;if(Crossed(S,Previous,Age))++Events;}Check(Events==1,TEXT("single release across frame rates"));}
        if(DungeonExpansionV2::Is(S)){
            for(const TCHAR* State:{TEXT("projectile"),TEXT("hero-impact"),TEXT("ground-impact")}){
                // Pure melee profiles use the existing shared hit effect, not projectile FX atlases.
                if(DungeonExpansionV2::Get(S).ProjectileSpeed<=0)continue;
                for(int F=0;F<DungeonExpansionV2::Count(State);++F){
                    const FString Name=FX(S,State,F);FString Asset=Name;int AF,Rows;
                    Check(DungeonExpansionV2::Atlas(Name,Asset,AF,Rows),TEXT("FX resolves atlas"));
                    auto* T=LoadObject<UTexture2D>(nullptr,*Path(Asset));Check(T!=nullptr,*Name);
                }
            }
            for(int D=0;D<8;++D)for(bool Ranged:{false,true})
                Check(DungeonExpansionV2::AttackFrame(S,D,Ranged,Windup(S)+.0001f)==DungeonExpansionV2::ReleaseFrame(S,D,Ranged),TEXT("authored release aligns damage"));
        }else Check(AttackFrame(S,Windup(S)+.0001f)==8,TEXT("release aligns pose eight"));
        Check(Crossed(S,0,Duration(S)),TEXT("long tick release"));
    }
    Check(FMath::IsNearlyEqual(CircleHit({0,0},{200,0},{100,0},20),.4f),TEXT("swept first contact"));
    Check(CircleHit({0,40},{200,40},{100,0},20)>1,TEXT("side dodge misses"));
    for(float Y:{-95.f,-65.f,-35.f,0.f})Check(PlayerBodyHit({-100,Y},{100,Y},{0,0},8)<=1,TEXT("projectile hits visible body height"));
    Check(PlayerBodyHit({-100,-140},{100,-140},{0,0},8)>1,TEXT("projectile above head misses"));
    Check(PlayerBodyHit({45,-140},{45,40},{0,0},8)>1,TEXT("projectile outside body width misses"));
    Check(Size(68)>Size(54)&&Size(58)>DungeonExpansionV2::RenderSize,TEXT("templar and elites enlarged"));
    Check(Size(68)==Size(75),TEXT("duelist matches templar draw scale"));
    Check(FMath::IsNearlyEqual(Size(68),435.f*1.3f*1.15f,.001f),TEXT("both duel test enemies fifteen percent larger"));
    Check(DungeonRoster::Get(75).Speed==123.f&&DungeonRoster::Get(68).Speed==68.f,TEXT("duelist movement 1.5x and templar movement unchanged"));
    Check(DungeonExpansionV2::Get(75).Elite&&DungeonExpansionV2::Get(75).Health==195&&DungeonExpansionV2::Get(75).Damage==27,TEXT("duelist elite balance"));
    for(int Removed:{56,64,67,69,74})Check(!DungeonExpansionV2::Enabled(Removed),TEXT("removed enemy excluded"));
    int ActiveSpecies=54;
    for(int I=0;I<20;++I){Check(DungeonExpansionV2::Enabled(ActiveSpecies),TEXT("active test cycle excludes removed enemies"));ActiveSpecies=DungeonExpansionV2::NextEnabled(ActiveSpecies,1);}
    Check(ActiveSpecies==54,TEXT("20-enemy test roster wraps"));
    Check(FMath::IsNearlyEqual(WallHit({1200,400},{1250,400}),.3f),TEXT("wall contact"));
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));Check(H!=nullptr,TEXT("hero"));
    if(H){
        StartGame();PendingSpawns=1;for(auto& E:Enemies)if(IsValid(E))E->Destroy();Enemies.Empty();
        const FVector2D P(640,450);
        for(int S=First;S<=Last;++S){
            auto* E=GetWorld()->SpawnActor<ADungeonEnemy>();E->Species=S;E->SpawnTime=0;E->Health=E->MaxHealth=100;Enemies.Add(E);
            for(int D=0;D<8;++D)for(float FPS:{30.f,60.f,120.f}){
                H->Restart();H->Armor=H->DamageReduction=0;Shots.Empty();E->ExpansionAttackAge=E->ExpansionHurtAge=-1;E->Recovery=0;E->ExpansionAdvanceTime=0;
                E->SetActorLocation(DungeonView::Unproject(P));H->SetActorLocation(DungeonView::Unproject(P+RustbladeSquire::Aim(D)*75));
                E->TickExpansion(1/FPS,H,this);Check(H->Health==H->MaxHealth,TEXT("no contact damage"));
                float Age=0;while(Age<Duration(S)){E->TickExpansion(1/FPS,H,this);Age+=1/FPS;}
                Check(E->Facing==D,TEXT("locked facing"));
                const bool Melee=DungeonExpansionV2::Is(S)?DungeonExpansionV2::Get(S).Hybrid||DungeonExpansionV2::Get(S).ProjectileSpeed<=0:S==51||S==53;
                if(Melee)Check(FMath::IsNearlyEqual(H->Health,H->MaxHealth-DungeonRoster::Get(S).Damage),TEXT("one raw melee hit"));
                else Check(Shots.Num()==1&&Shots[0].Damage==DungeonRoster::Get(S).Damage,TEXT("one unmultiplied projectile"));
            }
            H->Restart();E->ExpansionAttackAge=.1f;E->ExpansionHurtAge=-1;E->TakeDungeonDamage(1);
            Check(E->ExpansionAttackAge<0&&E->ExpansionHurtAge==0,TEXT("stagger cancels"));
            E->TakeDungeonDamage(1000);Check(!Enemies.Contains(E)&&E->IsActorBeingDestroyed(),TEXT("death clears actor"));
            Check(Blood.Last().ExpansionSpecies==S,TEXT("corpse keeps species"));
        }
        for(int S=54;S<=78;++S){
            const auto& Profile=DungeonExpansionV2::Get(S);
            if(Profile.Elite)Check(Profile.Health>=185&&Profile.Damage>=25,TEXT("elite health/damage pool"));
            if(Profile.ProjectileSpeed<=0)continue;
            auto* E=GetWorld()->SpawnActor<ADungeonEnemy>();E->Species=S;E->Health=100;E->ExpansionHurtAge=-1;
            E->SetActorLocation(DungeonView::Unproject({640,300}));E->bExpansionRanged=true;E->ChargeAim={0,1};
            H->Restart();H->Armor=H->DamageReduction=0;H->SetActorLocation(DungeonView::Unproject({640,550}));Shots.Empty();
            ReleaseExpansion(E,H);Check(Shots.Num()==1,TEXT("new ranged release once"));
            if(Shots.Num()==1){
                auto Shot=Shots[0];Check(Shot.Damage==Profile.Damage,TEXT("ranged raw damage"));
                UpdateExpansionShot(Shot,1.7f,H);
                Check(Shot.Life==0&&FMath::IsNearlyEqual(H->Health,H->MaxHealth-Profile.Damage),TEXT("new ranged direct hit once"));
                H->Restart();H->Armor=H->DamageReduction=0;H->SetActorLocation(DungeonView::Unproject({640,550}));H->Dodge();
                Shot=Shots[0];UpdateExpansionShot(Shot,1.7f,H);Check(H->Health==H->MaxHealth,TEXT("new ranged dodge avoids damage"));
                if(Profile.Arc){
                    H->Restart();H->Armor=H->DamageReduction=0;H->SetActorLocation(DungeonView::Unproject({840,550}));
                    Shot=Shots[0];UpdateExpansionShot(Shot,1.7f,H);
                    Check(Shot.Life==0&&(Shot.Position-Shot.Target).Size()<.01f&&H->Health==H->MaxHealth,TEXT("arc lands at locked target without overshoot"));
                }
            }
            if(Profile.Elite){
                H->Restart();H->SetActorLocation(DungeonView::Unproject({640,550}));
                E->SetActorLocation(DungeonView::Unproject({640,300}));
                E->SpawnTime=0;E->Recovery=0;E->ExpansionHurtAge=-1;
                E->ExpansionAttackAge=Duration(S)-.01f;E->bExpansionReleased=true;E->bExpansionRanged=true;
                E->TickExpansion(.02f,H,this);
                Check(E->ExpansionAdvanceTime==3.f,TEXT("elite ranged recovery schedules pursuit"));
                E->Recovery=0;E->TickExpansion(.5f,H,this);
                Check(E->ExpansionAttackAge<0&&DungeonView::Project(E->GetActorLocation()).Y>335.f,TEXT("elite advances inside ranged distance instead of firing again"));
                E->SetActorLocation(DungeonView::Unproject({640,460}));E->TickExpansion(.016f,H,this);
                Check(E->ExpansionAttackAge==0&&!E->bExpansionRanged&&E->ExpansionAdvanceTime==0,TEXT("elite close pursuit switches to melee"));
            }
            E->Destroy();
        }
        for(int Style:{1,10,11,12,15}){
            H->Restart();H->Armor=H->DamageReduction=0;H->SetActorLocation(DungeonView::Unproject({640,450}));Shots.Empty();
            FDungeonShot Body;Body.Style=Style;Body.Position={540,365};Body.Velocity={270,0};Body.Damage=11;
            Body.bMotionTuned=true;Body.bTrackingStopped=true;Shots.Add(Body);UpdateProjectiles(.5f);
            Check(FMath::IsNearlyEqual(H->Health,H->MaxHealth-11)&&Shots.IsEmpty(),TEXT("legacy projectile torso contact resolves one hit"));
            H->Restart();H->Armor=H->DamageReduction=0;H->SetActorLocation(DungeonView::Unproject({640,450}));H->Dodge();Shots.Add(Body);UpdateProjectiles(.5f);
            Check(H->Health==H->MaxHealth&&Shots.IsEmpty(),TEXT("legacy torso hit still respects dodge"));
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

void ADungeonGameMode::StartEliteEncounter(int32 Species)
{
#if !UE_BUILD_SHIPPING
    if(!DungeonExpansionV2::Enabled(Species))return;
    if(FParse::Param(FCommandLine::Get(),TEXT("DuelTest"))&&Species!=68&&Species!=75)return;
    StartGame();
    EliteReviewSpecies=Species;
    PendingSpawns=0;CancelBossIntro();DialogueLines.Empty();BossGrace=0;
    bAtlasActive=false;Room=1;Wave=2;bChest=false;bLootClaimed=false;
    for(auto& Enemy:Enemies)if(IsValid(Enemy))Enemy->Destroy();
    Enemies.Empty();Shots.Empty();Splashes.Empty();Blood.Empty();
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));
    if(H){H->Restart();H->SetActorLocation(DungeonView::Unproject({640,610}));}
    auto* E=GetWorld()->SpawnActor<ADungeonEnemy>();
    E->Species=Species;E->bBoss=false;E->SpawnTime=.6f;E->Facing=4;
    E->Health=E->MaxHealth=DungeonExpansionV2::Get(Species).Health;
    E->SetActorLocation(DungeonView::Unproject({640,350}));Enemies.Add(E);
    UE_LOG(LogTemp,Display,TEXT("ELITE_TEST %s HP=%.1f damage=%.1f"),DungeonRoster::Get(Species).Name,E->MaxHealth,DungeonRoster::Get(Species).Damage);
#endif
}
