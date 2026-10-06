#pragma once
#include "FullBodyArtMetrics.h"

// Right-handed, one-handed registration in the final 256px body canvases.
// Never derive blade angles from index/pinky landmarks: their order flips on
// closed fists and cape-occluded hands. Weapon artwork remains interchangeable.
namespace HeroMeleeRig {
struct Pose { float X,Y,Angle; bool Behind; };
inline bool Supports(int C) { return C>=FullBodyArt::Idle && C<=FullBodyArt::Block; }
inline Pose Get(int C,int D,int F)
{
    const auto& S=FullBodyArt::Sockets[C*8+D][F];
    constexpr float Carry[]={24,32,38,-24,-26,-32,-30,28};
    constexpr float Sway[]={-3,-1,2,4,3,1,-2,-4};
    Pose P{S.X,S.Y,Carry[D],D==6||D==7};
    if(C==FullBodyArt::Walk||C==FullBodyArt::Run)
        P.Angle+=Sway[F]*(C==FullBodyArt::Run?.65f:1.f);
    if(C==FullBodyArt::Block){
        constexpr float Guard[]={24,30,22,-22,-28,-30,-24,-25};
        P.Angle=Guard[D]+(F==5?9.f:0.f);
    }
    if(C>=FullBodyArt::MeleeSlash&&C<=FullBodyArt::MeleeCombo){
        // Strike frame 4 remains synchronized with the existing damage event.
        const float Target=D*45.f;
        const float Slash[]={-90,-125,-100,-40,0,42,15,0};
        const float Back[]={90,120,100,42,0,-45,-20,0};
        const float Combo[]={-65,-95,-115,-70,0,28,10,0};
        P.Angle=Target+(C==FullBodyArt::MeleeSlash?Slash[F]:C==FullBodyArt::MeleeBackhand?Back[F]:Combo[F]);
        // An overhead wind-up is blade-up in every view, including west. It
        // cannot inherit a world-facing offset that points through the face.
        constexpr float Overhead[]={-55,-40,-20,-15,-20,30,35,45};
        if(C!=FullBodyArt::MeleeBackhand&&F==2)P.Angle=Overhead[D];
        if(C==FullBodyArt::MeleeCombo&&F==3)P.Angle=Overhead[D];
        if(F==6){
            const float Follow=Target+(C==FullBodyArt::MeleeSlash?Slash[5]:C==FullBodyArt::MeleeBackhand?Back[5]:Combo[5]);
            P.Angle=Follow+FMath::FindDeltaAngleDegrees(Follow,Carry[D])*.65f;
        }
        if(F==0||F==7)P.Angle=Carry[D];
        P.Behind=false;
        // The far hand can pass behind the chest/cape during the wind-up.
        if(D==7&&(F==0||F==1||F==6||F==7))P.Behind=true;
        if(D==6&&(F==0||F==6||F==7))P.Behind=true;
    }
    struct Correction { int C,D,F;float X,Y;bool Behind; };
    static constexpr Correction Fix[]={
        {0,5,0,86.09f,117.15f,false},{0,5,1,87.25f,117.15f,false},{0,5,2,87.17f,117.15f,false},{0,5,3,86.09f,117.15f,false},
        {0,5,4,86.09f,118.23f,false},{0,5,5,86.71f,118.23f,false},{0,5,6,86.09f,118.23f,false},{0,5,7,87.17f,118.23f,false},
        {6,5,0,87.43f,118.29f,false},{6,5,1,107.86f,68,false},{6,5,2,94.9f,56.48f,false},{6,5,3,94.38f,56.48f,false},
        {6,5,4,94.43f,56.52f,false},{6,5,5,128.29f,85.9f,false},{6,5,6,94.9f,57.05f,false},{6,5,7,87.43f,118.33f,false},
        {0,0,3,181,128,false},
        // North gait: right arm disappears UNDER the cape on the return step.
        {1,0,4,170,92,true},{1,0,5,172,97,true},{1,0,6,173,96,true},{1,0,7,174,98,true},
        {2,0,0,181,86,false},{2,0,1,184,91,false},{2,0,2,185,84,false},{2,0,3,187,82,false},
        {2,0,4,175,92,true},{2,0,5,171,99,true},{2,0,6,176,96,true},{2,0,7,180,99,true},
        // North-west far arm is not the visible left fist at screen left.
        {0,7,0,158,127,true},{0,7,1,158,126,true},{0,7,2,158,126,true},{0,7,3,158,126,true},
        {0,7,4,158,128,true},{0,7,5,159,129,true},{0,7,6,159,128,true},{0,7,7,159,128,true},
        {1,7,0,172,83,true},{1,7,1,176,79,true},{1,7,2,179,85,true},{1,7,3,177,88,true},
        {1,7,4,180,89,true},{1,7,5,173,85,true},{1,7,6,179,92,true},{1,7,7,181,85,true},
        {2,7,0,158,65,true},{2,7,1,168,107,true},{2,7,2,165,83,true},{2,7,3,166,77,true},
        {2,7,4,155,64,true},{2,7,5,161,81,true},{2,7,6,171,76,true},{2,7,7,168,76,true},
        // Missing detections and wrong-hand estimates in the attack atlases.
        {3,0,2,120,20,false},{3,5,2,93,20,false},
        {3,1,2,140,20,false},{3,1,4,220,55,false},
        {5,3,3,148,21,false},{5,5,3,95,20,false},
        {5,1,2,131,23,false},{5,1,3,175,27,false},{5,1,4,221,158,false},{5,1,5,215,172,false},
        {5,0,1,198,151,false},{5,0,3,176,43,false},{5,0,4,192,154,false},{5,0,5,192,149,false},
        {4,0,2,162,86,true},{4,1,3,192,88,false},
        {4,5,2,181,76,true},{4,6,2,174,72,true},{4,6,3,62,48,false},{4,6,4,40,65,false},
        {2,6,3,156,118,true},{2,6,4,152,104,true},
        {1,6,2,151,126,true},{1,6,4,157,115,true},{1,6,6,150,129,true},
        {3,6,4,20,80,false},{5,6,4,23,179,false},
        {3,7,5,115,159,false},{4,7,7,66,67,true},
        // Raised right fist, not belt/cape landmarks, in north-facing guards.
        {6,0,1,175,48,false},{6,0,2,174,48,false},{6,0,3,176,46,false},
        {6,0,4,174,59,false},{6,0,5,169,63,false},{6,0,6,174,49,false},
        {6,7,0,156,130,true},{6,7,1,84,49,true},{6,7,2,88,51,true},
        {6,7,3,92,40,true},{6,7,4,76,62,true},{6,7,5,96,61,true},{6,7,6,91,47,true},{6,7,7,159,129,true}
    };
    for(const auto& A:Fix)if(A.C==C&&A.D==D&&A.F==F){P.X=A.X;P.Y=A.Y;P.Behind=A.Behind;break;}
    return P;
}
// Correct the small intrinsic lean of off-axis handles, not the blade silhouette.
inline float HandleCorrection(int Id)
{
    return Id==5?-14.f:Id==6?-12.f:Id==18?-24.f:0.f;
}
}
