#include "DungeonActors.h"
#include "HeroMeleeRig.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "HAL/PlatformMisc.h"

void ADungeonGameMode::ReviewMeleeRig()
{
#if !UE_BUILD_SHIPPING
    static bool Started=false;static int Frame=0;
    if(!Started){
        StartGame();InitializeAtlasFloor(12);bMenu=false;PendingSpawns=0;CancelBossIntro();DialogueLines.Empty();BossGrace=0;
        AtlasArrivalTime=AtlasTravelTime=0;bAtlasMap=bTraderOpen=false;Breakables.Empty();
        for(auto& E:Enemies)if(IsValid(E))E->Destroy();Enemies.Empty();
        GEngine->bUseFixedFrameRate=true;GEngine->FixedFrameRate=30;Started=true;
    }
    if(auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0))){H->HurtTime=0;H->QuipTime=0;H->QuipCooldown=100;}
    const bool Blades=FParse::Param(FCommandLine::Get(),TEXT("MeleeBladeReview"));
    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/%s/Frame%04d.png"),Blades?TEXT("MeleeBlades"):TEXT("MeleeRig"),Frame++),false,false);
    if(Frame>=(Blades?840:990))FPlatformMisc::RequestExit(false);
#endif
}

void ADungeonHUD::DrawMeleeRigReview(ADungeonHero* H)
{
#if !UE_BUILD_SHIPPING
    static float Start=GetWorld()->GetTimeSeconds();
    const FLinearColor Gold(1,.76f,.35f),Pale(.82f,.86f,.85f);
    const float T=GetWorld()->GetTimeSeconds()-Start;
    const bool Blades=FParse::Param(FCommandLine::Get(),TEXT("MeleeBladeReview"));
    const int Clip=Blades?int(T)%7:FMath::Min(int(T/3),6);
    const float Phase=Blades?FMath::Frac(T)*8:FMath::Fmod(T,3.f)*(Clip==0?4.f:Clip==2?12.f:8.f);
    Box(0,0,1280,800,FLinearColor(.027f,.036f,.038f,1));
    const TCHAR* Names[]={TEXT("RELAXED CARRY"),TEXT("WALK"),TEXT("RUN"),TEXT("FOREHAND"),TEXT("BACKHAND"),TEXT("COMBO"),TEXT("GUARD")};
    Label(Blades||T<21?FString::Printf(TEXT("ONE-HANDED MELEE / %s"),Names[Clip]):TEXT("ONE-HANDED MELEE / WEAPON GRIP CHECK"),28,22,Gold,1.1f);
    Label(TEXT("Anatomical right hand / original body art / separate weapon layer"),28,52,Pale,.7f);
    const FDungeonItem Original=H->Equipment[0];
    const TCHAR* Directions[]={TEXT("N"),TEXT("NE"),TEXT("E"),TEXT("SE"),TEXT("S"),TEXT("SW"),TEXT("W"),TEXT("NW")};
    for(int D=0;D<8;++D){
        constexpr int BladeIds[]={5,6,15,17};
        const int Id=Blades?BladeIds[FMath::Min(int(T/7),3)]:T<21?0:FMath::Clamp(int((T-21)/4)*8+D,0,23);
        H->Equipment[0].CatalogId=Id;H->Equipment[0].Slot=0;H->Equipment[0].Name=TEXT("Grip review");
        MeleeReviewClip=Blades||T<21?Clip:((int(T*2)%2)?FullBodyArt::Block:FullBodyArt::Idle);
        MeleeReviewDirection=D;MeleeReviewFrame=int(Phase)%8;MeleeReviewFraction=FMath::Frac(Phase);
        FullBodyHero(H,{160.f+(D%4)*320,350.f+(D/4)*360},2.f,FLinearColor::White);
        Label(FString::Printf(TEXT("%s / weapon %d"),Directions[D],Id),100+(D%4)*320,365+(D/4)*360,Pale,.8f);
    }
    H->Equipment[0]=Original;MeleeReviewClip=-1;
#endif
}
