#pragma once
#include <algorithm>

namespace DungeonTeaDrink
{
    // Deliberate raise, lip contact, sip/tilt, untilt, and lower. No attack poses.
    inline constexpr float Duration=1.2f;
    inline constexpr float Ends[]={.12f,.24f,.36f,.52f,.76f,.90f,1.06f,Duration};
    struct Region { float X,Y,W,H,FootX,FootY; };
    // Measured opaque silhouette bounds plus safe alpha margin. Foot coordinates
    // are in the complete sheet: lower-row artwork is nine pixels higher.
    inline constexpr Region Regions[]={
        {58,50,274,446,194,493},{441,50,275,446,578,493},
        {825,50,275,446,962,493},{1205,50,279,446,1346,493},
        {56,550,276,449,194,996},{441,554,275,445,578,996},
        {825,554,275,445,962,996},{1208,554,276,445,1346,996}
    };
    inline constexpr float SheetWidth=1536,SheetHeight=1024,PixelScale=97.f/440.f;
    inline int Frame(float Elapsed)
    {
        for(int I=0;I<7;++I)if(Elapsed<Ends[I])return I;
        return 7;
    }
    inline float AuraRadius(float Age) { return 22.f+62.f*std::clamp(Age/1.65f,0.f,1.f); }
}
