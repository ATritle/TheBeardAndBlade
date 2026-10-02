#include "../Source/TheBeardAndBlade/DungeonRevival.h"
#include <cassert>
#include <limits>
#include <iostream>
int main(){
    using DungeonRevival::Cost;
    assert(Cost(0)==0&&Cost(-1)==0&&Cost(1)==1);
    assert(Cost(2499)==2499&&Cost(2500)==1250&&Cost(2501)==1251);
    assert(Cost(6001)==3001);
    for(std::int64_t Gold=1;Gold<100000;++Gold){
        const auto Toll=Cost(Gold);assert(Toll>0&&Toll<=Gold);
        if(Gold<2500)assert(Toll==Gold);else assert(Gold-Toll==Gold/2);
    }
    const auto Max=std::numeric_limits<std::int64_t>::max();
    assert(Cost(Max)==Max/2+1&&Max-Cost(Max)==Max/2);
    std::cout<<"Revival cost boundaries and 99,999 wallet values passed.\n";
}
