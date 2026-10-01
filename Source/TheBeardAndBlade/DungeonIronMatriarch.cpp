#include "DungeonActors.h"
#include "DungeonRoster.h"
#include "DungeonCombatBalance.h"
#include "IronMatriarchSockets.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "Sound/SoundBase.h"

void ADungeonEnemy::BeginIronAttack(int Attack,ADungeonHero* H,ADungeonGameMode* G)
{
    using namespace IronMatriarch;
    Iron.Attack=FMath::Clamp(Attack,0,2);Iron.Age=0;Iron.Released=false;Iron.BreathTicks=0;Iron.FlameExposure=0;Iron.Meteors.Empty();
    Iron.From=DungeonView::Project(GetActorLocation());
    const auto Player=DungeonView::Project(H->GetActorLocation());
    Iron.Aim=(Player-Iron.From).GetSafeNormal();if(Iron.Aim.IsNearlyZero())Iron.Aim={0,1};
    Iron.View=View(Player-Iron.From);
    // Land short of the player with the heads on-screen and room to retaliate.
    Iron.Target=FloorTarget(Player-FVector2D(0,60));
    if((Iron.Target-Iron.From).Size()>270)Iron.Target=Iron.From+(Iron.Target-Iron.From).GetSafeNormal()*270;
    Iron.Target=FloorTarget(Iron.Target);
    Iron.Target.X=FMath::Clamp(Iron.Target.X,285.,995.);Iron.Target.Y=FMath::Clamp(Iron.Target.Y,410.,610.);
    if(Iron.Attack==2){
        // Fixed, spaced targets: one snapshot of the player, four arena quadrants.
        const FVector2D Sites[]={{300,340},{940,350},{360,605},{900,600}};
        Iron.Meteors.Add({FloorTarget(Player),MeteorRelease+MeteorWarning+MeteorFall,false});
        for(int I=0;I<4;++I){auto P=Sites[(I+Iron.Serial)%4];if(FVector2D::Distance(P,Iron.Meteors[0].Target)<145)P=FloorTarget(P+FVector2D(I%2?150:-150,0));Iron.Meteors.Add({P,MeteorRelease+MeteorWarning+MeteorFall+(I+1)*MeteorCadence,false});}
    }
    G->PlaySound(Iron.Attack==0?TEXT("IronSlamVoice"):Iron.Attack==1?TEXT("IronBreathVoice"):TEXT("IronMeteorVoice"),.65f);
}

void ADungeonEnemy::TickIronMatriarch(float Dt,ADungeonHero* H,ADungeonGameMode* G)
{
    using namespace IronMatriarch;
    if(Iron.DeathAge>=0){Iron.DeathAge+=Dt;if(Iron.DeathAge>=1.8f)G->EnemyDefeated(this);return;}
    if(Iron.Attack<0){
        Iron.View=View(DungeonView::Project(H->GetActorLocation())-DungeonView::Project(GetActorLocation()));
        Iron.Cooldown-=Dt;if(Iron.Cooldown<=0){const int Next=Iron.Serial++%3;BeginIronAttack(Next,H,G);}return;
    }
    const float Before=Iron.Age;Iron.Age+=FMath::Max(0.f,Dt);
    const auto Player=DungeonView::Project(H->GetActorLocation());
    const auto P=DungeonView::Project(GetActorLocation());
    if(Iron.Attack==0){
        const float Travel=FMath::Clamp((Iron.Age-.65f)/(SlamImpact-.65f),0.f,1.f);
        SetActorLocation(DungeonView::Unproject(FMath::Lerp(Iron.From,Iron.Target,Travel)));
        if(!Iron.Released&&Crossed(Before,Iron.Age,SlamImpact)){
            Iron.Released=true;++Iron.Landings;G->PlaySound(TEXT("Explosion"),.65f);
            if(GroundHit(Player-Iron.Target,SlamRadius))H->ReceiveHit(SlamDamage,false,Iron.Target,true);
        }
    }else if(Iron.Attack==1){
        const bool InFlame=BreathHits(Player-P,Iron.Aim)&&!H->IsBulletImmune();
        if(!InFlame)Iron.FlameExposure=0;
        // Shared cadence; leaving the cone or dodging resets exposure.
        while(Iron.BreathTicks<5&&Iron.Age>=BreathStart+Iron.BreathTicks*BreathInterval){
            ++Iron.BreathTicks;
            if(InFlame){H->ReceiveHit(BreathDamage*FlameMultiplier(Iron.FlameExposure),true,P,true);Iron.FlameExposure+=BreathInterval;}
        }
    }else{
        if(!Iron.Released&&Crossed(Before,Iron.Age,MeteorRelease)){Iron.Released=true;G->PlaySound(TEXT("Magic"),.5f);}
        for(auto& M:Iron.Meteors)if(!M.Hit&&Crossed(Before,Iron.Age,M.ImpactAt)){
            M.Hit=true;++Iron.MeteorHits;G->PlaySound(TEXT("Explosion"),.5f);
            const float Damage=MeteorSplash(Player-M.Target);if(Damage>0)H->ReceiveHit(Damage,true,M.Target,true);
        }
    }
    const float End=Iron.Attack==0?SlamEnd:Iron.Attack==1?BreathFinish:MeteorEnd;
    if(Iron.Age>=End){Iron.Cancel();Iron.Cooldown=Health<MaxHealth*.4f?1.1f:1.7f;}
}

void ADungeonHUD::IronSprite(const FString& Name,int Frame,int Frames,FVector2D Origin,FVector2D Size,FLinearColor Tint,float Angle,FVector2D Pivot)
{
    if(auto* T=Texture(Name)){
        Frame=FMath::Clamp(Frame,0,Frames-1);const float Rows=(Frames+3)/4;
        DrawTexture(T,Offset.X+Origin.X*Scale,Offset.Y+Origin.Y*Scale,Size.X*Scale,Size.Y*Scale,
            (Frame%4)*.25f,(Frame/4)/Rows,.25f,1.f/Rows,Tint,BLEND_Translucent,1,false,Angle,Pivot);
    }
}
void ADungeonHUD::DrawIronMatriarch(ADungeonEnemy* E)
{
    using namespace IronMatriarch;
    const auto& S=E->Iron;const float T=S.Age,Size=DrawSize;
    const auto P=DungeonView::Project(E->GetActorLocation());
    auto Effect=[&](const TCHAR* N,int Frame,FVector2D Ground,float W,float H,float Alpha=1.f){IronSprite(FString(TEXT("Iron_"))+N,Frame,8,Ground-FVector2D(W*.5f,H*440/512),{W,H},FLinearColor(1,1,1,Alpha));};
    if(S.Attack==0&&T<SlamImpact){Effect(TEXT("meteor_warning"),int(T*8)%8,S.Target,SlamRadius*2.5f,100);Ring(S.Target,SlamRadius,FLinearColor(1,.38f,.1f,.8f),3);}
    if(S.Attack==2)for(const auto& M:S.Meteors){
        const float Start=M.ImpactAt-MeteorWarning-MeteorFall;
        if(T>=Start&&T<M.ImpactAt){Effect(TEXT("meteor_warning"),int((T-Start)*7)%8,M.Target,MeteorRadius*2.5f,MeteorRadius*1.3f);Ring(M.Target,MeteorRadius,FLinearColor(1,.35f,.1f,.9f),2);}
    }
    float Lift=0;
    if(S.Attack==0&&T>.65f&&T<SlamImpact)Lift=FMath::Sin((T-.65f)/(SlamImpact-.65f)*PI)*FlightHeight;
    Shadow(P,65-Lift*.2f);
    const FString State=S.Attack<0?TEXT("idle"):S.Attack==0?TEXT("flying_slam"):S.Attack==1?TEXT("flamethrower"):TEXT("meteor_summon");
    const int Count=S.Attack<0?8:S.Attack==0?16:12;
    const int Pose=S.Attack<0?int(E->MotionClock*7)%8:Frame(S.Attack,T);
    const float Death=S.DeathAge<0?0:FMath::Clamp(S.DeathAge/1.8f,0.f,1.f);
    const float Height=Size*(1-Death*.6f);
    const auto Origin=P-FVector2D(Size*.5f,Height*440/512+Lift);
    const auto Tint=E->HurtTime>0?FLinearColor(1,.65f,.5f,1-Death):FLinearColor(1,1,1,1-Death);
    IronSprite(FString::Printf(TEXT("Iron_%s_%s"),*State,ViewName(S.View)),Pose,Count,Origin,{Size,Height},Tint);
    if(S.DeathAge>=0){Effect(TEXT("meteor_impact"),FMath::Min(7,int(S.DeathAge*5)),P,240,220,1-Death);return;}
    if(S.Attack==0&&T>=SlamImpact)Effect(TEXT("slam_shockwave"),FMath::Min(7,int((T-SlamImpact)*9)),P,SlamRadius*3,155,1-FMath::Clamp((T-SlamImpact)/1.1f,0.f,1.f));
    if(S.Attack==1&&T>=BreathStart-.15f&&T<BreathEnd+.2f){
        const TCHAR* Colors[]={TEXT("white"),TEXT("blue"),TEXT("red"),TEXT("green"),TEXT("violet")};
        // Preserve viewer-left white through viewer-right violet; do not cross beams.
        const auto Side=FVector2D(S.Aim.Y,-S.Aim.X);
        for(int I=0;I<5;++I){
            const auto Mouth=Origin+Mouths[S.View][Pose][I]*(Size/512.f);
            const auto End=FloorTarget(P+S.Aim*BreathRange+Side*((I-2)*30))-FVector2D(0,30);
            const auto Delta=End-Mouth;const float Angle=FMath::RadiansToDegrees(FMath::Atan2(Delta.Y,Delta.X));
            const float W=Delta.Size()*512/470.f,H=120;
            const int F=T<BreathStart?1:T>=BreathEnd?6+FMath::Min(1,int((T-BreathEnd)*10)):2+int((T-BreathStart)*10)%4;
            const float Alpha=T<BreathStart?.5f:T>=BreathEnd?1-(T-BreathEnd)/.2f:.85f;
            IronSprite(FString(TEXT("Iron_flame_"))+Colors[I],F,8,Mouth-FVector2D(W*16/512,H*.5f),{W,H},FLinearColor(1,1,1,Alpha),Angle,{16.f/512,.5f});
        }
    }
    if(S.Attack==2)for(const auto& M:S.Meteors){
        const float Fall=M.ImpactAt-MeteorFall;
        if(T>=Fall&&T<M.ImpactAt)Effect(TEXT("meteor_fall"),FMath::Clamp(int((T-Fall)/MeteorFall*8),0,7),M.Target-FVector2D(0,(M.ImpactAt-T)/MeteorFall*280),90,150);
        if(M.Hit&&T-M.ImpactAt<.85f)Effect(TEXT("meteor_impact"),FMath::Min(7,int((T-M.ImpactAt)*10)),M.Target,MeteorRadius*2.5f,190,1-(T-M.ImpactAt)/.85f);
    }
    DrawStatus(P-FVector2D(0,Size*.73f+20),0,E->SlowTime,E->PoisonTime,E->BleedTime,E->FreedomImmuneTime>0,E->MotionClock);
}

void ADungeonGameMode::ReviewIronMatriarch(float Dt)
{
#if !UE_BUILD_SHIPPING
    const bool Auto=FParse::Param(FCommandLine::Get(),TEXT("IronReview"));
    const bool Floor=FParse::Param(FCommandLine::Get(),TEXT("IronFloorTest"));
    if(!Auto&&!Floor&&!FParse::Param(FCommandLine::Get(),TEXT("IronTest")))return;
    static bool Started=false;static float Time=0;static int Captured=-1;
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));if(!H)return;
    auto* PC=UGameplayStatics::GetPlayerController(this,0);
    if(!Started||(PC&&PC->WasInputKeyJustPressed(EKeys::R))){
        Started=true;Time=0;StartPlaytestRoom(Floor?29:32);
        if(Floor){InitializeAtlasFloor(42,7);return;}
        SpawnOneEnemy();PendingSpawns=0;FreedomKills=15;
    }
    if(!Auto)return;
    Time+=Dt;
    if(Time>7.5f&&IsBossDialogueActive()){FinishBossIntro();DialogueWait=0;AdvanceBossDialogue(true);BossGrace=0;}
    if(Time>8){
        H->Health=H->MaxHealth;
        const FVector2D Sites[]={{510,610},{840,610},{640,590}};
        H->SetActorLocation(DungeonView::Unproject(FParse::Param(FCommandLine::Get(),TEXT("IronFrontReview"))?FVector2D(640,590):Sites[int((Time-8)/8)%3]));
    }
    const int Frame=int(Time*10);
    FString Capture=TEXT("Saved/IronReview");FParse::Value(FCommandLine::Get(),TEXT("IronCapture="),Capture);
    if(Frame!=Captured&&Time>.3f){Captured=Frame;FScreenshotRequest::RequestScreenshot(FPaths::ProjectDir()/Capture/FString::Printf(TEXT("frame-%05d.png"),Frame),true,false);}
    if(Time>35)FPlatformMisc::RequestExit(false);
#endif
}

void ADungeonGameMode::VerifyIronMatriarch()
{
#if !UE_BUILD_SHIPPING
    using namespace IronMatriarch;
    int Checks=0,Errors=0;auto Check=[&](bool OK,const TCHAR* Why){++Checks;if(!OK){++Errors;UE_LOG(LogTemp,Error,TEXT("IRON FAIL %s"),Why);}};
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));if(!H){FPlatformMisc::RequestExitWithStatus(false,1);return;}
    StartPlaytestRoom(32);SpawnOneEnemy();PendingSpawns=0;
    Check(AtlasChapter==7&&GetBiome()==7&&GetBossSpecies()==Species,TEXT("eighth floor and final boss"));
    Check(IsBossIntroActive()&&IntroAudio,TEXT("intro and delayed timeline cue start together"));
    auto* Cue=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/IronIntro.IronIntro"));
    Check(Cue&&FMath::IsNearlyEqual(Cue->GetDuration(),6.f,.05f),TEXT("six second supplied intro timeline"));
    Tick(2.1f);Check(IsBossIntroActive(),TEXT("name entrance during intro, no combat"));
    FinishBossIntro();DialogueWait=0;AdvanceBossDialogue(true);BossGrace=0;
    Check(Enemies.Num()==1,TEXT("one boss"));if(Enemies.IsEmpty()){FPlatformMisc::RequestExitWithStatus(false,1);return;}
    auto* E=Enemies[0].Get();
    Check(E->bBoss&&E->Species==Species&&E->MaxHealth>3000,TEXT("scaled boss health"));
    H->Armor=H->DamageReduction=0;H->MaxHealth=H->Health=10000;
    for(float FPS:{15.f,30.f,60.f,120.f})for(int Attack=0;Attack<3;++Attack){
        H->Restart();H->Armor=H->DamageReduction=0;H->MaxHealth=H->Health=10000;
        E->Iron=FState();E->SetActorLocation(DungeonView::Unproject({640,410}));H->SetActorLocation(DungeonView::Unproject({640,575}));
        E->BeginIronAttack(Attack,H,this);
        Check(E->Iron.View==0,TEXT("front view selection"));
        for(auto& M:E->Iron.Meteors)Check(FloorTarget(M.Target).Equals(M.Target),TEXT("meteor stays on valid floor"));
        const float End=Attack==0?SlamEnd:Attack==1?BreathFinish:MeteorEnd;
        for(float T=0;T<End+.02f;T+=1/FPS)E->TickIronMatriarch(1/FPS,H,this);
        Check(E->Iron.Attack==-1,TEXT("attack completes at each frame rate"));
        Check(Attack!=0||E->Iron.Landings==1,TEXT("one landing event"));
        Check(Attack!=1||E->Iron.BreathTicks==5,TEXT("five shared flame ticks, not per-head or frame"));
        Check(Attack!=2||E->Iron.MeteorHits==5,TEXT("each meteor impacts once"));
        if(Attack==1)Check(FMath::IsNearlyEqual(H->Health,10000-BreathDamage*10*IncomingDamageScale(true),.01f),TEXT("overlapping heads never stack damage"));
    }
    Check(View({-200,100})==1&&View({200,100})==2,TEXT("independent three-quarter views, no mirroring"));
    Check(Frame(0,SlamImpact)==12&&Frame(2,MeteorRelease)==7,TEXT("damage events match authored impact/release poses"));
    Check(!BreathHits({0,-100},{0,1})&&!BreathHits({400,0},{1,0}),TEXT("breath limited to frontal close range"));
    Check(GroundHit({50,0},58)&&!GroundHit({0,40},58),TEXT("impact hit region matches warning ellipse"));
    Check(MeteorSplash({0,0})==55&&MeteorSplash({35,0})==55,TEXT("meteor core does 55 raw damage"));
    Check(MeteorSplash({80,0})>0&&MeteorSplash({80,0})<55&&MeteorSplash({111,0})==0,TEXT("meteor splash falls off and stops at warning edge"));
    Check(FlameMultiplier(0)==1&&FlameMultiplier(1.6f)==3,TEXT("continuous flame ramps from one to three times"));
    H->Restart();H->Armor=H->DamageReduction=0;H->MaxHealth=H->Health=10000;
    E->SetActorLocation(DungeonView::Unproject({640,410}));H->SetActorLocation(DungeonView::Unproject({640,575}));
    E->BeginIronAttack(1,H,this);E->TickIronMatriarch(1.61f,H,this);
    Check(FMath::IsNearlyEqual(10000-H->Health,30.f*IncomingDamageScale(true),.01f),TEXT("first two flame ticks scale from 12 then 18"));
    H->SetActorLocation(DungeonView::Unproject({1000,575}));E->TickIronMatriarch(.1f,H,this);
    Check(E->Iron.FlameExposure==0,TEXT("leaving flame immediately resets exposure"));
    H->SetActorLocation(DungeonView::Unproject({640,575}));const float ReentryHP=H->Health;E->TickIronMatriarch(.3f,H,this);
    Check(FMath::IsNearlyEqual(ReentryHP-H->Health,12.f*IncomingDamageScale(true),.01f),TEXT("reentering flame begins at scaled base damage"));
    H->Restart();H->MaxHealth=H->Health=10000;H->Armor=H->DamageReduction=0;E->SetActorLocation(DungeonView::Unproject({640,410}));H->SetActorLocation(DungeonView::Unproject({640,575}));
    H->SetFlashReviewAim({0,-1});H->BlockPressed();const float GuardHealth=H->Health;
    E->BeginIronAttack(1,H,this);E->TickIronMatriarch(3.25f,H,this);
    Check(FMath::IsNearlyEqual(H->Health,GuardHealth-BreathDamage*10*IncomingDamageScale(true)*.75f,.01f)&&H->Stamina==100,TEXT("facing boss block absorbs only 25 percent without stamina"));H->BlockReleased();
    E->BeginIronAttack(2,H,this);const auto Warned=E->Iron.Meteors;
    H->SetActorLocation(DungeonView::Unproject({640,285}));E->TickIronMatriarch(1,H,this);
    Check(E->Iron.Meteors[0].Target.Equals(Warned[0].Target),TEXT("meteor target never tracks after warning"));
    E->Iron.Cancel();E->TickIronMatriarch(.1f,H,this);Check(E->Iron.Meteors.IsEmpty(),TEXT("explicit cancellation removes all pending effects"));
    const float HP=E->Health;FreedomTime=1;bFreedomResolved=false;UpdateFreedom(1);
    Check(E->Health==HP&&E->FreedomImmuneTime>0,TEXT("FREEDOM immunity and indicator"));
    E->BeginIronAttack(2,H,this);E->TakeDungeonDamage(100000);
    Check(E->Iron.DeathAge==0&&E->Iron.Meteors.IsEmpty()&&E->Iron.Attack<0,TEXT("death cancels pending meteor events"));
    Check(!IsVictory(),TEXT("death presentation finishes before ending"));
    E->TickIronMatriarch(1.9f,H,this);Check(IsVictory(),TEXT("Iron Matriarch defeat ends eighth floor"));
    StartPlaytestRoom(28);SpawnOneEnemy();PendingSpawns=0;CancelBossIntro();DialogueLines.Empty();BossGrace=0;
    if(!Enemies.IsEmpty())EnemyDefeated(Enemies[0]);
    Check(!IsVictory()&&AtlasRooms[5].Cleared,TEXT("Twister now opens eighth floor, not victory"));
    int D=0;while(D<4&&!AtlasIsDescentDoor(D))++D;Check(D<4,TEXT("eighth-floor descent available"));
    if(D<4){StartAtlasTravel(D);TickAtlasTravel(5);Check(AtlasChapter==7&&AtlasRooms.Num()==14,TEXT("eighth floor has expanded persistent map"));}
    UE_LOG(LogTemp,Display,TEXT("IRON_VERIFY_COMPLETE checks=%d errors=%d"),Checks,Errors);FPlatformMisc::RequestExitWithStatus(false,Errors?1:0);
#endif
}
