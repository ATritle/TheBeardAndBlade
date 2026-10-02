#include "DungeonActors.h"
#include "DungeonCombatBalance.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Texture2D.h"
#include "Engine/Canvas.h"

bool ADungeonHero::CanDrinkPotion() const
{
    return CanStrike()&&PotionCharges>0&&Health<MaxHealth;
}
void ADungeonHero::DrinkPotion()
{
    if(!CanDrinkPotion())return;
    --PotionCharges;
    PotionSip=PotionDrinkDuration;
    RestoreHealth(MaxHealth*.25f);
    if(auto* G=Cast<ADungeonGameMode>(UGameplayStatics::GetGameMode(this)))G->PlaySound(TEXT("Equip"),.4f,1.15f);
}

void ADungeonHero::SpendStamina(float Amount)
{
    Stamina=FMath::Max(0.f,Stamina-Amount); StaminaDelay=.8f;
    if(Stamina<=KINDA_SMALL_NUMBER) { Stamina=0; bExhausted=true; }
}
void ADungeonHero::UpdateStamina(float Dt,bool Sprinting)
{
    if(Sprinting) { SpendStamina(DungeonCombatBalance::SprintCost*Dt); return; }
    const float RegenTime=FMath::Max(0.f,Dt-StaminaDelay);
    StaminaDelay=FMath::Max(0.f,StaminaDelay-Dt);
    Stamina=FMath::Min(MaxStamina,Stamina+25.f*StaminaRegen*RegenTime);
    if(Stamina>=MaxStamina) bExhausted=false;
}
void ADungeonGameMode::UpdatePotions(float Dt)
{
    auto* H=Cast<ADungeonHero>(UGameplayStatics::GetPlayerPawn(this,0));
    if(!H||H->Health<=0||H->IsInventoryOpen()||IsGameplayBlocked()) return;
    for(int32 I=Potions.Num()-1;I>=0;--I)
    {
        auto& P=Potions[I]; P.Age+=Dt;
        // Potions are carried, even at full health. Excess stays on the floor.
        if(P.Age>.4f&&H->PotionCharges<ADungeonHero::PotionCapacity&&FVector2D::Distance(P.Position,DungeonView::Project(H->GetActorLocation()))<34)
        {
            ++H->PotionCharges;
            PlaySound(TEXT("Equip"),.65f,1.3f); Potions.RemoveAt(I);
        }
    }
}
void ADungeonHUD::Orb(FVector2D C,float Fraction,FLinearColor Color)
{
    const float Size=96,X=C.X-Size*.5f,Y0=C.Y-Size*.48f,R=Size*.265f;
    Sprite(TEXT("HUD_EmptyOrb"),X,Y0,Size,Size);
    auto* Liquid=Texture(Color.R>Color.B?TEXT("HUD_HealthOrb"):TEXT("HUD_StaminaOrb"));
    if(!Liquid) return;
    const float Level=R-2*R*FMath::Clamp(Fraction,0.f,1.f);
    // Clip the detailed liquid texture, not the metal frame. Values and art share one fill level.
    for(int Y=FMath::CeilToInt(Level);Y<R;++Y)
    {
        const float Half=FMath::Sqrt(FMath::Max(0.f,R*R-Y*Y));
        DrawTexture(Liquid,Offset.X+(C.X-Half)*Scale,Offset.Y+(C.Y+Y)*Scale,Half*2*Scale,Scale,
            .5f-Half/Size,.48f+Y/Size,Half*2/Size,1.f/Size,FLinearColor::White,BLEND_Translucent);
        if(Y<Level+1&&Fraction<.99f) Box(C.X-Half,C.Y+Y,Half*2,1,Color);
    }
}
void ADungeonHUD::DrawVitals(ADungeonHero* H)
{
    auto* G=Cast<ADungeonGameMode>(UGameplayStatics::GetGameMode(this));
    if(!G)return;
    // Scale the complete HUD around a bottom-center anchor, not the game world.
    // Scoped restoration also protects later inventory/map/guard drawing.
    constexpr float CompactScale=.68f;
    TGuardValue<float> RestoreScale(Scale,Scale*CompactScale);
    TGuardValue<FVector2D> RestoreOffset(Offset,Offset+FVector2D(640,790)*(Scale/CompactScale)*(1-CompactScale));
    const FLinearColor Gold(.94f,.69f,.3f),Pale(.95f,.96f,.9f),Blue(.035f,.24f,.72f),Green(.012f,.56f,.09f),Ink(.009f,.014f,.016f,1);
    auto Disk=[&](FVector2D C,int R,FLinearColor Color){for(int Y=-R;Y<R;++Y){float Half=FMath::Sqrt(FMath::Max(0.f,float(R*R-Y*Y)));Box(C.X-Half,C.Y+Y,Half*2,1,Color);}};
    // Small code-native mouse pictograms: left, wheel, right. A basic silhouette
    // remains crisp at the reduced HUD size and needs no baked text or texture.
    auto Mouse=[&](FVector2D C,int Button){
        auto Rounded=[&](float X,float Y,float W,float Height,float R,FLinearColor Color){
            for(int Row=0;Row<FMath::CeilToInt(Height);++Row){
                const float D=Row<R?R-Row-.5f:Row>=Height-R?Row-(Height-R)+.5f:0;
                const float Inset=D>0?R-FMath::Sqrt(FMath::Max(0.f,R*R-D*D)):0;
                Box(X+Inset,Y+Row,W-2*Inset,1,Color);
            }
        };
        Rounded(C.X-12,C.Y-17,24,34,7,FLinearColor(.75f,.68f,.48f));
        Rounded(C.X-10.5f,C.Y-15.5f,21,31,5.5f,Ink);
        if(Button==0)Rounded(C.X-9,C.Y-14,8,13,2,Gold);
        if(Button==2)Rounded(C.X+1,C.Y-14,8,13,2,Gold);
        Box(C.X-.7f,C.Y-14,1.4f,15,FLinearColor(.35f,.32f,.24f));
        Box(C.X-9,C.Y,18,1,FLinearColor(.35f,.32f,.24f));
        Rounded(C.X-2,C.Y-12,4,8,1.5f,Button==1?Gold:FLinearColor(.16f,.17f,.16f));
    };
    const float Dt=GetWorld()->GetDeltaSeconds();
    const float Health=FMath::Clamp(H->Health/FMath::Max(1.f,H->MaxHealth),0.f,1.f),Stamina=FMath::Clamp(H->Stamina/FMath::Max(1.f,H->MaxStamina),0.f,1.f);
    if(DisplayHealth<0)DisplayHealth=Health;
    if(DisplayStamina<0)DisplayStamina=Stamina;
    DisplayHealth=FMath::FInterpConstantTo(DisplayHealth,Health,Dt,2.f);
    DisplayStamina=FMath::FInterpConstantTo(DisplayStamina,Stamina,Dt,3.f);

    // One uncut chassis, uniformly scaled: orb, stamina recess, six sockets and
    // overlapping keycaps all share the source artwork's coordinate system.
    constexpr float K=.43f,BaseX=140,BaseY=458;
    auto P=[](float X,float Y){return FVector2D(BaseX+X*K,BaseY+Y*K);};
    if(auto* Chassis=Texture(TEXT("Hotbar_Chassis"))){
        // Extend the original curved gold rail, using its actual hammered-metal
        // pixels rather than flat box outlines. The ends tuck behind the dragon
        // and lightning mount when the original chassis is drawn over them.
        MeterArc(P(342,502),303*K,3.35f,1.47f,1,FLinearColor(.015f,.018f,.017f),49*K);
        TArray<FCanvasUVTri> Rail;
        constexpr int RailSteps=100;
        for(int I=0;I<RailSteps;++I){
            FVector2D V[4],UV[4];
            for(int J=0;J<4;++J){
                const float T=float(I+(J>=2))/RailSteps;
                const bool Outer=J==1||J==2;
                const float A=3.35f+1.47f*T,SourceA=3.47f+1.12f*T;
                V[J]=Offset+P(342+FMath::Cos(A)*(Outer?334:314),502+FMath::Sin(A)*(Outer?334:314))*Scale;
                UV[J]=FVector2D((342+FMath::Cos(SourceA)*(Outer?303:283))/1774.f,(502+FMath::Sin(SourceA)*(Outer?303:283))/887.f);
            }
            for(int J=0;J<2;++J){
                FCanvasUVTri T;const int B=J?2:1,C=J?3:2;
                T.V0_Pos=V[0];T.V1_Pos=V[B];T.V2_Pos=V[C];
                T.V0_UV=UV[0];T.V1_UV=UV[B];T.V2_UV=UV[C];
                T.V0_Color=T.V1_Color=T.V2_Color=FLinearColor::White;Rail.Add(T);
            }
        }
        Canvas->K2_DrawTriangle(Chassis,MoveTemp(Rail));
        // Keep one shared transform, but omit the two baked mouse-key plaques.
        // A clean rail sample bridges their sockets; no opaque cover hides the world.
        auto RawRegion=[&](float X,float Y,float W,float Height,float U,float V,float UW,float VH){
            const auto Dest=P(X,Y);
            DrawTexture(Chassis,Offset.X+Dest.X*Scale,Offset.Y+Dest.Y*Scale,W*K*Scale,Height*K*Scale,
                U/1774.f,V/887.f,UW/1774.f,VH/887.f,FLinearColor::White,BLEND_Translucent);
        };
        // Exclude the baked dark backing inside the health ellipse, preserving
        // the ornamental rim and all other chassis regions without editing art.
        auto Region=[&](float X,float Y,float W,float Height,float U,float V,float UW,float VH){
            if(X>=493||X+W<=173||Y>=654||Y+Height<=344){RawRegion(X,Y,W,Height,U,V,UW,VH);return;}
            for(float Row=Y;Row<Y+Height;Row+=1){
                const float DY=Row+.5f-499;
                const float Half=160*FMath::Sqrt(FMath::Max(0.f,1-DY*DY/(155*155)));
                auto Strip=[&](float Left,float Right){if(Right>Left)RawRegion(Left,Row,Right-Left,1,U+(Left-X)*UW/W,V+(Row-Y)*VH/Height,(Right-Left)*UW/W,VH/Height);};
                if(Half<=0)Strip(X,X+W);
                else {Strip(X,FMath::Clamp(333-Half,X,X+W));Strip(FMath::Clamp(333+Half,X,X+W),X+W);}
            }
        };
        Region(0,0,1774,614,0,0,1774,614);
        Region(0,614,1352,273,0,614,1352,273);
        Region(1458,614,79,273,1458,614,79,273);
        Region(1643,614,131,273,1643,614,131,273);
        Region(1352,614,106,42,1110,614,50,42);
        Region(1537,614,106,42,1110,614,50,42);
    }
    const FVector2D OrbCenter=P(333,499);
    const float RX=160*K,RY=155*K,Level=RY*(1-2*DisplayHealth);
    auto* Liquid=Texture(TEXT("HUD_HealthOrb"));
    for(int Y=FMath::FloorToInt(-RY);Y<RY;++Y){
        const float Half=RX*FMath::Sqrt(FMath::Max(0.f,1-Y*Y/(RY*RY)));
        // No opaque backing: the live dungeon shows through depleted liquid.
        if(Liquid&&Y>=Level){
            DrawTexture(Liquid,Offset.X+(OrbCenter.X-Half)*Scale,Offset.Y+(OrbCenter.Y+Y)*Scale,Half*2*Scale,Scale,
                .5f-Half/RX*.25f,.48f+Y/RY*.25f,Half/RX*.5f,.25f/RY,FLinearColor::White,BLEND_Translucent);
            if(Y<Level+1&&DisplayHealth>.01f&&DisplayHealth<.99f)Box(OrbCenter.X-Half,OrbCenter.Y+Y,Half*2,1,FLinearColor(.7f,.11f,.09f));
        }
    }
    // The gemstone fill stays inside the chassis' recessed gold rails. Its ends
    // stop under the dragon/medallion mounts, never across the ornamental rim.
    GemMeter(P(342,502),264*K,38*K,3.47f,1.21f,DisplayStamina,H->bExhausted?FLinearColor(.09f,.11f,.17f):Blue,6);
    // Ruby inlay shares the chassis' continuous inner/outer rails, like stamina.
    GemMeter(P(342,502),303*K,26*K,3.47f,1.21f,H->PotionCharges/4.f,FLinearColor(.82f,.035f,.055f),4);
    const auto PotionKey=P(7,235);
    CardText(DungeonKeys::Label(DungeonKeys::Potion),PotionKey.X-15,PotionKey.Y,Gold,18,54,24,true);
    const auto Lightning=P(359,166);
    Sprite(TEXT("Hotbar_Lightning"),Lightning.X,Lightning.Y,110*K,110*K);

    const float Centers[]={665,850,1035,1220,1405,1590};
    const FString Keys[]={DungeonKeys::Label(DungeonKeys::Freedom),DungeonKeys::Label(DungeonKeys::Tea),TEXT("3"),TEXT("4")};
    auto Binding=[&](FVector2D C,int Action){const auto K=DungeonKeys::Key(Action);if(K==EKeys::LeftMouseButton)Mouse(C,0);else if(K==EKeys::MiddleMouseButton)Mouse(C,1);else if(K==EKeys::RightMouseButton)Mouse(C,2);else CardText(DungeonKeys::Label(Action),C.X-38,C.Y-8,Gold,17,76,30,true);};
    for(int I=0;I<6;++I){
        const FVector2D Tile=P(Centers[I]-77,465),Key=P(Centers[I]-40,626);
        const float S=154*K;
        const bool Ready=I==0?H->CanUseFreedom():I==1?H->CanDrinkTea():I==4?H->CanStrike():I==5?H->CanUseTea():false;
        const auto Tint=Ready?FLinearColor::White:FLinearColor(.42f,.45f,.45f);
        if(I==0){
            const float Charge=FMath::Clamp(G->FreedomKills/15.f,0.f,1.f);
            Sprite(TEXT("Hotbar_Freedom"),Tile.X,Tile.Y,S,S,FLinearColor(.16f,.16f,.16f));
            if(auto* Eagle=Texture(TEXT("Hotbar_Freedom"));Eagle&&Charge>0){
                const float Top=1-Charge;
                DrawTexture(Eagle,Offset.X+Tile.X*Scale,Offset.Y+(Tile.Y+S*Top)*Scale,S*Scale,S*Charge*Scale,
                    0,Top,1,Charge,FLinearColor::White,BLEND_Translucent);
            }
        }
        if(I==4)Sprite(TEXT("Hotbar_Attack"),Tile.X,Tile.Y,S,S,Tint);
        if(I==1){
            Sprite(TEXT("Hotbar_TeaSpirit"),Tile.X,Tile.Y,S,S,H->IsTeaEmpowered()?FLinearColor(1,.95f,.75f):Tint);
            if(H->IsTeaEmpowered()){
                Box(Tile.X,Tile.Y+S-3,S*H->GetTeaSpiritTime()/FDungeonTeaSpirit::Duration,3,Gold);
                CardText(FString::Printf(TEXT("%.1f"),H->GetTeaSpiritTime()),Tile.X,Tile.Y+S*.35f,Pale,21,S,26,true);
            }
        }
        if(I==5)KeySprite(TEXT("TeaFX_0"),Tile.X+4,Tile.Y+4,S-8,S-8,Tint);
        if(I>1&&I<4)Sprite(TEXT("AudioThumb"),Tile.X+S*.32f,Tile.Y+S*.3f,S*.36f,S*.4f,FLinearColor(.32f,.3f,.22f,.8f));
        const float Cooldown=I==1?H->GetTeaSpiritCooldown():I==5?H->GetPowerCooldown():I==4&&H->IsAttacking()?(1-H->GetAttackProgress())*.48f/FMath::Max(.01f,H->AttackSpeed):0;
        const float Fraction=I==1?1-H->GetTeaSpiritCooldown()/FDungeonTeaSpirit::Recharge:I==0?FMath::Clamp(G->FreedomKills/15.f,0.f,1.f):I==4?H->IsAttacking()?H->GetAttackProgress():1.f:I==5?1-H->GetPowerCooldown()/10.f:0;
        if(Cooldown>0){
            Box(Tile.X,Tile.Y,S,S*(1-Fraction),FLinearColor(0,0,0,.60f));
            CardText(FString::Printf(TEXT("%.1f"),Cooldown),Tile.X,Tile.Y+S*.35f,Pale,21,S,26,true);
        }
        if(I>=4){Box(Tile.X+3,Tile.Y+S-4,S-6,2,Ink);Box(Tile.X+3,Tile.Y+S-4,(S-6)*Fraction,2,Ready?Green:Gold);}
        // Labels sit on the actual raised keycap artwork crossing the lower rail.
        if(I<4)CardText(Keys[I],Key.X-25*K,Key.Y,Gold,19,130*K,42*K,true);
        else Binding(P(Centers[I],647),I==4?DungeonKeys::Attack:DungeonKeys::Throw);
    }
    const FVector2D BlockCenter(985,702);
    Disk(BlockCenter,43,Ink);
    Sprite(TEXT("Hotbar_OrbFrame"),918,635,134,134);
    GemMeter(BlockCenter,42,10,-PI/2,2*PI,H->GetBlockFraction(),Green,12);
    Sprite(TEXT("Hotbar_Block"),959,676,52,52,H->IsBlocking()||H->CanStartBlock()?FLinearColor::White:FLinearColor(.38f,.43f,.4f));
    if(H->GetBlockCountdown()>0){
        Box(967,702,36,23,FLinearColor(0,.018f,.008f,.8f));
        CardText(FString::Printf(TEXT("%.1f"),H->GetBlockCountdown()),967,702,Pale,18,36,23,true);
    }
    // Detached block medallion and utility map are deliberately outside chassis.
    Binding(FVector2D(985,766),DungeonKeys::Block);
    CardText(TEXT("BLOCK"),946,786,Gold,12,78,14,true);
    if(H->IsBlocking()||H->GetBlockCooldown()>0)CardText(H->IsBlocking()?TEXT("GUARDING"):TEXT("REFILLING"),940,618,Gold,12,90,18,true);
    else if(H->NeedsBlockRelease())CardText(TEXT("RELEASE ")+DungeonKeys::Label(DungeonKeys::Block),935,618,Gold,13,100,20,true);
    Sprite(TEXT("AtlasMapIcon"),1070,699,42,42);
    CardText(DungeonKeys::Label(DungeonKeys::Map)+TEXT(" MAP"),1053,751,Gold,13,76,20,true);
}

void ADungeonHUD::GemMeter(FVector2D C,float Radius,float Width,float Start,float Sweep,float Fraction,FLinearColor Tint,int Pieces)
{
    auto* Gem=Texture(TEXT("Hotbar_Gem"));
    if(!Canvas||!Gem)return;
    Fraction=FMath::Clamp(Fraction,0.f,1.f);
    auto Draw=[&](float Begin,float Length,FLinearColor Color){
        if(Length<=0)return;
        TArray<FCanvasUVTri> Tris;
        const int Steps=FMath::Max(1,FMath::CeilToInt(Length*Radius));
        auto Vertex=[&](float A,bool Outer,FVector2D& Pos,FVector2D& UV){
            FVector2D Direction(FMath::Cos(A),FMath::Sin(A));
            Pos=Offset+(C+Direction*(Radius+(Outer?Width/2:-Width/2)))*Scale;
            // UVs include the generated gemstone's inner/outer chamfered edges.
            UV=FVector2D(.5f,.494f)+Direction*(Outer?.45f:.297f);
        };
        for(int I=0;I<Steps;++I){
            const float A=Begin+Length*I/Steps,B=Begin+Length*(I+1)/Steps;
            FVector2D Pos[4],UV[4];
            Vertex(A,false,Pos[0],UV[0]);Vertex(A,true,Pos[1],UV[1]);
            Vertex(B,true,Pos[2],UV[2]);Vertex(B,false,Pos[3],UV[3]);
            for(int J=0;J<2;++J){
                const int V1=J?2:1,V2=J?3:2;FCanvasUVTri T;
                T.V0_Pos=Pos[0];T.V1_Pos=Pos[V1];T.V2_Pos=Pos[V2];
                T.V0_UV=UV[0];T.V1_UV=UV[V1];T.V2_UV=UV[V2];
                T.V0_Color=T.V1_Color=T.V2_Color=Color;Tris.Add(T);
            }
        }
        Canvas->K2_DrawTriangle(Gem,MoveTemp(Tris));
    };
    const float Segment=Sweep/Pieces,Gap=.018f;
    for(int I=0;I<Pieces;++I){
        const float A=Start+Segment*I+Gap,Length=Segment-2*Gap;
        Draw(A,Length,FLinearColor(.023f,.033f,.03f));
        // Clip the angular end only; never stretch the visible texture as it drains.
        Draw(A,Length*FMath::Clamp(Fraction*Pieces-I,0.f,1.f),Tint);
    }
}

void ADungeonHUD::MeterArc(FVector2D C,float Radius,float Start,float Sweep,float Fraction,FLinearColor Color,float Width)
{
    const float Length=Sweep*FMath::Clamp(Fraction,0.f,1.f);
    if(!Canvas||Length<=0)return;
    const int Segments=FMath::Max(1,FMath::CeilToInt(FMath::Abs(Length)*Radius/2));
    TArray<FCanvasUVTri> Triangles;Triangles.Reserve(Segments*2);
    auto Point=[&](float A,float R){return Offset+(C+FVector2D(FMath::Cos(A),FMath::Sin(A))*R)*Scale;};
    auto Triangle=[&](FVector2D A,FVector2D B,FVector2D D){
        FCanvasUVTri T;T.V0_Pos=A;T.V1_Pos=B;T.V2_Pos=D;
        T.V0_UV=T.V1_UV=T.V2_UV=FVector2D::ZeroVector;
        T.V0_Color=T.V1_Color=T.V2_Color=Color;Triangles.Add(T);
    };
    // Exact annular strips: thick line caps would fill the stamina segment gaps
    // and repeatedly blend translucent guard feedback over itself.
    for(int I=0;I<Segments;++I){
        const float A=Start+Length*I/Segments,B=Start+Length*(I+1)/Segments;
        const auto AI=Point(A,Radius-Width/2),AO=Point(A,Radius+Width/2),BI=Point(B,Radius-Width/2),BO=Point(B,Radius+Width/2);
        Triangle(AI,AO,BO);Triangle(AI,BO,BI);
    }
    Canvas->K2_DrawTriangle(nullptr,MoveTemp(Triangles));
}
void ADungeonHUD::DrawGuard(ADungeonHero* H)
{
    if(!H->IsBlocking())return;
    const auto P=DungeonView::Project(H->GetActorLocation())-FVector2D(0,39),Aim=H->GetVisualFacing();
    const float A=FMath::Atan2(Aim.Y,Aim.X),Flash=H->GetBlockImpact()/.18f;
    MeterArc(P,52,A-PI*.5f,PI,1,FLinearColor(.08f,.7f,.42f,.3f+Flash*.5f),3+Flash*3);
    if(Flash>0)Sprite(TEXT("Hotbar_Block"),P.X+Aim.X*47-17,P.Y+Aim.Y*47-17,34,34,FLinearColor(.8f,1,.85f,Flash));
}
void ADungeonHUD::DrawPotions(ADungeonGameMode* G)
{
    for(const auto& P:G->GetPotions())
    {
        const float X=P.Position.X,Y=P.Position.Y+FMath::Sin(P.Age*3)*2;
        Shadow(P.Position,20);
        Ring(P.Position,21+FMath::Sin(P.Age*3)*2,FLinearColor(1,.12f,.18f,.35f),2);
        Sprite(TEXT("HUD_Potion"),X-32,Y-60,64,64);
    }
}
