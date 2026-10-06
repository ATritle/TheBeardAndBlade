#include "DungeonActors.h"
#include "DungeonExpansion.h"
#include "DungeonSkillFX.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "HAL/PlatformMisc.h"

void ADungeonGameMode::VerifySkillFX()
{
#if !UE_BUILD_SHIPPING
 int Checks=0,Errors=0;auto Check=[&](bool OK,const TCHAR* Name){++Checks;if(!OK){++Errors;UE_LOG(LogTemp,Error,TEXT("SKILL_FX %s"),Name);}};
 StartGame();auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));
 for(int Style:{1,8,10,11,12,13,14,15})for(bool Boss:{false,true}){
  H->Restart();H->SetActorLocation(DungeonView::Unproject({300,600}));Splashes.Empty();
  FDungeonShot S;S.Style=Style;S.bFriendly=Style==8;S.bBossAttack=Boss;S.Origin=S.Position={900,300};S.Target={900,300};S.Damage=7;S.BlastRadius=35;
  const auto Before=S;DungeonSkillFX::Trail(GetWorld(),S,{900,300},{930,300});
  Check(S.Damage==Before.Damage&&S.Life==Before.Life&&S.BlastRadius==Before.BlastRadius&&S.Position==Before.Position,TEXT("trail leaves gameplay data untouched"));
  Check(DungeonSkillFX::Impact(GetWorld(),S,{900,300})==!Boss,TEXT("boss effects excluded"));
  ResolveProjectile(S,{900,300});
  Check(H->Health==H->MaxHealth,TEXT("cosmetics do not extend damage coverage"));
  if(!Splashes.IsEmpty())Check(Splashes.Last().bNiagara==!Boss,TEXT("legacy boss renderer retained"));
 }
 for(int Species=0;Species<79;++Species){const auto C=DungeonSkillFX::Color(Species);Check(C.R>=0&&C.R<=1&&C.G>=0&&C.G<=1&&C.B>=0&&C.B<=1,TEXT("all enemy palette values finite and bounded"));}
 UE_LOG(LogTemp,Display,TEXT("SKILL_FX_VERIFY checks=%d errors=%d"),Checks,Errors);FPlatformMisc::RequestExitWithStatus(false,Errors?1:0);
#endif
}

void ADungeonGameMode::ReviewSkillFX()
{
#if !UE_BUILD_SHIPPING
 static int Frame=-1,Beat=-1;static float Start=-1;
 auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));if(!H)return;
 if(Start<0){
  StartGame();InitializeAtlasFloor(12);bMenu=false;PendingSpawns=0;CancelBossIntro();DialogueLines.Empty();BossGrace=0;
  AtlasArrivalTime=AtlasTravelTime=0;bAtlasMap=bTraderOpen=false;Breakables.Empty();
  for(auto& E:Enemies)if(IsValid(E))E->Destroy();Enemies.Empty();
  H->SetActorLocation(DungeonView::Unproject({570,490}));H->Health=H->MaxHealth*.5f;H->PotionCharges=4;
  auto* Target=GetWorld()->SpawnActor<ADungeonEnemy>();Target->Species=49;Target->Health=Target->MaxHealth=1000;Target->SpawnTime=0;Target->Recovery=100;Target->SetActorLocation(DungeonView::Unproject({785,485}));Enemies.Add(Target);
  GEngine->bUseFixedFrameRate=true;GEngine->FixedFrameRate=30;Start=GetWorld()->GetTimeSeconds();
 }
 const float T=GetWorld()->GetTimeSeconds()-Start;const int B=int(T);
 H->QuipTime=0;H->QuipCooldown=100;H->UIClickFrame=MAX_uint64;
 bChest=false;
 if(B!=Beat){Beat=B;
  if(B==1)H->DrinkPotion();
  if(B==3)H->DrinkTea();
  if(B==5){H->SetFlashReviewAim({1,0});if(auto* PC=GetWorld()->GetFirstPlayerController())PC->SetMouseLocation(775,480);H->PowerMove();}
  if(B==10){for(auto& E:Enemies)if(IsValid(E))E->Health=10;FreedomKills=15;ActivateFreedom(H);}
  if(B==14||B==18||B==22||B==26||B==30||B==34){
   for(auto& E:Enemies)if(IsValid(E))E->Destroy();Enemies.Empty();Shots.Empty();
   const int Species=B==14?59:B==18?66:B==22?77:B==26?57:B==30?70:49;
   auto* E=GetWorld()->SpawnActor<ADungeonEnemy>();E->Species=Species;E->Health=E->MaxHealth=10000;E->SetActorLocation(DungeonView::Unproject({825,475}));Enemies.Add(E);
  }
 }
 if(T>=14){H->Health=H->MaxHealth;H->MoveForward(T<34?FMath::Sin(T*1.5f)*.65f:0);}
 ++Frame;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/SkillNiagara/Frame%04d.png"),Frame),false,false);
 if(T>=39)FPlatformMisc::RequestExit(false);
#endif
}
