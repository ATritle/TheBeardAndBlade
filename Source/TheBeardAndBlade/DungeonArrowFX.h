#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonArrowFX.generated.h"
class UNiagaraComponent;
class UNiagaraSystem;
class UMaterialInterface;
class UTextureRenderTarget2D;
class USceneCaptureComponent2D;

// Niagara lives in a private orthographic effects plane, composited over the canvas world.
UCLASS()
class ADungeonArrowFX : public AActor
{
    GENERATED_BODY()
public:
    ADungeonArrowFX();
    static ADungeonArrowFX* Find(UWorld* World,bool Create=false,bool Background=false);
    static bool BuildAssets();
    void Emit(FVector2D Point,FVector2D Direction,int Element,bool Power,int Phase);
    void EmitStyled(FVector2D Point,FVector2D Direction,FLinearColor Tint,float Size,float Lifetime,int Phase,float Power=1);
    virtual void Tick(float Dt) override;
    UTextureRenderTarget2D* Capture();
private:
    UPROPERTY() USceneCaptureComponent2D* Camera;
    UPROPERTY() UTextureRenderTarget2D* Target;
    UPROPERTY() UNiagaraSystem* System;
    UPROPERTY() UMaterialInterface* Material;
    UPROPERTY() TArray<UNiagaraComponent*> Particles;
    UPROPERTY() TArray<class UMaterialInstanceDynamic*> Materials;
    TArray<float> Ages;
    TArray<float> Lifetimes;
    bool bBackground=false;
    float SkillClock=0,WindClock=0;
};
