#include "DungeonActors.h"
#include "Engine/Texture2D.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "CanvasItem.h"
#include "GameFramework/PlayerController.h"

// Cosmetic-only: never advance the gameplay random stream for particles.
void ADungeonHUD::SoftEllipse(FVector2D Center,FVector2D Radius,FLinearColor Color)
{
    if(Color.A<=0)return;
    if(!PolishSoftTexture)
    {
        PolishSoftTexture=UTexture2D::CreateTransient(64,64,PF_B8G8R8A8);
        if(!PolishSoftTexture)return;
        PolishSoftTexture->Filter=TF_Bilinear;
        auto& Mip=PolishSoftTexture->GetPlatformData()->Mips[0];
        auto* Pixels=static_cast<FColor*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
        for(int Y=0;Y<64;++Y)for(int X=0;X<64;++X)
        {
            const float R2=FMath::Square((X-31.5f)/31.5f)+FMath::Square((Y-31.5f)/31.5f);
            Pixels[Y*64+X]=FColor(255,255,255,uint8(255*FMath::Square(FMath::Max(0.f,1-R2))));
        }
        Mip.BulkData.Unlock();PolishSoftTexture->UpdateResource();
    }
    DrawTexture(PolishSoftTexture,Offset.X+(Center.X-Radius.X)*Scale,Offset.Y+(Center.Y-Radius.Y)*Scale,
        Radius.X*2*Scale,Radius.Y*2*Scale,0,0,1,1,Color,BLEND_Translucent);
}

void ADungeonHUD::DrawSwordTrail(ADungeonHero* H,FVector2D Hand,float Angle,float Length,float Opacity)
{
    if(H->Equipment[0].IsEmpty()||!H->IsAttacking())return;
    const float T=H->GetAttackProgress();
    const bool Combo=H->IsComboSwing();
    const float Begin=Combo?.52f:.25f,EndTime=Combo?.84f:.8f;
    if(T<=Begin||T>=EndTime)return;
    const float End=FMath::DegreesToRadians(Angle-90);
    const float S=Length/65.f;
    const float Fade=FMath::Min(1.f,FMath::Min((T-Begin)*20,(EndTime-T)*12))*Opacity;
    for(int I=0;I<24;++I)
    {
        const float Weight=1-I/24.f;
        const float A=End-I*(Combo?.068f:.035f),B=A-(Combo?.075f:.04f);
        const auto V=Hand+FVector2D(FMath::Cos(A),FMath::Sin(A))*Length;
        const auto W=Hand+FVector2D(FMath::Cos(B),FMath::Sin(B))*Length;
        DrawLine(Offset.X+V.X*Scale,Offset.Y+V.Y*Scale,Offset.X+W.X*Scale,Offset.Y+W.Y*Scale,
            FLinearColor(1,Combo?.8f:.65f,.2f,.22f*Weight*Fade),(2+Weight*(Combo?11:7))*S*Scale);
        DrawLine(Offset.X+V.X*Scale,Offset.Y+V.Y*Scale,Offset.X+W.X*Scale,Offset.Y+W.Y*Scale,
            FLinearColor(1,.94f,.73f,.8f*Weight*Fade),(.6f+Weight*2)*S*Scale);
    }
}

void ADungeonHUD::DrawHitFeedback(ADungeonGameMode* G)
{
    if(auto* PC=GetOwningPlayerController())if(auto* H=Cast<ADungeonHero>(PC->GetPawn());H&&H->ComboPopupTime>0){
        const float Age=1-H->ComboPopupTime;
        const float Pop=Age<.12f?.65f+Age/.12f*.5f:1.f+.15f*FMath::Max(0.f,1-(Age-.12f)/.18f);
        const float Alpha=FMath::Clamp(H->ComboPopupTime/.35f,0.f,1.f);
        const auto P=H->ComboPopupPosition-FVector2D(0,155+Age*45);
        Sprite(TEXT("Combat_Combo"),P.X-88*Pop,P.Y-44*Pop,176*Pop,88*Pop,FLinearColor(1,1,1,Alpha));
    }
    for(const auto& I:G->GetImpacts())
    {
        const float Age=FMath::Max(0.f,1.05f-I.Life);
        const bool Heal=I.Damage<0;
        if(!Heal&&Age<.28f)
        {
            const float Fade=1-Age/.28f;
            const auto Origin=I.Position-FVector2D(0,I.Damage>0?38:0);
            SoftEllipse(Origin,FVector2D(22,17),FLinearColor(1,.76f,.3f,.32f*FMath::Max(0.f,1-Age/.1f)));
            for(int J=0;J<9;++J)
            {
                const float Angle=J*2*PI/9+I.Position.X*.013f;
                const FVector2D Direction(FMath::Cos(Angle),FMath::Sin(Angle));
                const auto P=Origin+Direction*(5+Age*(105+J*11))+FVector2D(0,Age*Age*160);
                const auto Q=P-Direction*(3+Fade*7);
                DrawLine(Offset.X+P.X*Scale,Offset.Y+P.Y*Scale,Offset.X+Q.X*Scale,Offset.Y+Q.Y*Scale,
                    FLinearColor(1,.65f,.18f,Fade),2.5f*Scale);
                DrawLine(Offset.X+P.X*Scale,Offset.Y+P.Y*Scale,Offset.X+Q.X*Scale,Offset.Y+Q.Y*Scale,
                    FLinearColor(1,.96f,.78f,Fade),Scale);
            }
        }
        if(I.Damage==0){Ring(I.Position,30+Age*150,FLinearColor(1,.65f,.2f,FMath::Max(0.f,1-Age/.65f)),3);continue;}
        FLinearColor C=Heal?FLinearColor(.4f,1,.62f):I.bBoss?FLinearColor(1,.32f,.22f):FLinearColor(1,.92f,.68f);
        C.A=FMath::Clamp(I.Life/.4f,0.f,1.f);
        const float Amount=FMath::Abs(I.Damage);
        const FString Number=(Heal?FString(TEXT("+")):FString())+(Amount<1?FString::Printf(TEXT("%.1f"),Amount):FString::Printf(TEXT("%.0f"),Amount));
        const float X=I.Position.X+(FMath::Abs(FMath::RoundToInt(I.Position.X+I.Position.Y))%3-1)*14;
        const float Y=I.Position.Y-72-Age*48;
        FCanvasTextItem Text(Offset+FVector2D(X,Y)*Scale,FText::FromString(Number),GEngine->GetMediumFont(),C);
        Text.bCentreX=true;Text.bOutlined=true;Text.OutlineColor=FLinearColor(0,0,0,C.A*.9f);
        Text.Scale=FVector2D(Scale*(1.05f+.2f*FMath::Max(0.f,1-Age/.15f)));
        Canvas->DrawItem(Text);
    }
}

void ADungeonHUD::DrawPolishAtmosphere(ADungeonGameMode* G,ADungeonHero* H)
{
    const float Dt=G->IsGameplayBlocked()||H->IsInventoryOpen()||G->IsAtlasTravel()?0:FMath::Min(GetWorld()->GetDeltaSeconds(),.05f);
    AtmosphereClock+=Dt;
    const auto P=DungeonView::Project(H->GetActorLocation());
    const float Distance=FVector2D::Distance(P,LastDustPosition);
    if(AtmosphereRoom!=G->GetRoom()||Distance>150){MovementDust.Reset();DustDistance=0;AtmosphereRoom=G->GetRoom();}
    else if(Dt>0&&H->IsWalking())
    {
        DustDistance+=Distance;
        if(DustDistance>=20){FMovementDust D;D.Position=P;MovementDust.Add(D);DustDistance=FMath::Fmod(DustDistance,20.f);}
    }
    LastDustPosition=P;
    for(auto& D:MovementDust)
    {
        D.Age+=Dt;
        const float Fade=FMath::Max(0.f,1-D.Age/.55f);
        SoftEllipse(D.Position+FVector2D(D.Age*10,-D.Age*8),FVector2D(6+D.Age*22,3+D.Age*9),FLinearColor(.65f,.59f,.47f,.18f*Fade));
    }
    MovementDust.RemoveAll([](const FMovementDust& D){return D.Age>=.55f;});
    if(!G->IsAtlasFloor())return;
    const FVector2D Lights[]={{469,53},{813,53},{44,266},{1236,266},{44,497},{1236,497},{469,715},{813,715}};
    const bool Bunker=G->GetBiome()==5;
    const bool Cold=G->GetBiome()==2||G->GetBiome()==4;
    for(int I=0;I<8;++I)
    {
        const float T=AtmosphereClock+I*1.71f;
        const float Flicker=1+.09f*FMath::Sin(T*7)+.06f*FMath::Sin(T*13.7f);
        const FLinearColor Warm=Bunker?FLinearColor(.45f,1,.65f,.08f*Flicker):Cold?FLinearColor(.2f,.65f,1,.12f*Flicker):FLinearColor(1,.52f,.12f,.16f*Flicker);
        SoftEllipse(Lights[I],FVector2D(51,65)*Flicker,Warm);
        if(!Bunker&&!Cold&&G->GetBiome()!=7)for(int J=0;J<3;++J)
        {
            const float A=FMath::Fmod(T*.62f+J/3.f,1.f);
            const auto Ember=Lights[I]+FVector2D(FMath::Sin(T*2+J)*8,-A*42);
            SoftEllipse(Ember,FVector2D(2,3),FLinearColor(1,.55f,.15f,(1-A)*.65f));
            SoftEllipse(Lights[I]+FVector2D(FMath::Sin(T+J)*9,-8-A*43),FVector2D(5+A*8,9+A*9),FLinearColor(.3f,.31f,.32f,.045f*FMath::Sin(A*PI)));
        }
    }
    // Thin moving haze at the perimeter, underneath characters and attack telegraphs.
    for(int I=0;I<6;++I)
    {
        const float T=AtmosphereClock*.15f+I*2.4f;
        const FVector2D Center(200+I%3*420+FMath::Sin(T)*58,I<3?190:646);
        SoftEllipse(Center,FVector2D(220,30+FMath::Sin(T)*6),FLinearColor(.65f,.73f,.76f,.065f));
    }
}
