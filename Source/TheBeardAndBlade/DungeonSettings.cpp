#include "DungeonSettings.h"
#include "DungeonActors.h"
#include "Components/InputComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "HAL/PlatformTime.h"
#include "Engine/Engine.h"
#include "Engine/Canvas.h"
#include "RHI.h"

namespace DungeonKeys
{
static const TCHAR* Names[]={TEXT("Move up"),TEXT("Move down"),TEXT("Move left"),TEXT("Move right"),TEXT("Weapon attack"),TEXT("Throw tea"),TEXT("Block"),TEXT("FREEDOM"),TEXT("Golden tea"),TEXT("Health potion"),TEXT("Interact"),TEXT("Inventory"),TEXT("Dodge"),TEXT("Sprint"),TEXT("Map"),TEXT("Pause")};
static const FKey Defaults[]={EKeys::W,EKeys::S,EKeys::A,EKeys::D,EKeys::LeftMouseButton,EKeys::MiddleMouseButton,EKeys::RightMouseButton,EKeys::One,EKeys::Two,EKeys::Q,EKeys::E,EKeys::I,EKeys::SpaceBar,EKeys::LeftShift,EKeys::M,EKeys::P};
static FKey Keys[Count];
static bool Loaded=false;
const TCHAR* Name(int A){return A>=0&&A<Count?Names[A]:TEXT("");}
void Save(){for(int A=0;A<Count;++A)GConfig->SetString(TEXT("DungeonKeys"),Names[A],*Keys[A].GetFName().ToString(),GGameUserSettingsIni);GConfig->Flush(false,GGameUserSettingsIni);}
void Reset(bool Persist){for(int A=0;A<Count;++A)Keys[A]=Defaults[A];Loaded=true;if(Persist)Save();}
void Load(bool Force){if(Loaded&&!Force)return;Reset(false);for(int A=0;A<Count;++A){FString S; if(GConfig->GetString(TEXT("DungeonKeys"),Names[A],S,GGameUserSettingsIni)){FKey K(*S);if(K.IsValid()&&!K.IsGamepadKey()&&!K.IsAxis1D()&&!K.IsAxis2D()&&K!=EKeys::Escape&&K!=EKeys::Enter&&K!=EKeys::AnyKey&&K!=EKeys::F11&&K!=EKeys::Tilde)Keys[A]=K;}}for(int A=0;A<Count;++A)for(int B=A+1;B<Count;++B)if(Keys[A]==Keys[B]){Reset(false);return;}}
FKey Key(int A){Load();return A>=0&&A<Count?Keys[A]:EKeys::Invalid;}
FString Label(int A){const auto K=Key(A);if(K==EKeys::LeftMouseButton)return TEXT("LMB");if(K==EKeys::RightMouseButton)return TEXT("RMB");if(K==EKeys::MiddleMouseButton)return TEXT("MMB");if(K==EKeys::LeftShift)return TEXT("SHIFT");if(K==EKeys::RightShift)return TEXT("RSHFT");if(K==EKeys::LeftControl)return TEXT("CTRL");if(K==EKeys::RightControl)return TEXT("RCTRL");if(K==EKeys::LeftAlt)return TEXT("ALT");if(K==EKeys::RightAlt)return TEXT("RALT");if(K==EKeys::PageUp)return TEXT("PGUP");if(K==EKeys::PageDown)return TEXT("PGDN");if(K==EKeys::SpaceBar)return TEXT("SPACE");return K.GetDisplayName().ToString().ToUpper();}
bool Set(int A,FKey K,FString& Error,bool Persist){Load();if(A<0||A>=Count||!K.IsValid()||K.IsGamepadKey()||K.IsAxis1D()||K.IsAxis2D()||K==EKeys::Escape||K==EKeys::Enter||K==EKeys::AnyKey||K==EKeys::F11||K==EKeys::Tilde){Error=TEXT("That key is reserved or unsupported. Try another.");return false;}for(int B=0;B<Count;++B)if(B!=A&&Keys[B]==K){Error=FString::Printf(TEXT("Already assigned to %s. Choose another key."),Names[B]);return false;}Keys[A]=K;Error=TEXT("Binding updated. HUD and controls updated.");if(Persist)Save();return true;}
}

void ADungeonHero::MoveKey(int D,bool Held){HeldDirections[D]=Held;InputX=int(HeldDirections[3])-int(HeldDirections[2]);InputY=int(HeldDirections[1])-int(HeldDirections[0]);}
void ADungeonHero::RebindInput()
{
    if(!InputComponent)return;
    ResetMeleeChain();Block.Release();bSprinting=false;InputX=InputY=0;for(bool& B:HeldDirections)B=false;
    InputComponent->ClearActionBindings();InputComponent->AxisBindings.Empty();InputComponent->AxisKeyBindings.Empty();InputComponent->KeyBindings.Empty();
    SetupPlayerInputComponent(InputComponent);PendingRebind=false;
}
void ADungeonHero::InputClick()
{
    auto* G=Cast<ADungeonGameMode>(UGameplayStatics::GetGameMode(this));
    if(G&&(G->IsMenu()||G->IsTraderOpen()||G->HasEnding()||G->IsBossIntroActive()||G->IsBossDialogueActive()||IsInventoryOpen())){UIClickFrame=GFrameCounter;Attack();}
}
void ADungeonHero::GameplayAttack()
{
    auto* G=Cast<ADungeonGameMode>(UGameplayStatics::GetGameMode(this));
    if(UIClickFrame!=GFrameCounter&&G&&!G->IsGameplayBlocked()&&!IsInventoryOpen()&&Health>0&&StunTime<=0&&!IsBlocking()&&!IsRolling()&&!IsDrinking()&&!IsCasting())
    {bAttackHeld=true;Attack();}
}

void ADungeonGameMode::LoadAudioSettings()
{
    float* Values[]={&MusicVolume,&EffectsVolume,&VoiceVolume,&InterfaceVolume};const TCHAR* Names[]={TEXT("MusicVolume"),TEXT("EffectsVolume"),TEXT("VoiceVolume"),TEXT("InterfaceVolume")};
    for(int I=0;I<4;++I){GConfig->GetFloat(TEXT("DungeonAudio"),Names[I],*Values[I],GGameUserSettingsIni);*Values[I]=FMath::Clamp(*Values[I],0.f,1.f);}
}
void ADungeonGameMode::SaveAudioSettings()
{
    const float Values[]={MusicVolume,EffectsVolume,VoiceVolume,InterfaceVolume};const TCHAR* Names[]={TEXT("MusicVolume"),TEXT("EffectsVolume"),TEXT("VoiceVolume"),TEXT("InterfaceVolume")};
    for(int I=0;I<4;++I)GConfig->SetFloat(TEXT("DungeonAudio"),Names[I],Values[I],GGameUserSettingsIni);
    SetMasterVolume(GetMasterVolume(),true);
}
float ADungeonGameMode::SoundCategoryGain(const FString& Name) const
{
    if(Name==TEXT("UI")||Name==TEXT("Equip")||Name.StartsWith(TEXT("Inventory")))return InterfaceVolume;
    if(Name.Contains(TEXT("Voice"))||Name.StartsWith(TEXT("Human"))||Name.StartsWith(TEXT("Rime")))return VoiceVolume;
    return EffectsVolume;
}

namespace {const float FPSValues[]={0,30,60,90,120,144,165,240}; const TCHAR* ModeNames[]={TEXT("Fullscreen"),TEXT("Borderless"),TEXT("Windowed")};const TCHAR* QualityNames[]={TEXT("Low"),TEXT("Medium"),TEXT("High"),TEXT("Epic"),TEXT("Cinematic")};}
void ADungeonHUD::OpenSettings()
{
    CaptureBinding=AudioDragging=-1;SettingsNotice.Empty();
    auto* U=GEngine->GetGameUserSettings();if(!U)return;
    DisplaySizes.Empty();FScreenResolutionArray Res;
    if(RHIGetAvailableResolutions(Res,true))for(auto R:Res)if(R.Width>=1024&&R.Height>=720)DisplaySizes.AddUnique(FIntPoint(R.Width,R.Height));
    if(DisplaySizes.IsEmpty()){DisplaySizes.Add({1280,720});DisplaySizes.Add({1600,900});DisplaySizes.Add({1920,1080});}
    DisplaySizes.AddUnique(U->GetScreenResolution());DisplaySizes.Sort([](FIntPoint A,FIntPoint B){return A.X==B.X?A.Y<B.Y:A.X<B.X;});
    DraftResolution=DisplaySizes.IndexOfByKey(U->GetScreenResolution());DraftMode=int(U->GetFullscreenMode());DraftVSync=U->IsVSyncEnabled();DraftQuality=FMath::Clamp(U->GetOverallScalabilityLevel(),0,4);
    DraftFPS=0;for(int I=0;I<UE_ARRAY_COUNT(FPSValues);++I)if(FMath::IsNearlyEqual(U->GetFrameRateLimit(),FPSValues[I]))DraftFPS=I;
}
void ADungeonHUD::RevertDisplay()
{
    if(DisplayDeadline<=0)return;
    auto* U=GEngine->GetGameUserSettings();U->SetScreenResolution(OldResolution);U->SetFullscreenMode(EWindowMode::Type(OldMode));U->SetVSyncEnabled(OldVSync);U->SetFrameRateLimit(OldFPS);if(OldScalability)U->ScalabilityQuality=*OldScalability;else U->SetOverallScalabilityLevel(OldQuality);U->ApplySettings(false);U->ConfirmVideoMode();U->SaveSettings();DisplayDeadline=0;OpenSettings();SettingsNotice=TEXT("Display changes reverted.");
}
void ADungeonHUD::CloseSettings(){RevertDisplay();CaptureBinding=-1;AudioDragging=-1;if(auto* G=Cast<ADungeonGameMode>(UGameplayStatics::GetGameMode(this)))G->SaveAudioSettings();}
void ADungeonHUD::SettingsTick()
{
    if(DisplayDeadline>0&&FPlatformTime::Seconds()>=DisplayDeadline)RevertDisplay();
    auto* G=Cast<ADungeonGameMode>(UGameplayStatics::GetGameMode(this));auto* PC=GetOwningPlayerController();
    if(!G||!PC||!G->bShowSettings||CaptureBinding<0||FPlatformTime::Seconds()<CaptureReady)return;
    if(PC->WasInputKeyJustPressed(EKeys::Escape)){CaptureBinding=-1;SettingsNotice=TEXT("Binding cancelled.");return;}
    TArray<FKey> Keys;EKeys::GetAllKeys(Keys);
    // AnyKey is a synthetic aggregate: it reports true for every physical press.
    // Never let it (or analog mouse motion) consume the real binding candidate.
    for(auto K:Keys)if(K!=EKeys::AnyKey&&!K.IsAnalog()&&PC->WasInputKeyJustPressed(K)){
        if(DungeonKeys::Set(CaptureBinding,K,SettingsNotice)){CaptureBinding=-1;if(auto* H=Cast<ADungeonHero>(PC->GetPawn()))H->PendingRebind=true;}
        return;
    }
}

void ADungeonHUD::DrawSettings(ADungeonGameMode* G)
{
    const FLinearColor Gold(.94f,.69f,.3f),Pale(.92f,.94f,.89f);
    if(DisplaySizes.IsEmpty())OpenSettings();
    auto* PC=GetOwningPlayerController();float MX=0,MY=0;PC->GetMousePosition(MX,MY);const FVector2D M=(FVector2D(MX,MY)-Offset)/Scale;
    auto In=[&](float X,float Y,float W,float H){return M.X>=X&&M.X<=X+W&&M.Y>=Y&&M.Y<=Y+H;};
    Box(90,305,1100,455,FLinearColor(.006f,.012f,.012f,.96f));Box(110,320,1060,1,Gold);Box(110,743,1060,1,Gold);
    CardText(TEXT("SETTINGS"),130,335,Gold,26,280);
    auto Button=[&](FString Text,float X,float Y,float W,bool Selected=false){const bool Hover=In(X,Y,W,38);Box(X,Y,W,38,FLinearColor(.018f,.055f,.045f,Hover||Selected?.95f:.65f));Box(X,Y+37,W,1,Hover||Selected?Gold:FLinearColor(.25f,.22f,.13f));CardText(Text,X+8,Y+9,Hover||Selected?Gold:Pale,17,W-16,27,true);};
    const TCHAR* Tabs[]={TEXT("AUDIO"),TEXT("DISPLAY"),TEXT("CONTROLS")};for(int I=0;I<3;++I)Button(Tabs[I],540+I*205,330,190,SettingsTab==I);
    if(SettingsTab==0){
        const TCHAR* Names[]={TEXT("Master volume"),TEXT("Music"),TEXT("Sound effects"),TEXT("Voices"),TEXT("Interface")};
        float Values[]={G->GetMasterVolume(),G->MusicVolume,G->EffectsVolume,G->VoiceVolume,G->InterfaceVolume};
        if(AudioDragging>=0){
            const float V=FMath::Clamp(float((M.X-425)/430),0.f,1.f);
            if(PC->IsInputKeyDown(EKeys::LeftMouseButton)){
                switch(AudioDragging){case 0:G->SetMasterVolume(V,false);break;case 1:G->MusicVolume=V;break;case 2:G->EffectsVolume=V;break;case 3:G->VoiceVolume=V;break;case 4:G->InterfaceVolume=V;break;}
                G->RefreshMusicVolume();G->UpdateAudio();
            }else {G->SaveAudioSettings();G->PlaySound(TEXT("UI"),.5f);AudioDragging=-1;}
        }
        for(int I=0;I<5;++I){float Y=403+I*48;CardText(Names[I],155,Y,Pale,19,250);Box(425,Y+12,430,5,FLinearColor(.1f,.12f,.11f));Box(425,Y+12,430*Values[I],5,Gold);Sprite(TEXT("AudioThumb"),415+430*Values[I],Y+1,20,25);CardText(FString::Printf(TEXT("%d%%"),FMath::RoundToInt(100*Values[I])),885,Y,Gold,18,85);}
        Button(G->IsMusicMuted()?TEXT("MUSIC: MUTED"):TEXT("MUSIC: ON"),980,416,175);Button(G->AreEffectsMuted()?TEXT("SFX: MUTED"):TEXT("SFX: ON"),980,464,175);
        CardText(TEXT("Music keeps its subtle gameplay mix. Voices and interface have independent levels."),155,650,Pale,15,930);
    }else if(SettingsTab==1){
        const auto R=DisplaySizes[DraftResolution];
        const FString Values[]={ModeNames[DraftMode],DraftMode==1?TEXT("Desktop resolution"):FString::Printf(TEXT("%d x %d"),R.X,R.Y),DraftVSync?TEXT("On"):TEXT("Off"),DraftFPS==0?TEXT("Unlimited"):FString::Printf(TEXT("%d FPS"),int(FPSValues[DraftFPS])),QualityNames[DraftQuality]};
        const TCHAR* Names[]={TEXT("Window mode"),TEXT("Resolution"),TEXT("Vertical sync"),TEXT("Frame-rate limit"),TEXT("Graphics preset")};
        for(int I=0;I<5;++I){float Y=395+I*49;CardText(Names[I],175,Y+9,Pale,19,310);Button(TEXT("<"),525,Y,42);CardText(Values[I],590,Y+9,Gold,19,340,30,true);Button(TEXT(">"),955,Y,42);}
        CardText(TEXT("Borderless uses the desktop resolution. Pixel-art HUD and sprites remain crisp."),175,650,Pale,15,875);
        Button(TEXT("APPLY DISPLAY"),770,693,220);
    }else{
        for(int I=0;I<DungeonKeys::Count;++I){const int Col=I/8,Row=I%8;const float X=145+Col*520,Y=388+Row*36;CardText(DungeonKeys::Name(I),X,Y+7,Pale,17,245);Button(CaptureBinding==I?TEXT("PRESS KEY..."):DungeonKeys::Label(I),X+260,Y,205,CaptureBinding==I);}
        CardText(TEXT("Click a binding, then press a key or mouse button. Escape cancels. Enter / Esc stay reserved for menus."),145,681,Pale,13,970);
    }
    CardText(SettingsNotice,150,369,Gold,13,980,18);
    Button(TEXT("RESET TAB"),520,693,190);Button(TEXT("BACK"),1010,693,145);
    if(DisplayDeadline>0){
        Box(90,305,1100,455,FLinearColor(0,0,0,.94f));CardText(TEXT("KEEP THESE DISPLAY SETTINGS?"),310,435,Gold,27,660,45,true);
        CardText(FString::Printf(TEXT("Reverting in %d seconds"),FMath::CeilToInt(DisplayDeadline-FPlatformTime::Seconds())),370,491,Pale,19,540,35,true);
        Button(TEXT("KEEP"),390,553,220);Button(TEXT("REVERT"),670,553,220);
    }
}

void ADungeonHUD::SettingsClick(ADungeonGameMode* G,FVector2D P)
{
    auto In=[&](float X,float Y,float W,float H){return P.X>=X&&P.X<X+W&&P.Y>=Y&&P.Y<Y+H;};
    if(DisplayDeadline>0){if(In(390,553,220,38)){auto* U=GEngine->GetGameUserSettings();U->ConfirmVideoMode();U->SaveSettings();DisplayDeadline=0;SettingsNotice=TEXT("Display settings saved.");}else if(In(670,553,220,38))RevertDisplay();return;}
    if(CaptureBinding>=0)return;
    for(int I=0;I<3;++I)if(In(540+205*I,330,190,38)){G->SaveAudioSettings();AudioDragging=-1;SettingsTab=I;SettingsNotice.Empty();return;}
    if(In(1010,693,145,38)){CloseSettings();G->bShowSettings=false;return;}
    if(In(520,693,190,38)){
        if(SettingsTab==0){G->MusicVolume=G->EffectsVolume=G->VoiceVolume=G->InterfaceVolume=1;G->SetMasterVolume(1);if(G->IsMusicMuted())G->ToggleMusic();if(G->AreEffectsMuted())G->ToggleEffects();G->SaveAudioSettings();}
        else if(SettingsTab==1){DraftMode=1;DraftVSync=true;DraftFPS=2;DraftQuality=2;}
        else{DungeonKeys::Reset();if(auto* H=Cast<ADungeonHero>(GetOwningPlayerController()->GetPawn()))H->PendingRebind=true;}
        SettingsNotice=SettingsTab==1?TEXT("Display defaults selected. Apply to preview."):TEXT("Defaults restored.");return;
    }
    if(SettingsTab==0){for(int I=0;I<5;++I)if(In(405,394+I*48,470,36)){AudioDragging=I;return;}if(In(980,416,175,38))G->ToggleMusic();if(In(980,464,175,38))G->ToggleEffects();}
    else if(SettingsTab==1){
        for(int I=0;I<5;++I){int D=In(525,395+I*49,42,38)?-1:In(955,395+I*49,42,38)?1:0;if(!D)continue;switch(I){case 0:DraftMode=(DraftMode+D+3)%3;break;case 1:if(DraftMode!=1)DraftResolution=(DraftResolution+D+DisplaySizes.Num())%DisplaySizes.Num();break;case 2:DraftVSync=!DraftVSync;break;case 3:DraftFPS=(DraftFPS+D+UE_ARRAY_COUNT(FPSValues))%UE_ARRAY_COUNT(FPSValues);break;case 4:DraftQuality=(DraftQuality+D+5)%5;break;}}
        if(In(770,693,220,38)){
            auto* U=GEngine->GetGameUserSettings();OldResolution=U->GetScreenResolution();OldMode=int(U->GetFullscreenMode());OldVSync=U->IsVSyncEnabled();OldFPS=U->GetFrameRateLimit();OldQuality=U->GetOverallScalabilityLevel();OldScalability=MakeShared<Scalability::FQualityLevels>(U->ScalabilityQuality);
            U->SetScreenResolution(DisplaySizes[DraftResolution]);U->SetFullscreenMode(EWindowMode::Type(DraftMode));U->SetVSyncEnabled(DraftVSync);U->SetFrameRateLimit(FPSValues[DraftFPS]);U->SetOverallScalabilityLevel(DraftQuality);
            DisplayDeadline=FPlatformTime::Seconds()+15;U->ApplyResolutionSettings(false);U->ApplyNonResolutionSettings();
        }
    }else for(int I=0;I<DungeonKeys::Count;++I)if(In(405+(I/8)*520,388+(I%8)*36,205,35)){CaptureBinding=I;CaptureReady=FPlatformTime::Seconds()+.25;SettingsNotice=TEXT("Press the new binding. Escape cancels.");return;}
}
