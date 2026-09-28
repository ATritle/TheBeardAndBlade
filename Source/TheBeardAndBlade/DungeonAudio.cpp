#include "DungeonActors.h"
#include "DungeonEnemyAudio.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"
#include "UnrealClient.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"

namespace {
struct FFoleyFamily {const TCHAR* Name;int Count;};
const FFoleyFamily FoleyFamilies[]={
    {TEXT("HumanPain"),3},{TEXT("HumanDeath"),3},{TEXT("HumanSpawn"),2},
    {TEXT("DronePain"),2},{TEXT("DroneDeath"),2},{TEXT("DroneFlight"),1},
    {TEXT("Rifle"),3},{TEXT("Grenade"),1},
    {TEXT("Step"),7},{TEXT("Sword"),4},{TEXT("Equip"),2},
    {TEXT("Door"),1},{TEXT("InventoryOpen"),1},{TEXT("InventoryClose"),1},
    {TEXT("PropBreak"),5},{TEXT("Spawn"),2},{TEXT("EnemyDeath"),5},{TEXT("EnemyPain"),4},
    {TEXT("Explosion"),1},{TEXT("TeaSplash"),1},{TEXT("FlashBang"),1},
    {TEXT("Paper"),1},{TEXT("UI"),1},{TEXT("Roll"),1},{TEXT("Throw"),3},
    {TEXT("Magic"),1},{TEXT("Portal"),1},{TEXT("Chest"),1},{TEXT("Hit"),1}
};
int FoleyCount(const FString& Name){for(const auto& F:FoleyFamilies)if(Name==F.Name)return F.Count;return 0;}
float MusicGain(const FString& Name){return Name==TEXT("MusicDungeon")?.06f:.23f;}
}
void ADungeonGameMode::PlaySound(const FString& Name,float Volume,float Pitch)
{
    if(bEffectsMuted||!GetWorld()) return;
    const double Now=GetWorld()->GetTimeSeconds();
    // Volley and AoE callbacks can fire many times in one frame. Limit each cue.
    const double Gap=Name==TEXT("DroneFlight")?2.3:Name.EndsWith(TEXT("Spawn"))?.24:Name.EndsWith(TEXT("Pain"))?.18:Name.EndsWith(TEXT("Death"))?.13:.055;
    if(const double* Last=LastSoundTime.Find(Name)) if(Now-*Last<Gap) return;
    LastSoundTime.Add(Name,Now);
    FString AssetName=Name;
    const int Count=FoleyCount(Name);
    if(Count) {
        const int* Previous=LastFoleyVariant.Find(Name);
        int Pick=Count==1?0:FMath::RandRange(0,Count-(Previous?2:1));
        if(Count>1&&Previous&&Pick>=*Previous)++Pick;
        LastFoleyVariant.Add(Name,Pick);
        AssetName=FString::Printf(TEXT("Foley%s%d"),*Name,Pick);
    }
    auto& Sound=Sounds.FindOrAdd(AssetName);
    if(!Sound) Sound=LoadObject<USoundBase>(nullptr,*FString::Printf(TEXT("/Game/Audio/%s.%s"),*AssetName,*AssetName));
    if(Sound) UGameplayStatics::PlaySound2D(this,Sound,.48f*Volume*MasterVolume,Pitch);
}
void ADungeonGameMode::StopMusic()
{
    if(IsValid(MusicComponent)){MusicComponent->Stop();MusicComponent->DestroyComponent();}
    MusicComponent=nullptr;MusicName.Empty();
}
void ADungeonGameMode::RefreshMusicVolume()
{
    if(IsValid(MusicComponent)) {
        MusicComponent->SetPaused(bMusicMuted||MasterVolume<=0);
        MusicComponent->SetVolumeMultiplier(IsBossIntroActive()?0.f:MusicGain(MusicName)*MasterVolume);
    }
}
void ADungeonGameMode::UpdateAudio()
{
    const FString Next=HasEnding()?TEXT("MusicEnding"):bMenu?TEXT("MusicMenu"):IsBossRoom()?TEXT("MusicBoss"):TEXT("MusicDungeon");
    if(Next==MusicName&&IsValid(MusicComponent)&&(MusicComponent->IsPlaying()||MusicComponent->bIsPaused)) return;
    // Never abandon a paused/fading component: only one track may own playback.
    StopMusic();
    MusicName=Next;
    if(bMusicMuted||MasterVolume<=0) return;
    auto& Sound=Sounds.FindOrAdd(Next);
    if(!Sound) Sound=LoadObject<USoundBase>(nullptr,*FString::Printf(TEXT("/Game/Audio/%s.%s"),*Next,*Next));
    if(Sound)
    {
        MusicComponent=NewObject<UAudioComponent>(this);
        MusicComponent->bAutoDestroy=false;
        MusicComponent->bStopWhenOwnerDestroyed=true;
        MusicComponent->bIsUISound=true;
        MusicComponent->bAllowSpatialization=false;
        MusicComponent->SetSound(Sound);
        RefreshMusicVolume();
        MusicComponent->RegisterComponent();
        MusicComponent->FadeIn(1.2f,1.f);
    }
}
void ADungeonGameMode::ToggleMusic()
{
    bMusicMuted=!bMusicMuted;
    // AdjustVolume(...,0) finishes playback in UE. Pause instead, preserving the track.
    RefreshMusicVolume();
    if(!bMusicMuted&&MasterVolume>0) UpdateAudio();
    GConfig->SetBool(TEXT("DungeonAudio"),TEXT("MuteMusic"),bMusicMuted,GGameUserSettingsIni);GConfig->Flush(false,GGameUserSettingsIni);
}
void ADungeonGameMode::ToggleEffects()
{
    bEffectsMuted=!bEffectsMuted;
    UE_LOG(LogTemp,Display,TEXT("AUDIO_SFX_MUTE changed=%d menu=%d"),bEffectsMuted,bMenu);
    GConfig->SetBool(TEXT("DungeonAudio"),TEXT("MuteEffects"),bEffectsMuted,GGameUserSettingsIni);GConfig->Flush(false,GGameUserSettingsIni);
    if(!bEffectsMuted) PlaySound(TEXT("UI"));
}
void ADungeonGameMode::SetMasterVolume(float Value,bool Save)
{
    MasterVolume=FMath::Clamp(Value,0.f,1.f);
    RefreshMusicVolume();
    if(!bMusicMuted&&MasterVolume>0) UpdateAudio();
    if(Save) { GConfig->SetFloat(TEXT("DungeonAudio"),TEXT("Volume"),MasterVolume,GGameUserSettingsIni); GConfig->Flush(false,GGameUserSettingsIni); }
}
void ADungeonHero::ToggleMusic() { if(auto* G=Cast<ADungeonGameMode>(UGameplayStatics::GetGameMode(this))) G->ToggleMusic(); }
void ADungeonHero::ToggleEffects() { if(auto* G=Cast<ADungeonGameMode>(UGameplayStatics::GetGameMode(this))) G->ToggleEffects(); }

// Explicit unattended release check; absent this flag it never alters gameplay.
void ADungeonGameMode::RunPackagedSmokeTest()
{
    if(FParse::Param(FCommandLine::Get(),TEXT("EconomyReview"))) {
        static int Stage=0;const float T=GetWorld()->GetTimeSeconds();
        auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));if(!H)return;
        if(Stage==0&&T>2) {
            StartGame();InitializeAtlasFloor(12);H->Coins=500;
            for(int I=0;I<4;++I){FDungeonCoinDrop C;C.Position=FVector2D(450+I*90,420);C.Amount=12+I*7;CoinDrops.Add(C);}
            H->Inventory.Empty();H->AddToInventory(RollItem(48,3,3));H->DiscardItem(0);++Stage;
        }
        if(Stage==1&&T>3){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/EconomyCoins.png"),false,false);++Stage;}
        if(Stage==2&&T>4){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/EconomySpin.png"),false,false);++Stage;}
        if(Stage==3&&T>5){EnterAtlasRoom(6,-1);H->AddToInventory(RollItem(48,3,3));H->ToggleInventory();H->SelectedItem=0;++Stage;}
        if(Stage==4&&T>6){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/EconomySell.png"),false,false);++Stage;}
        if(Stage==5&&T>7){FPlatformMisc::RequestExit(false);++Stage;}
        return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("InventoryAudioVerify")))
    {
        static bool Done=false;if(Done)return;
        auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));if(!H)return;
        Done=true;int Errors=0;StartGame();const bool SavedMute=bEffectsMuted;
        for(bool Muted:{false,true}) {
            bEffectsMuted=Muted;
            for(int I=0;I<20;++I){H->ToggleInventory();H->ToggleInventory();if(H->IsInventoryOpen()||bEffectsMuted!=Muted)++Errors;}
            LastSoundTime.Remove(TEXT("Sword"));PlaySound(TEXT("Sword"),0);
            if(LastSoundTime.Contains(TEXT("Sword"))==Muted)++Errors;
        }
        bEffectsMuted=SavedMute;
        auto* Track=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/MusicDungeon.MusicDungeon"));
        if(!Track||Track->GetDuration()<30||!FMath::IsNearlyEqual(MusicGain(TEXT("MusicDungeon")),.06f))++Errors;
        // Room entry calls CancelBossIntro even in ordinary rooms. It must never
        // restore the old menu gain or bypass the user's volume setting.
        const float SavedVolume=MasterVolume;const bool SavedMusicMute=bMusicMuted;
        bMusicMuted=false;SetMasterVolume(.5f,false);UpdateAudio();
        for(int RoomIndex:{1,0,1}) {
            EnterAtlasRoom(RoomIndex,-1);CancelBossIntro();UpdateAudio();
            if(!IsValid(MusicComponent)||!FMath::IsNearlyEqual(MusicComponent->VolumeMultiplier,.03f))++Errors;
        }
        BossIntroTime=0;RefreshMusicVolume();
        if(!FMath::IsNearlyZero(MusicComponent->VolumeMultiplier))++Errors;
        CancelBossIntro();
        if(!FMath::IsNearlyEqual(MusicComponent->VolumeMultiplier,.03f))++Errors;
        bMusicMuted=true;CancelBossIntro();if(!MusicComponent->bIsPaused)++Errors;
        bMusicMuted=SavedMusicMute;SetMasterVolume(SavedVolume,false);
        UE_LOG(LogTemp,Display,TEXT("INVENTORY_AUDIO_VERIFY errors=%d"),Errors);
        FPlatformMisc::RequestExitWithStatus(false,Errors?1:0);return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("AudioToggleVerify")))
    {
        static int Stage=0,Errors=0; static bool SavedMute=false; static float SavedVolume=1;
        const float T=GetWorld()->GetTimeSeconds();
        if(Stage==0&&T>2) { SavedMute=bMusicMuted;SavedVolume=MasterVolume;bMusicMuted=false;SetMasterVolume(1,false);++Stage; }
        else if(Stage==1&&T>3) { ToggleMusic();++Stage; }
        else if(Stage==2&&T>4) { if(!IsValid(MusicComponent)||!MusicComponent->bIsPaused)++Errors;ToggleMusic();++Stage; }
        else if(Stage==3&&T>5) { if(!IsValid(MusicComponent)||!MusicComponent->IsPlaying()||MusicComponent->bIsPaused)++Errors;SetMasterVolume(0,false);++Stage; }
        else if(Stage==4&&T>6) { SetMasterVolume(.5f,false);++Stage; }
        else if(Stage==5&&T>7)
        {
            if(!IsValid(MusicComponent)||!MusicComponent->IsPlaying()||MusicComponent->bIsPaused||!FMath::IsNearlyEqual(MusicComponent->VolumeMultiplier,.115f))++Errors;
            // Track switches while muted must retire the old component, not
            // leave an unreferenced loop behind. Session shutdown must stop it.
            TWeakObjectPtr<UAudioComponent> Previous=MusicComponent;
            bMusicMuted=true;MusicComponent->SetPaused(true);MusicName=TEXT("VerifyOldTrack");UpdateAudio();
            if(IsValid(MusicComponent)||(Previous.IsValid()&&Previous->IsPlaying()))++Errors;
            bMusicMuted=false;UpdateAudio();
            if(!IsValid(MusicComponent)||MusicComponent->GetOwner()!=this||!MusicComponent->IsPlaying())++Errors;
            Previous=MusicComponent;StopMusic();
            if(IsValid(MusicComponent)||(Previous.IsValid()&&Previous->IsPlaying()))++Errors;
            bMusicMuted=SavedMute;SetMasterVolume(SavedVolume);
            GConfig->SetBool(TEXT("DungeonAudio"),TEXT("MuteMusic"),SavedMute,GGameUserSettingsIni);GConfig->Flush(false,GGameUserSettingsIni);
            UE_LOG(LogTemp,Display,TEXT("AUDIO_TOGGLE_VERIFY errors=%d"),Errors);++Stage;FPlatformMisc::RequestExitWithStatus(false,Errors?1:0);
        }
        return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("DungeonFreedomPreview")))
    {
        static int Step=0; const float T=GetWorld()->GetTimeSeconds();
        auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));
        if(Step==0&&T>2&&H)
        {
            StartGame(); PendingSpawns=0;
            for(int I=0;I<4;++I) { SpawnOneEnemy(); Enemies.Last()->SpawnTime=0; }
            FreedomKills=15; ActivateFreedom(H); ++Step;
        }
        if(Step==1&&T>3.45f) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Freedom.png"),false,false); ++Step; }
        if(Step==2&&T>5.2f) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/FreedomAftermath.png"),false,false); ++Step; }
        if(Step==3&&T>6.f) { FPlatformMisc::RequestExit(false); ++Step; }
        return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("DungeonVitalsPreview")))
    {
        static int Stage=0; const float T=GetWorld()->GetTimeSeconds();
        auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));
        if(Stage==0&&T>2&&H)
        {
            StartGame(); PendingSpawns=0; H->Health=85; H->SpendStamina(65);
            FDungeonPotion P; P.Position=FVector2D(760,530); Potions.Add(P); ++Stage;
        }
        if(Stage==1&&T>2.3f) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/VitalsHUD.png"),false,false); ++Stage; }
        if(Stage==2&&T>4) { ++Stage; FPlatformMisc::RequestExit(false); }
        return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("DungeonDialogueSmokeTest")))
    {
        static int Step=0,Errors=0;
        const float Time=GetWorld()->GetTimeSeconds();
        auto* Hero=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));
        if(Step==0&&Time>2&&Hero) { StartGame(); Room=DungeonProgression::BossRoom(24); SpawnWave(); ++Step; }
        if(Step==1&&Time>5)
        {
            FinishBossIntro(); // This harness reviews dialogue; IntroVerify covers the cinematic.
            if(!IsBossDialogueActive()||!IsGameplayBlocked()) ++Errors;
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/BossDialogue.png"),false,false); ++Step;
        }
        if(Step==2&&Time>6) { AdvanceBossDialogue(); ++Step; }
        if(Step==3&&Time>7)
        {
            if(DialogueIndex!=1||!IsGameplayBlocked()) ++Errors;
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/BossReply.png"),false,false); ++Step;
        }
        if(Step==4&&Time>8) { AdvanceBossDialogue(true); ++Step; }
        if(Step==5&&Time>9&&Hero)
        {
            if(IsGameplayBlocked()) ++Errors;
            Hero->Attack(); if(!Hero->IsAttacking()) ++Errors;
            Hero->AttackQuip=TEXT("Take this you C*NT!"); Hero->QuipTime=1.5f;
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/AttackQuip.png"),false,false); ++Step;
        }
        if(Step==6&&Time>11)
        {
            FFileHelper::SaveStringToFile(FString::Printf(TEXT("DIALOGUE_SMOKE errors=%d; introduction; boss reply; skip; combat resume; quip render\n"),Errors),*(FPaths::ProjectSavedDir()/TEXT("DialogueSmokeTest.txt")));
            ++Step; FPlatformMisc::RequestExitWithStatus(false,Errors?1:0);
        }
        return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("FoleyVerify")))
    {
        static bool Done=false;if(Done)return;Done=true;
        int Errors=0;
        const bool WasMuted=bEffectsMuted;bEffectsMuted=false;
        for(int Species:{30,37,38,39,41,42}) {
            if(!DungeonEnemyAudio::Human(Species)||FString(DungeonEnemyAudio::Hit(Species,true))!=TEXT("Hit")||FString(DungeonEnemyAudio::Hit(Species,false))!=TEXT("HumanPain"))++Errors;
        }
        if(DungeonEnemyAudio::Human(40)||FString(DungeonEnemyAudio::Hit(40,true))!=TEXT("DroneDeath")||FString(DungeonEnemyAudio::Spawn(40))!=TEXT("DroneFlight"))++Errors;
        if(FString(DungeonEnemyAudio::Hit(25,true))!=TEXT("EnemyDeath"))++Errors;
        if(DungeonEnemyAudio::Human(24)||FString(DungeonEnemyAudio::Hit(24,true))!=TEXT("Hit")||FString(DungeonEnemyAudio::Hit(24,false))!=TEXT("Hit")||FString(DungeonEnemyAudio::Spawn(24))!=TEXT("Paper"))++Errors;
        int Assets=0;
        for(const auto& Entry:FoleyFamilies) {
            const TCHAR* Family=Entry.Name;const int Count=Entry.Count;Assets+=Count;
            for(int I=0;I<Count;++I) {
                const FString Name=FString::Printf(TEXT("Foley%s%d"),Family,I);
                auto* Sound=LoadObject<USoundBase>(nullptr,*FString::Printf(TEXT("/Game/Audio/%s.%s"),*Name,*Name));
                if(!Sound||Sound->GetDuration()<.04f||Sound->GetDuration()>3.f)++Errors;
            }
            int Previous=-1;TSet<int> Seen;
            for(int I=0;I<100;++I) {
                LastSoundTime.Remove(Family);PlaySound(Family,0);
                const int Pick=LastFoleyVariant.FindRef(Family);
                if((Count>1&&Pick==Previous)||Pick<0||Pick>=Count)++Errors;
                Seen.Add(Pick);Previous=Pick;
            }
            if(Seen.Num()!=Count)++Errors;
            bEffectsMuted=true;LastSoundTime.Remove(Family);PlaySound(Family);
            if(LastSoundTime.Contains(Family)||LastFoleyVariant.FindRef(Family)!=Previous)++Errors;
            bEffectsMuted=false;
        }
        bEffectsMuted=WasMuted;
        UE_LOG(LogTemp,Display,TEXT("FOLEY_VERIFY_COMPLETE errors=%d; %d assets; variation; mute"),Errors,Assets);
        FPlatformMisc::RequestExitWithStatus(false,Errors?1:0);return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("TraderComparisonReview")))
    {
        static int Step=0;const float Time=GetWorld()->GetTimeSeconds();
        auto* Hero=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));if(!Hero)return;
        auto* PC=GetWorld()->GetFirstPlayerController();auto* HUD=PC?Cast<ADungeonHUD>(PC->GetHUD()):nullptr;if(!HUD)return;
        if(Step==0&&Time>2) {
            StartGame();EnterAtlasRoom(6,-1);Hero->Coins=2500;TraderStock.Empty();
            auto Gear=RollItem(69,4,8),Offer=RollItem(70,4,8);
            Gear.Defense=8;Gear.Vitality=10;Gear.Speed=.12f;Gear.Movement=.15f;Gear.CritDamage=.20f;
            Offer.Defense=0;Offer.Vitality=16;Offer.Speed=.14f;Offer.Movement=.123f;Offer.CritDamage=.20f;
            Hero->Equip(Gear);TraderStock.Add(Offer);
            Hero->Equip(RollItem(0,4,8));TraderStock.Add(RollItem(8,4,8));
            Hero->Equip(RollItem(48,4,8));TraderStock.Add(RollItem(49,4,8));
            FDungeonItem Empty;Empty.Slot=4;Hero->Equip(Empty);TraderStock.Add(RollItem(62,4,8));
            PC->SetMouseLocation(10,10);HUD->TraderSelection=0;++Step;
        }
        if(Step>=1&&Step<=4&&Time>3+Step*2) {
            HUD->TraderSelection=Step-1;
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/TraderCompare%d.png"),Step),false,false);++Step;
        }
        if(Step==5&&Time>13){UE_LOG(LogTemp,Display,TEXT("TRADER_COMPARISON_REVIEW_COMPLETE"));++Step;FPlatformMisc::RequestExitWithStatus(false,0);}
        return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("GearReview")))
    {
        static int Step=0,Errors=0;const float Time=GetWorld()->GetTimeSeconds();
        auto* Hero=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));if(!Hero)return;
        auto* PC=GetWorld()->GetFirstPlayerController();auto* HUD=PC?Cast<ADungeonHUD>(PC->GetHUD()):nullptr;if(!HUD)return;
        if(Step==0&&Time>2) {
            StartGame();Hero->Inventory.Empty();Hero->ToggleInventory();
            Errors+=HUD->VerifyInventoryGestures(Hero);
            for(int ID=60;ID<72;++ID)Hero->AddToInventory(RollItem(ID,ID%3+2,4));
            for(int I=0;I<4;++I)if(!LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Art/V2/GearSheet_%d.GearSheet_%d"),I,I)))++Errors;
            for(int S=4;S<8;++S)Hero->Equip(RollItem(62+(S-4)*3,4,4));
            Hero->SelectedItem=0;PC->SetMouseLocation(10,10);++Step;
        }
        if(Step==1&&Time>4){Hero->SelectedItem=0;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/GearHead.png"),false,false);++Step;}
        if(Step==2&&Time>5){Hero->SelectedItem=3;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/GearHands.png"),false,false);++Step;}
        if(Step==3&&Time>6){Hero->SelectedItem=6;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/GearLegs.png"),false,false);++Step;}
        if(Step==4&&Time>7){Hero->SelectedItem=9;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/GearFeet.png"),false,false);++Step;}
        if(Step==5&&Time>8){Hero->ToggleInventory();EnterAtlasRoom(6,-1);Hero->Coins=2500;++Step;}
        if(Step==6&&Time>9){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/GearTrader.png"),false,false);++Step;}
        if(Step==7&&Time>10){UE_LOG(LogTemp,Display,TEXT("GEAR_REVIEW_COMPLETE errors=%d"),Errors);++Step;FPlatformMisc::RequestExitWithStatus(false,Errors?1:0);}
        return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("DungeonLootSmoke")))
    {
        static int Step=0,Errors=0; const float Time=GetWorld()->GetTimeSeconds();
        auto* Hero=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0)); if(!Hero) return;
        if(Step==0&&Time>2) {
            StartGame();Hero->AddToInventory(RollItem(8,4,4));Hero->AddToInventory(RollItem(28,3,4));Hero->AddToInventory(RollItem(41,2,4));
            Hero->Equip(RollItem(8,4,4));Hero->ToggleInventory();Hero->SelectedItem=INDEX_NONE;
            if(auto* PC=Cast<APlayerController>(Hero->GetController())) {
                if(auto* HUD=Cast<ADungeonHUD>(PC->GetHUD())) Errors+=HUD->VerifyInventoryGestures(Hero);
                int W=0,H=0;PC->GetViewportSize(W,H);float Scale=FMath::Min(W/1280.f,H/800.f);PC->SetMouseLocation((W-1280*Scale)/2+489*Scale,(H-800*Scale)/2+230*Scale);}
            for(int I=0;I<60;++I) if(!LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Art/V2/Loot_%d.Loot_%d"),I,I))) ++Errors;
            for(int I=0;I<4;++I) if(!LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Art/V2/GearSheet_%d.GearSheet_%d"),I,I))) ++Errors;
            if(!LoadObject<UTexture2D>(nullptr,TEXT("/Game/Art/V2/InventoryFrame.InventoryFrame"))) ++Errors;
            ++Step;
        }
        if(Step==1&&Time>4) {FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/LootHover.png"),false,false);++Step;}
        if(Step==2&&Time>5) {
            Hero->ToggleInventory();while(PendingSpawns>0){SpawnOneEnemy();--PendingSpawns;}
            int I=0;for(auto& E:Enemies) {E->SpawnTime=0;E->Windup=0;E->Recovery=20;E->Health=E->MaxHealth=1000;E->SetActorLocation(DungeonView::Unproject(FVector2D(400+I*140,420)));if(I++%2){E->BleedTime=3;E->BleedDPS=10;}else{E->PoisonTime=4;E->PoisonDPS=10;}}
            ++Step;
        }
        if(Step==3&&Time>6.2f){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/LootEffects.png"),false,false);++Step;}
        if(Step==4&&Time>10) {
            for(auto& E:Enemies) if(E->BleedTime>0||E->PoisonTime>0||E->Health>=1000) ++Errors;
            FFileHelper::SaveStringToFile(FString::Printf(TEXT("LOOT_SMOKE errors=%d; 60 item textures; ring gestures; unselected hover card; ailment rendering and expiration\n"),Errors),*(FPaths::ProjectSavedDir()/TEXT("LootSmokeTest.txt")));
            ++Step;FPlatformMisc::RequestExitWithStatus(false,Errors?1:0);
        }
        return;
    }
    if(!FParse::Param(FCommandLine::Get(),TEXT("DungeonSmokeTest"))) return;
    static int Stage=0; const float T=GetWorld()->GetTimeSeconds();
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));
    if(Stage==0&&T>2) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/SmokeMenu.png"),false,false); ++Stage; }
    if(Stage==1&&T>3&&H) { StartGame(); ++Stage; }
    if(Stage==2&&T>4&&H) { H->PowerMove(); ++Stage; }
    if(Stage==3&&T>4.8f) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/SmokeGame.png"),false,false); ++Stage; }
    if(Stage==4&&T>7)
    {
        int Errors=0;
        for(const TCHAR* Name:{TEXT("MusicMenu"),TEXT("MusicDungeon"),TEXT("MusicBoss"),TEXT("Sword"),TEXT("Roll"),TEXT("Hit"),TEXT("Explosion"),TEXT("TeaSplash"),TEXT("Throw"),TEXT("Paper"),TEXT("Magic"),TEXT("Step"),TEXT("Hurt"),TEXT("Death"),TEXT("Chest"),TEXT("Equip"),TEXT("Portal"),TEXT("UI"),TEXT("Spawn")})
            if(!LoadObject<USoundBase>(nullptr,*FString::Printf(TEXT("/Game/Audio/%s.%s"),Name,Name))) ++Errors;
        if(!H||H->GetPowerCooldown()<=0||bMenu||!MusicComponent||!MusicComponent->IsPlaying()) ++Errors;
        FFileHelper::SaveStringToFile(FString::Printf(TEXT("PACKAGED_SMOKE errors=%d; audio assets=19; menu->game; tea cooldown; music component playing\n"),Errors),*(FPaths::ProjectSavedDir()/TEXT("SmokeTest.txt")));
        ++Stage; FPlatformMisc::RequestExitWithStatus(false,Errors?1:0);
    }
}
