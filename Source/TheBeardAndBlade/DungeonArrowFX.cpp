#include "DungeonArrowFX.h"
#include "DungeonActors.h"
#include "DungeonBow.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#if WITH_EDITOR
#include "NiagaraSpriteRendererProperties.h"
#include "Stateless/NiagaraStatelessEmitter.h"
#include "Stateless/Modules/NiagaraStatelessModule_InitializeParticle.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionParticleRelativeTime.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#endif

ADungeonArrowFX::ADungeonArrowFX()
{
    PrimaryActorTick.bCanEverTick=true;
    Camera=CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("ArrowCapture"));RootComponent=Camera;
    Camera->ProjectionType=ECameraProjectionMode::Orthographic;Camera->OrthoWidth=1280;
    Camera->CaptureSource=ESceneCaptureSource::SCS_SceneColorHDR;
    Camera->bCaptureEveryFrame=false;Camera->bCaptureOnMovement=false;
    Camera->PrimitiveRenderMode=ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
    Camera->ShowFlags.SetAtmosphere(false);Camera->ShowFlags.SetFog(false);
    Camera->ShowFlags.SetPostProcessing(false);Camera->ShowFlags.SetLighting(false);
    Camera->ShowFlags.SetMotionBlur(false);
}
ADungeonArrowFX* ADungeonArrowFX::Find(UWorld* World,bool Create,bool Background)
{
    for(TActorIterator<ADungeonArrowFX> I(World);I;++I)if(I->bBackground==Background)return *I;
    if(!Create||!FApp::CanEverRender())return nullptr;
    auto* FX=World->SpawnActor<ADungeonArrowFX>();
    FX->bBackground=Background;
    FX->SetActorLocationAndRotation(FVector(-400,640,1000),FRotator(-90,0,0));
    FX->System=LoadObject<UNiagaraSystem>(nullptr,TEXT("/Game/Effects/Arrows/NS_ArrowWisp.NS_ArrowWisp"));
    FX->Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Effects/Arrows/M_ArrowWisp.M_ArrowWisp"));
    FX->Target=NewObject<UTextureRenderTarget2D>(FX);FX->Target->ClearColor=FLinearColor::Black;
    FX->Target->InitCustomFormat(1280,800,PF_FloatRGBA,false);FX->Target->UpdateResourceImmediate(true);
    FX->Camera->TextureTarget=FX->Target;
    UE_LOG(LogTemp,Display,TEXT("ARROW_NIAGARA: system=%d material=%d"),FX->System!=nullptr,FX->Material!=nullptr);
    return FX;
}
void ADungeonArrowFX::Emit(FVector2D P,FVector2D D,int Element,bool Power,int Phase)
{
    EmitStyled(P,D,Element?DungeonBow::Color(Element):FLinearColor(.75f,.53f,.24f),Phase==2?110:Phase==1?57:88,.55f,Phase,Power?1.35f:1.f);
}
void ADungeonArrowFX::EmitStyled(FVector2D P,FVector2D D,FLinearColor Tint,float Size,float Lifetime,int Phase,float Power)
{
    if(!System||!Material||Particles.Num()>=160)return;
    auto* C=NewObject<UNiagaraComponent>(this);C->bAutoActivate=false;C->SetAsset(System);
    C->SetAutoDestroy(false);C->RegisterComponent();C->SetWorldLocation(FVector(-P.Y,P.X,0));
    auto* M=UMaterialInstanceDynamic::Create(Material,C);
    M->SetVectorParameterValue(TEXT("Tint"),Tint);
    M->SetScalarParameterValue(TEXT("Angle"),FMath::Atan2(D.Y,D.X));
    M->SetScalarParameterValue(TEXT("Phase"),Phase);
    M->SetScalarParameterValue(TEXT("Power"),Power);
    C->SetVariableMaterial(TEXT("User.Material"),M);
    C->SetWorldScale3D(FVector(Size/88));
    Camera->ShowOnlyComponent(C);C->Activate(true);Particles.Add(C);Materials.Add(M);Ages.Add(0);Lifetimes.Add(FMath::Clamp(Lifetime,.1f,7.f));
}
void ADungeonArrowFX::Tick(float Dt)
{
    Super::Tick(Dt);
    auto* G=Cast<ADungeonGameMode>(UGameplayStatics::GetGameMode(this));
    const bool Clear=!G||G->IsMenu()||G->IsTraderOpen()||G->IsAtlasTravel()||G->IsAtlasScrolling();
    // Inventory pauses the same lifetime clock as the gameplay simulation.
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));
    const bool Pause=!Clear&&G&&(G->IsGameplayBlocked()||(H&&H->IsInventoryOpen()));
    if(!Clear&&!Pause&&!bBackground&&H){
        SkillClock+=Dt;WindClock+=Dt;
        if(SkillClock>=.55f){
            SkillClock=0;
            if(H->IsTeaEmpowered()){
                const auto P=DungeonView::Project(H->GetActorLocation())-FVector2D(0,48);
                const float Life=FMath::Min(.85f,H->GetTeaSpiritTime());
                if(Life>.1f){EmitStyled(P,{1,0},{1,.65f,.15f},175,Life,8);EmitStyled(P,{1,0},{1,.78f,.34f},120,Life,9,.35f);}
            }
        }
        if(WindClock>=.07f){
            WindClock=0;
            if(G->IsFreedomActive()){
                const float T=G->FreedomProgress();const FVector2D P(FMath::Lerp(-220.f,1500.f,T),280+FMath::Sin(T*PI*2)*55);
                for(int I=-1;I<=1;++I)EmitStyled(P+FVector2D(-75,I*45),{1,0},{.85f,.77f,.48f},210,.65f,0,.8f);
            }
            if(H->IsRolling())EmitStyled(DungeonView::Project(H->GetActorLocation()),{1,0},{.45f,.4f,.3f},90,.45f,6,2);
        }
    }
    for(int I=Particles.Num()-1;I>=0;--I){
        Particles[I]->SetPaused(Pause);
        if(!Pause)Ages[I]+=Dt;
        Materials[I]->SetScalarParameterValue(TEXT("Age"),Ages[I]/Lifetimes[I]);
        if(Clear||Ages[I]>Lifetimes[I]+.07f){Camera->RemoveShowOnlyComponent(Particles[I]);Particles[I]->DestroyComponent();Particles.RemoveAtSwap(I);Materials.RemoveAtSwap(I);Ages.RemoveAtSwap(I);Lifetimes.RemoveAtSwap(I);}
    }
}
UTextureRenderTarget2D* ADungeonArrowFX::Capture()
{
    if(!Particles.Num())return nullptr;
    Camera->CaptureScene();return Target;
}

bool ADungeonArrowFX::BuildAssets()
{
#if WITH_EDITOR
    auto Save=[](UObject* O){auto* P=O->GetOutermost();P->MarkPackageDirty();FSavePackageArgs A;A.TopLevelFlags=RF_Public|RF_Standalone;
        return UPackage::SavePackage(P,O,*FPackageName::LongPackageNameToFilename(P->GetName(),FPackageName::GetAssetPackageExtension()),A);};
    auto* MP=CreatePackage(TEXT("/Game/Effects/Arrows/M_ArrowWisp"));
    MP->FullyLoad();
    auto* M=NewObject<UMaterial>(MP,TEXT("M_ArrowWisp"),RF_Public|RF_Standalone);
    M->BlendMode=BLEND_Additive;M->SetShadingModel(MSM_Unlit);M->TwoSided=true;
    M->SetUsageByFlag(MATUSAGE_NiagaraSprites,true);
    auto* UV=NewObject<UMaterialExpressionTextureCoordinate>(M);
    auto Scalar=[&](const TCHAR* N,float V){auto* E=NewObject<UMaterialExpressionScalarParameter>(M);E->ParameterName=N;E->DefaultValue=V;M->GetExpressionCollection().AddExpression(E);return E;};
    auto* Angle=Scalar(TEXT("Angle"),0);auto* Phase=Scalar(TEXT("Phase"),0);auto* Power=Scalar(TEXT("Power"),1);
    auto* Age=Scalar(TEXT("Age"),0);
    auto* Tint=NewObject<UMaterialExpressionVectorParameter>(M);Tint->ParameterName=TEXT("Tint");Tint->DefaultValue=FLinearColor(.2f,.6f,1);
    auto* Code=NewObject<UMaterialExpressionCustom>(M);Code->OutputType=CMOT_Float3;Code->Inputs.Reset();
    auto Input=[&](const TCHAR* N,UMaterialExpression* E){FCustomInput I;I.InputName=N;I.Input.Expression=E;Code->Inputs.Add(I);};
    Input(TEXT("UV"),UV);Input(TEXT("Age"),Age);Input(TEXT("Angle"),Angle);Input(TEXT("Phase"),Phase);Input(TEXT("Power"),Power);Input(TEXT("Tint"),Tint);
    Code->Code=TEXT(R"(
float2 p=(UV-.5)*2;float c=cos(Angle),s=sin(Angle);p=float2(c*p.x+s*p.y,-s*p.x+c*p.y);
float fade=pow(saturate(1-Age),2.1), light=0;
if(Phase<.5){
 float envelope=pow(saturate(1-abs(p.x)),1.2);
 float bend=sin(p.x*3.2+Age*2.8)*(.035+Age*.10);
 float width=(.014+Age*.025)*Power;
 float ribbon=exp(-pow((p.y-bend-(.018+Age*.14))/width,2));
 ribbon+=exp(-pow((p.y+bend+(.018+Age*.14))/(width*.7),2))*.65;
 float haze=exp(-pow((p.y-bend)/(.055+Age*.20),2))*.13;
 light=(ribbon*.50+haze)*envelope*fade;
}else if(Phase<2.5){
 float r=length(p),a=atan2(p.y,p.x);
 float radius=.035+Age*(Phase>1.5?.78:.5);
 float ring=exp(-pow((r-radius)/(.024+Age*.016),2))*.24;
 float rays=pow(saturate(cos(a*7+Age*2)),18)*exp(-pow((r-radius*.65)/.16,2))*.65;
 float core=exp(-r*r/(.004+Age*.012))*pow(saturate(1-Age*4),2)*1.5;
 light=(ring+rays+core)*fade*Power;
}else if(Phase<3.5){
 float r=length(p),a=atan2(p.y,p.x);
 float sector=pow(saturate(1-abs(a+Age*.45)/1.3),1.7);
 float edge=exp(-pow((r-(.70+Age*.12))/.022,2));
 float veil=exp(-pow((r-.65)/.10,2))*.20;
 light=(edge+veil)*sector*fade*.6*Power;
}else if(Phase<4.5){
 float r=length(p),a=atan2(p.y,p.x);
 float arc=exp(-pow((r-(.5+Age*.3))/.025,2))*pow(saturate(cos(a)),3);
 float spokes=pow(saturate(cos(a*9)),26)*exp(-pow((r-(.1+Age*.8))/.10,2));
 light=(arc*.65+spokes)*fade*Power;
}else if(Phase<5.5){
 float y=p.y+.3, sway=sin(y*13-Age*14)*.035+sin(y*23+Age*19)*.018;
 float width=.05+.12*saturate(y+.45);
 float fire=exp(-pow((p.x-sway)/width,2))*exp(-pow((y+.05)/.39,2));
 float embers=0;
 for(int i=0;i<3;i++){float t=frac(Age+i*.31);float2 e=float2(sin(t*8+i*2)*.17,.3-t*1.1);embers+=exp(-dot(p-e,p-e)/.0006)*(1-t);}
 light=(fire*.65+embers)*sin(saturate(Age)*3.14159)*Power;
}else if(Phase<6.5){
 float2 q=p-float2(Age*.18-.09,0);
 float bands=.5+.25*sin(q.x*14+q.y*31+Age*5)+.25*sin(q.x*23-q.y*17-Age*3);
 float mask=exp(-pow(q.x/.7,4)-pow(q.y/.15,2));
 light=mask*bands*sin(saturate(Age)*3.14159)*.055*Power;
}else if(Phase<7.5){
 // Ballistic liquid jets and ceramic shards. No residual puddle or steam.
 float drops=0,shards=0;
 for(int i=0;i<15;i++){
  float a=i*2.39996, speed=.48+frac(i*.371)*.42;
  float2 v=float2(cos(a)*speed,sin(a)*speed*.46-.52);
  float2 q=v*Age+float2(0,Age*Age*.65);
  float2 d=p-q; float w=.017+frac(i*.617)*.014;
  drops+=exp(-dot(d,d)/(w*w))*(1-Age*.6);
  float2 tail=p-(q-v*.06); drops+=exp(-dot(tail,tail)/(w*w*.7))*.45;
  float2 axis=v*min(Age,.18);float u=saturate(dot(p-q,axis)/max(dot(axis,axis),.0001));
  float2 jet=p-q-axis*u;drops+=exp(-dot(jet,jet)/(w*w*.4))*saturate(1-Age*1.6)*.4;
  if(i<6){float2 z=d*float2(1,1.6);shards+=saturate(1-length(z)/.026);}
 }
 float crown=exp(-pow((length(p*float2(1,1.8))-(.08+Age*.42))/.025,2))*saturate(1-Age*2);
 return (Tint*(drops*.8+crown*.3)+float3(1,.9,.72)*shards)*saturate((1-Age)*5)*Power;
}else if(Phase<8.5){
 float r=length(p*float2(1,.84));
 float wave=exp(-pow((r-(.12+Age*.7))/(.045+Age*.03),2));
 float mist=exp(-pow((r-(.16+Age*.58))/.15,2))*.14;
 light=(wave*.18+mist)*sin(saturate(Age)*3.14159)*Power;
}else if(Phase<11.5){
 float bits=0;
 for(int i=0;i<12;i++){
  float a=i*2.39996, t=Age;
  float2 q=float2(cos(a)*(.13+frac(i*.31)*.48),.65-frac(i*.47)*.8-t*.65);
  if(Phase>9.5&&Phase<10.5)q=float2(cos(a),sin(a)*.6)*(.8*(1-t));
  if(Phase>10.5)q=float2(cos(a)*t*.65,sin(a)*t*.35-t*.45);
  float2 d=p-q; bits+=exp(-dot(d,d)/.0008)+exp(-dot(d,d)/.008)*.12;
 }
 light=bits*sin(saturate(Age)*3.14159)*.55*Power;
}else{
 float r=length(p),a=atan2(p.y,p.x),radius=.06+Age*.78;
 float shock=exp(-pow((r-radius)/(.022+Age*.03),2))*.25;
 float shards=pow(saturate(cos(a*11+sin(a*5)*2)),24)*exp(-pow((r-radius*.9)/.13,2));
 float smoke=exp(-pow((r-radius*.6)/.22,2))*(.5+.5*sin(a*7+Age*6))*.10;
 light=(shock+shards+smoke)*fade*Power;
 if(Phase>12.5&&Phase<13.5){ // Flame petals curl upward and cool out.
  float2 q=p+float2(0,Age*.18);float f=atan2(q.y,q.x);
  light=(shards+exp(-pow((length(q)-radius*.55)/.12,2))*pow(saturate(.5+.5*sin(f*5+Age*9)),3))*fade*Power;
 }else if(Phase>13.5&&Phase<14.5){ // Six ice splinters, no persistent ground field.
  light=(shock*.6+pow(saturate(cos(a*6)),42)*exp(-pow((r-radius*.7)/.19,2)))*fade*Power;
 }else if(Phase>14.5&&Phase<15.5){
  light=(smoke*2+shards*.4+shock*.5)*fade*Power;
 }else if(Phase>15.5&&Phase<16.5){
  float zig=.04*sin(r*65+Age*12);float bolt=pow(saturate(cos((a+zig)*5)),65)*exp(-pow((r-radius*.65)/.25,2));
  light=(bolt+shock*.45)*fade*Power;
 }else if(Phase>16.5){light=shards*fade*Power;}
}
return Tint*light;
)");
    for(auto* E:TArray<UMaterialExpression*>{UV,Tint,Code})M->GetExpressionCollection().AddExpression(E);
    M->GetEditorOnlyData()->EmissiveColor.Expression=Code;
    M->GetEditorOnlyData()->Opacity.Constant=1;M->PostEditChange();
    if(!Save(M))return false;
    auto* Template=LoadObject<UNiagaraSystem>(nullptr,TEXT("/Niagara/DefaultAssets/Templates/Systems/MinimalLightweight.MinimalLightweight"));
    if(!Template)return false;
    auto* NP=CreatePackage(TEXT("/Game/Effects/Arrows/NS_ArrowWisp"));
    NP->FullyLoad();
    auto* N=DuplicateObject<UNiagaraSystem>(Template,NP,TEXT("NS_ArrowWisp"));N->SetFlags(RF_Public|RF_Standalone);
    for(auto& Handle:N->GetEmitterHandles()){
        auto* E=Handle.GetStatelessEmitter();if(!E)return false;
        for(auto& Module:E->GetModules())Module->SetIsModuleEnabled(Module->IsA<UNiagaraStatelessModule_InitializeParticle>());
        auto* Init=Cast<UNiagaraStatelessModule_InitializeParticle>(E->GetModule(UNiagaraStatelessModule_InitializeParticle::StaticClass()));
        if(!Init)return false;
        Init->LifetimeDistribution.InitConstant(8.f);Init->SpriteSizeDistribution.InitConstant(FVector2f(88,88));
        for(int I=0;I<E->GetNumSpawnInfos();++I)E->GetSpawnInfoByIndex(I)->bEnabled=false;
        auto& Spawn=E->AddSpawnInfo();Spawn.Type=ENiagaraStatelessSpawnInfoType::Burst;Spawn.Amount=FNiagaraDistributionRangeInt(1);
        auto* StateProp=FindFProperty<FStructProperty>(E->GetClass(),TEXT("EmitterState"));
        auto* State=StateProp->ContainerPtrToValuePtr<FNiagaraEmitterStateData>(E);
        State->LoopBehavior=ENiagaraLoopBehavior::Once;State->LoopDuration=FNiagaraDistributionRangeFloat(.01f);
        for(auto* R:E->GetRenderers())if(auto* Sprite=Cast<UNiagaraSpriteRendererProperties>(R)){
            Sprite->Material=M;Sprite->MaterialUserParamBinding.Parameter=FNiagaraVariable(FNiagaraTypeDefinition(UMaterialInterface::StaticClass()),TEXT("User.Material"));
            N->GetExposedParameters().AddParameter(Sprite->MaterialUserParamBinding.Parameter);
        }
        E->PostEditChange();
    }
    N->PostEditChange();N->RequestCompile(true);N->WaitForCompilationComplete(true,false);
    const bool OK=Save(N);UE_LOG(LogTemp,Display,TEXT("ARROW_NIAGARA_BUILD: %s"),OK?TEXT("SUCCESS"):TEXT("FAILED"));return OK;
#else
    return false;
#endif
}
