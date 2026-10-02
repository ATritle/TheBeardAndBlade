#include "DungeonActors.h"
#include "Components/InputComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/PlatformMisc.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/Engine.h"
#include "HAL/PlatformTime.h"

void ADungeonGameMode::VerifySettings()
{
#if WITH_EDITOR
    int Checks=0,Errors=0;
    auto Check=[&](bool OK,const TCHAR* Why){++Checks;if(!OK){++Errors;UE_LOG(LogTemp,Error,TEXT("SETTINGS: %s"),Why);}};
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));auto* PC=GetWorld()->GetFirstPlayerController();auto* HUD=PC?Cast<ADungeonHUD>(PC->GetHUD()):nullptr;
    if(!H||!HUD){FPlatformMisc::RequestExitWithStatus(false,1);return;}
    // Exercise persistence in a separate test file, never the player's preferences.
    TGuardValue<FString> TestConfig(GGameUserSettingsIni,FPaths::ProjectSavedDir()/TEXT("SettingsVerification.ini"));
    FConfigFile TestFile;TestFile.bCanSaveAllSections=true;GConfig->Add(GGameUserSettingsIni,TestFile);
    DungeonKeys::Reset(true);
    FString Why;
    Check(!DungeonKeys::Set(DungeonKeys::Potion,EKeys::W,Why,false),TEXT("duplicate movement binding rejected"));
    Check(!DungeonKeys::Set(DungeonKeys::Potion,EKeys::Escape,Why,false),TEXT("escape reserved"));
    Check(!DungeonKeys::Set(DungeonKeys::Potion,EKeys::Enter,Why,false),TEXT("enter reserved"));
    Check(DungeonKeys::Set(DungeonKeys::Potion,EKeys::K,Why,true)&&DungeonKeys::Label(DungeonKeys::Potion)==TEXT("K"),TEXT("potion remap and HUD label"));
    GConfig->UnloadFile(GGameUserSettingsIni);GConfig->LoadFile(GGameUserSettingsIni);DungeonKeys::Reset(false);DungeonKeys::Load(true);Check(DungeonKeys::Key(DungeonKeys::Potion)==EKeys::K,TEXT("binding survives disk reload"));
    Check(DungeonKeys::Set(DungeonKeys::Attack,EKeys::J,Why,false)&&DungeonKeys::Key(DungeonKeys::Attack)==EKeys::J,TEXT("attack remaps away from mouse"));
    Check(DungeonKeys::Set(DungeonKeys::Block,EKeys::LeftMouseButton,Why,false)&&DungeonKeys::Label(DungeonKeys::Block)==TEXT("LMB"),TEXT("mouse graphic key source"));
    H->RebindInput();
    bool PotionBound=false,BlockRelease=false,ClickBound=false;
    for(auto& B:H->InputComponent->KeyBindings){if(B.Chord.Key==EKeys::K&&B.KeyEvent==IE_Pressed)PotionBound=true;if(B.Chord.Key==EKeys::LeftMouseButton&&B.KeyEvent==IE_Released)BlockRelease=true;if(B.Chord.Key==EKeys::LeftMouseButton&&B.KeyEvent==IE_Pressed)ClickBound=true;}
    Check(PotionBound&&BlockRelease&&ClickBound,TEXT("live input rebuilt including releases and fixed UI click"));
    StartGame();AtlasArrivalTime=AtlasTravelTime=0;H->Health=30;H->PotionCharges=2;
    for(auto& B:H->InputComponent->KeyBindings)if(B.Chord.Key==EKeys::K&&B.KeyEvent==IE_Pressed)B.KeyDelegate.Execute(EKeys::K);
    Check(H->PotionCharges==1&&H->Health==67.5f,TEXT("rebound potion invokes gameplay"));
    H->CancelCombatActions();H->MoveKey(0,true);H->MoveKey(1,true);const auto P=H->GetActorLocation();H->Tick(.1f);Check(H->GetActorLocation().Equals(P),TEXT("opposing move keys cancel"));H->MoveKey(1,false);H->Tick(.1f);Check(!H->GetActorLocation().Equals(P),TEXT("release restores held direction"));H->MoveKey(0,false);
    const float Music=MusicVolume,Effects=EffectsVolume,Voice=VoiceVolume,UI=InterfaceVolume;
    MusicVolume=.2f;EffectsVolume=.3f;VoiceVolume=.4f;InterfaceVolume=.5f;
    Check(SoundCategoryGain(TEXT("Sword"))==.3f&&SoundCategoryGain(TEXT("Rifle"))==.3f,TEXT("effects mixer"));
    Check(SoundCategoryGain(TEXT("RimeAttack"))==.4f&&SoundCategoryGain(TEXT("IronHurtVoice"))==.4f,TEXT("voice mixer"));
    Check(SoundCategoryGain(TEXT("UI"))==.5f&&SoundCategoryGain(TEXT("Equip"))==.5f,TEXT("interface mixer"));
    GConfig->SetFloat(TEXT("DungeonAudio"),TEXT("MusicVolume"),MusicVolume,GGameUserSettingsIni);GConfig->Flush(false,GGameUserSettingsIni);MusicVolume=1;LoadAudioSettings();Check(MusicVolume==.2f,TEXT("audio volume reload"));
    MusicVolume=Music;EffectsVolume=Effects;VoiceVolume=Voice;InterfaceVolume=UI;
    ToggleMenu();bShowSettings=true;HUD->OpenSettings();Check(!HUD->DisplaySizes.IsEmpty(),TEXT("display resolutions populated"));
    HUD->SettingsClick(this,{980,345});Check(HUD->SettingsTab==2,TEXT("controls tab navigation"));
    HUD->SettingsClick(this,{460,405});Check(HUD->CaptureBinding==0,TEXT("key capture starts"));ToggleMenu();Check(HUD->CaptureBinding==-1&&bShowSettings,TEXT("escape cancels capture without closing settings"));
    // Exercise the actual polling path, including Unreal's synthetic AnyKey state.
    auto SavedInput=PC->PlayerInput;
    PC->PlayerInput=NewObject<UPlayerInput>(PC);
    PC->PlayerInput->InputKey(FInputKeyEventArgs(nullptr,FInputDeviceId::CreateFromInternalId(0),EKeys::G,IE_Pressed,uint64(0)));
    PC->PlayerInput->GetKeyState(EKeys::G)->EventCounts[IE_Pressed].Add(1);
    Check(PC->WasInputKeyJustPressed(EKeys::AnyKey),TEXT("physical G also activates synthetic AnyKey"));
    HUD->CaptureBinding=DungeonKeys::Tea;HUD->CaptureReady=0;HUD->SettingsTick();
    Check(HUD->CaptureBinding==-1&&DungeonKeys::Key(DungeonKeys::Tea)==EKeys::G&&H->PendingRebind,TEXT("real capture accepts G for Golden tea instead of rejecting AnyKey"));
    H->RebindInput();bool TeaBound=false;
    for(auto& B:H->InputComponent->KeyBindings)if(B.Chord.Key==EKeys::G&&B.KeyEvent==IE_Pressed)TeaBound=true;
    Check(TeaBound&&DungeonKeys::Label(DungeonKeys::Tea)==TEXT("G"),TEXT("captured Golden tea binding updates gameplay and HUD"));
    GConfig->UnloadFile(GGameUserSettingsIni);GConfig->LoadFile(GGameUserSettingsIni);DungeonKeys::Load(true);
    Check(DungeonKeys::Key(DungeonKeys::Tea)==EKeys::G,TEXT("captured G binding persists to disk"));
    PC->PlayerInput=SavedInput;
    auto* U=GEngine->GetGameUserSettings();HUD->OldResolution=U->GetScreenResolution();HUD->OldMode=int(U->GetFullscreenMode());HUD->OldVSync=U->IsVSyncEnabled();HUD->OldFPS=U->GetFrameRateLimit();HUD->OldQuality=FMath::Clamp(U->GetOverallScalabilityLevel(),0,4);
    HUD->OldScalability=MakeShared<Scalability::FQualityLevels>(U->ScalabilityQuality);U->SetOverallScalabilityLevel(4);
    U->SetFrameRateLimit(30);HUD->DisplayDeadline=FPlatformTime::Seconds()-.1;HUD->SettingsTick();
    Check(HUD->DisplayDeadline==0&&U->GetFrameRateLimit()==HUD->OldFPS&&U->GetScreenResolution()==HUD->OldResolution,TEXT("display timeout restores previous values"));
    Check(U->ScalabilityQuality==*HUD->OldScalability,TEXT("display rollback preserves custom quality"));
    DungeonKeys::Reset(false);
    H->RebindInput();HUD->CloseSettings();bShowSettings=false;
    UE_LOG(LogTemp,Display,TEXT("SETTINGS: %d checks, %d errors"),Checks,Errors);
    FPlatformMisc::RequestExitWithStatus(false,Errors?1:0);
#endif
}
