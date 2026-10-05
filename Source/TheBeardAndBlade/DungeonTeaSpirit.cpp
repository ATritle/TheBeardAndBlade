#include "DungeonTeaSpirit.h"
#include "DungeonActors.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "GlobalRenderResources.h"
#include "Engine/Texture2D.h"

bool ADungeonHero::CanDrinkTea() const
{
    return CanStrike()&&TeaSpirit.Ready();
}
void ADungeonHero::DrinkTea()
{
    if(!CanDrinkTea()||!TeaSpirit.Start())return;
    // Protection begins on press; the short sip is part of the ten-second window.
    SlowTime=FlashBlindTime=HurtTime=0;
    if(auto* G=Cast<ADungeonGameMode>(UGameplayStatics::GetGameMode(this)))
        G->PlaySound(TEXT("Equip"),.45f);
}

void ADungeonGameMode::VerifyTeaSpirit()
{
#if WITH_EDITOR
    int Errors=0,Checks=0;
    auto Check=[&](bool OK,const TCHAR* Why){++Checks;if(!OK){++Errors;UE_LOG(LogTemp,Error,TEXT("TEA SPIRIT: %s"),Why);}};
    StartGame();
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));
    if(!H){FPlatformMisc::RequestExitWithStatus(false,1);return;}
    auto* Sheet=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Art/TeaSpirit/TeaSpirit_DrinkSheet.TeaSpirit_DrinkSheet"));
    // Headless verification can precede asynchronous GPU texture compilation.
    // UV measurements refer to the imported source, not a transient RHI size.
    Check(Sheet&&Sheet->Source.GetSizeX()==1536&&Sheet->Source.GetSizeY()==1024,TEXT("authored sheet loads with measured dimensions"));
    H->Restart();H->DrinkTea();
    Check(H->IsTeaEmpowered()&&H->IsDrinkingTea(),TEXT("activation"));
    Check(!H->CanUseTea()&&!H->CanStrike()&&!H->CanStartBlock(),TEXT("sip locks other hand actions"));
    const float HP=H->Health;
    for(bool Boss:{false,true})for(bool Projectile:{false,true})H->ReceiveHit(10000,Projectile,{},Boss);
    H->ReceiveFlashStab();H->ApplyBurgerStatus();
    Check(H->Health==HP&&H->StunTime==0&&H->SlowTime==0,TEXT("normal, boss, projectile and special immunity"));
    Check(!H->ApplyFlashBang(DungeonView::Project(H->GetActorLocation()),200,true)&&H->FlashBlindTime==0,TEXT("flash immunity"));
    H->DrinkTea();Check(H->GetTeaSpiritTime()==10,TEXT("repeat cannot refresh"));
    H->Tick(FDungeonTeaSpirit::SipDuration);Check(!H->IsDrinkingTea()&&H->CanStrike(),TEXT("attack allowed after sip"));
    H->MoveRight(1);const auto Before=H->GetActorLocation();H->Tick(.1f);H->MoveRight(0);
    Check(FMath::IsNearlyEqual(float((DungeonView::Project(H->GetActorLocation())-DungeonView::Project(Before)).Size()),28.5f,.01f),TEXT("50 percent speed increase"));
    H->Tick(5-FDungeonTeaSpirit::SipDuration-.1f);
    H->ReceiveHit(10000,true,{},true);
    Check(H->IsTeaEmpowered()&&H->Health==HP&&H->GetTeaSpiritCooldown()==0,TEXT("protection continues after five seconds"));
    H->Tick(5);H->Tick(.001f);
    Check(!H->IsTeaEmpowered()&&H->GetTeaSpiritCooldown()>29.9f&&!H->CanDrinkTea(),TEXT("ten second expiry and recharge"));
    H->ReceiveHit(10,true,{},true);Check(H->Health<HP,TEXT("damage resumes"));
    H->Tick(30);Check(H->CanDrinkTea(),TEXT("recharge completes"));
    H->DrinkTea();H->Restart();Check(!H->IsTeaEmpowered()&&H->GetTeaSpiritCooldown()==0,TEXT("new run resets"));
    UE_LOG(LogTemp,Display,TEXT("TEA SPIRIT: %d checks, %d errors"),Checks,Errors);
    FPlatformMisc::RequestExitWithStatus(false,Errors?1:0);
#endif
}

void ADungeonHUD::DrawTeaSpiritAura(ADungeonHero* H,FVector2D Position,float HeroScale,float Opacity)
{
    if(!Canvas||!H->IsTeaEmpowered())return;
    const float Elapsed=FDungeonTeaSpirit::Duration-H->GetTeaSpiritTime();
    const float Fade=FMath::Clamp(Elapsed/.18f,0.f,1.f)*FMath::Clamp(H->GetTeaSpiritTime()/.35f,0.f,1.f)*Opacity;
    const FVector2D Center=Position-FVector2D(0,52*HeroScale);
    TArray<FCanvasUVTri> Triangles;
    Triangles.Reserve(864);
    auto Point=[&](float Angle,float Radius){
        return Offset+(Center+FVector2D(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius*1.2f)*HeroScale)*Scale;
    };
    auto Tri=[&](FVector2D A,FVector2D B,FVector2D C,float AlphaA,float AlphaB,float AlphaC){
        FCanvasUVTri T;T.V0_Pos=A;T.V1_Pos=B;T.V2_Pos=C;
        T.V0_UV=T.V1_UV=T.V2_UV=FVector2D::ZeroVector;
        T.V0_Color=FLinearColor(1,.74f,.25f,AlphaA);
        T.V1_Color=FLinearColor(1,.74f,.25f,AlphaB);
        T.V2_Color=FLinearColor(1,.74f,.25f,AlphaC);
        Triangles.Add(T);
    };
    // Each wave expands monotonically, with transparent inner/outer edges.
    // Vertex-alpha feathering avoids hard rings, texture rectangles and additive halos.
    const int Latest=FMath::FloorToInt(Elapsed/.55f);
    for(int Wave=FMath::Max(0,Latest-2);Wave<=Latest;++Wave){
        const float Age=Elapsed-Wave*.55f,T=Age/1.65f;
        if(T<0||T>=1)continue;
        const float R=DungeonTeaDrink::AuraRadius(Age),Width=8.f+8.f*T;
        const float Alpha=.23f*FMath::Sin(PI*T)*(1-T)*Fade;
        for(int Segment=0;Segment<72;++Segment){
            const float A=2*PI*Segment/72,B=2*PI*(Segment+1)/72;
            const auto AI=Point(A,R-Width),AM=Point(A,R),AO=Point(A,R+Width);
            const auto BI=Point(B,R-Width),BM=Point(B,R),BO=Point(B,R+Width);
            Tri(AI,AM,BM,0,Alpha,Alpha);Tri(AI,BM,BI,0,Alpha,0);
            Tri(AM,AO,BO,Alpha,0,0);Tri(AM,BO,BM,Alpha,0,Alpha);
        }
    }
    if(!Triangles.IsEmpty()){
        FCanvasTriangleItem Item(Triangles,GWhiteTexture);
        Item.BlendMode=SE_BLEND_Translucent;
        Canvas->DrawItem(Item);
    }
}

void ADungeonGameMode::ReviewTeaSpirit(float Dt)
{
#if !UE_BUILD_SHIPPING
    const bool PotionReview=FParse::Param(FCommandLine::Get(),TEXT("PotionReview"));
    if(!PotionReview&&!FParse::Param(FCommandLine::Get(),TEXT("TeaSpiritReview")))return;
    static bool Started=false,Activated=false;
    static float Time=0;
    static int Frame=-1;
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));if(!H)return;
    if(!Started){
        Started=true;StartGame();CancelBossIntro();DialogueLines.Empty();BossGrace=0;
        AtlasArrivalTime=AtlasTravelTime=0;bAtlasMap=bTraderOpen=false;PendingSpawns=0;
        for(auto& E:Enemies)if(IsValid(E))E->Destroy();Enemies.Empty();
        H->Restart();H->SetActorLocation(DungeonView::Unproject({540,500}));
        if(PotionReview){H->PotionCharges=4;H->Health=50;}
    }
    Time+=Dt;
    if(Time>=1&&!Activated){Activated=true;if(PotionReview)H->DrinkPotion();else H->DrinkTea();}
    if(PotionReview&&Time>3)H->PotionCharges=Time>6?0:2;
    const int N=int(Time*20);
    if(N!=Frame){
        Frame=N;
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectDir()/(PotionReview?TEXT("Saved/PotionBorderReview"):TEXT("Saved/TeaSpiritReviewV2"))/FString::Printf(TEXT("frame-%04d.png"),N),true,false);
    }
    if(Time>(PotionReview?8:FDungeonTeaSpirit::Duration+3))FPlatformMisc::RequestExit(false);
#endif
}
