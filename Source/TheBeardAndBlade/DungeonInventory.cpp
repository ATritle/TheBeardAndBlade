#include "DungeonActors.h"
#include "DungeonInventoryLayout.h"
using namespace DungeonInventoryLayout;
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace {
constexpr int PortraitFrames=32;
int PortraitFrame(float Yaw) { return FMath::RoundToInt(Yaw*PortraitFrames/360.f)%PortraitFrames; }
}
void ADungeonHUD::RotateInventoryPortrait(float DeltaX)
{
    // One complete revolution per 480 logical UI pixels; no vertical rotation.
    PortraitYaw=FMath::Fmod(PortraitYaw+DeltaX*.75f,360.f);
    if(PortraitYaw<0)PortraitYaw+=360.f;
}
void ADungeonHUD::DrawInventoryTurntable()
{
    auto* PC=GetOwningPlayerController();float X=0,Y=0;
    if(bPortraitDragging) {
        if(!PC||Scale<=0||!PC->GetMousePosition(X,Y)||!PC->IsInputKeyDown(EKeys::LeftMouseButton))bPortraitDragging=false;
        else {const float LogicalX=(X-Offset.X)/Scale;RotateInventoryPortrait(LogicalX-PortraitLastX);PortraitLastX=LogicalX;}
    }
    // Warm every frame together to avoid a first-drag texture streaming blur.
    for(int I=0;I<PortraitFrames;++I)Texture(FString::Printf(TEXT("InventoryTurn_%02d"),I));
    int Frame=PortraitFrame(PortraitYaw);
#if !UE_BUILD_SHIPPING
    FParse::Value(FCommandLine::Get(),TEXT("InventoryTurntableFrame="),Frame);
    Frame=FMath::Clamp(Frame,0,PortraitFrames-1);
#endif
    const FString Name=FString::Printf(TEXT("InventoryTurn_%02d"),Frame);
    Sprite(Texture(Name)?Name:TEXT("InventoryAdventurer"),84,155,280,420);
}

bool ADungeonHero::MoveBagItem(int32 Index,FIntPoint Cell)
{
    if(!Inventory.IsValidIndex(Index)||!CanPlace(Cell,Inventory[Index].Item.Size(),Index)) return false;
    Inventory[Index].Cell=Cell; SelectedItem=Index; InventoryMessage.Empty(); return true;
}
void ADungeonHUD::CancelInventoryGesture()
{
    DragItem=LastClickedItem=DragGear=LastClickedGear=INDEX_NONE; bDragging=false; bPortraitDragging=false; LastClickTime=-1;
}
bool ADungeonHero::UnequipToCell(int32 Slot,FIntPoint Cell)
{
    if(!Equipment.IsValidIndex(Slot)||Equipment[Slot].IsEmpty()||!CanPlace(Cell,Equipment[Slot].Size()))return false;
    // Validate the entire footprint before changing either container.
    Inventory.Add({Equipment[Slot],Cell});SelectedItem=Inventory.Num()-1;
    FDungeonItem Empty;Empty.Slot=Slot;Equip(Empty);InventoryMessage.Empty();return true;
}
void ADungeonHUD::UpdateInventoryDrag(ADungeonHero* H)
{
    if(DragItem==INDEX_NONE&&DragGear==INDEX_NONE) return;
    auto* PC=GetOwningPlayerController(); float X=0,Y=0;
    const bool FromGear=DragGear!=INDEX_NONE;
    const bool Valid=FromGear?H->Equipment.IsValidIndex(DragGear)&&!H->Equipment[DragGear].IsEmpty():H->Inventory.IsValidIndex(DragItem);
    if(!Valid||!PC||Scale<=0||!PC->GetMousePosition(X,Y)) {CancelInventoryGesture();return;}
    DragMouse=(FVector2D(X,Y)-Offset)/Scale;
    if(FVector2D::Distance(DragMouse,DragStart)>6) bDragging=true;
    if(PC->IsInputKeyDown(EKeys::LeftMouseButton)) return;
    if(bDragging)
    {
        const int Index=DragItem;
        bool Dropped=false;
        if(!FromGear)for(int Slot=0;Slot<DungeonLootCatalog::EquipmentSlots;++Slot)
            if(GearAt(DragMouse)==Slot)
                if(H->Inventory[Index].Item.Slot==Slot) Dropped=H->EquipFromInventory(Index);
        if(!Dropped&&In(DragMouse,BagX,BagY,360,360))
        {
            const auto TopLeft=DragMouse-DragGrab;
            Dropped=FromGear?H->UnequipToCell(DragGear,BagCell(TopLeft)):H->MoveBagItem(Index,BagCell(TopLeft));
        }
        if(!Dropped) H->InventoryMessage=TEXT("Cannot place here.");
        CancelInventoryGesture();
    }
    else DragItem=DragGear=INDEX_NONE; // Keep double-click identity after an ordinary release.
}
void ADungeonHUD::DrawInventoryDrag(ADungeonHero* H)
{
    if(!bDragging) return;
    const bool FromGear=DragGear!=INDEX_NONE;
    if(FromGear?(!H->Equipment.IsValidIndex(DragGear)||H->Equipment[DragGear].IsEmpty()):!H->Inventory.IsValidIndex(DragItem))return;
    const auto& Item=FromGear?H->Equipment[DragGear]:H->Inventory[DragItem].Item; const auto Size=Item.Size();
    const auto P=DragMouse-DragGrab;
    const FIntPoint Cell=BagCell(P);
    bool Valid=false;
    if(In(DragMouse,BagX,BagY,360,360))
    {
        Valid=H->CanPlace(Cell,Size,FromGear?INDEX_NONE:DragItem);
        if(Cell.X>=0&&Cell.Y>=0&&Cell.X+Size.X<=6&&Cell.Y+Size.Y<=6)
            Box(BagX+Cell.X*60,BagY+Cell.Y*60,Size.X*60-2,Size.Y*60-2,Valid?FLinearColor(0,.4f,.1f,.45f):FLinearColor(.7f,0,0,.45f));
    }
    if(!FromGear)for(int Slot=0;Slot<DungeonLootCatalog::EquipmentSlots;++Slot) if(GearAt(DragMouse)==Slot)
    {
        Valid=Item.Slot==Slot;Box(Gear(Slot).X,Gear(Slot).Y,80,80,Valid?FLinearColor(0,.4f,.1f,.45f):FLinearColor(.7f,0,0,.45f));
    }
    const float W=Size.X*60-8,Ht=Size.Y*60-8,Icon=FMath::Min(W,Ht);
    if(Item.Slot==0)Sprite(Item.BagArt(),P.X+3,P.Y+3,W,Ht,FLinearColor(1,1,1,.8f));
    else Sprite(Item.BagArt(),P.X+3+(W-Icon)/2,P.Y+3+(Ht-Icon)/2,Icon,Icon,FLinearColor(1,1,1,.8f));
}
int32 ADungeonHUD::VerifyInventoryGestures(ADungeonHero* H)
{
    auto* PC=GetOwningPlayerController();if(!PC||!H||!H->IsInventoryOpen()) return 1;
    const auto Bag=H->Inventory;const auto Gear=H->Equipment;const float HP=H->Health,SP=H->Stamina;
    int Errors=0;
    int CheckNumber=0;
    auto Check=[&](bool OK){++CheckNumber;if(!OK){++Errors;UE_LOG(LogTemp,Warning,TEXT("INVENTORY_GESTURE_CHECK_FAILED %d"),CheckNumber);}};
    auto Mouse=[&](float X,float Y){PC->SetMouseLocation(FMath::RoundToInt(Offset.X+X*Scale),FMath::RoundToInt(Offset.Y+Y*Scale));};
    auto Drag=[&](float X,float Y,float TX,float TY){CancelInventoryGesture();Mouse(X,Y);InventoryClick();Mouse(TX,TY);UpdateInventoryDrag(H);};
    const float SavedYaw=PortraitYaw;
    PortraitYaw=0;RotateInventoryPortrait(480);Check(FMath::IsNearlyZero(PortraitYaw));
    RotateInventoryPortrait(-15);Check(PortraitFrame(PortraitYaw)==31);
    RotateInventoryPortrait(15);Check(PortraitFrame(PortraitYaw)==0);
    for(int I=0;I<PortraitFrames;++I)Check(Texture(FString::Printf(TEXT("InventoryTurn_%02d"),I))!=nullptr);
    Mouse(220,350);InventoryClick();Check(bPortraitDragging&&DragGear==INDEX_NONE&&DragItem==INDEX_NONE);
    CancelInventoryGesture();Check(!bPortraitDragging);PortraitYaw=SavedYaw;
    H->Inventory.Empty();H->AddToInventory(ADungeonGameMode::RollItem(8,4));H->AddToInventory(ADungeonGameMode::RollItem(24,2));
    const auto Weapon=H->Inventory[0].Item;
    Drag(476,198,716,438);Check(H->Inventory[0].Cell==FIntPoint(4,4));
    Drag(716,438,536,198);Check(H->Inventory[0].Cell==FIntPoint(4,4)); // occupied armor
    Drag(716,438,820,550);Check(H->Inventory[0].Cell==FIntPoint(4,4)); // outside
    Drag(716,438,350,460);Check(H->Equipment[0].Attack==Gear[0].Attack&&H->Inventory[0].Cell==FIntPoint(4,4)); // wrong slot
    Drag(716,438,70,460);Check(H->Equipment[0].CatalogId==Weapon.CatalogId&&H->Equipment[0].Attack==Weapon.Attack);
    const auto Old=H->Inventory[0].Item;
    CancelInventoryGesture();Mouse(716,438);InventoryClick();InventoryClick();Check(H->Equipment[0].Name==Old.Name&&H->Equipment[0].Attack==Old.Attack);
    Check(!H->MoveBagItem(0,FIntPoint(5,5))&&!H->MoveBagItem(-1,FIntPoint(0,0)));
    FDungeonItem EmptyRing;EmptyRing.Slot=3;H->Equip(EmptyRing);
    H->Inventory.Empty();H->AddToInventory(ADungeonGameMode::RollItem(48,4));
    const auto RingItem=H->Inventory[0].Item;
    Drag(476,198,350,590);Check(H->Equipment[3].CatalogId==48&&H->Equipment[3].CoinValue==RingItem.CoinValue);
    Check(H->Unequip(3)&&H->Equipment[3].IsEmpty());
    CancelInventoryGesture();Mouse(476,198);InventoryClick();InventoryClick();Check(H->Equipment[3].CatalogId==48);
    // Aim inside the destination cell, not exactly on its boundary:
    // viewport scaling rounds synthetic cursor positions to physical pixels.
    Drag(350,590,676,398);
    Check(H->Equipment[3].IsEmpty()&&H->Inventory.Num()==1&&H->Inventory[0].Cell==FIntPoint(3,3)&&H->Inventory[0].Item.CoinValue==RingItem.CoinValue);
    CancelInventoryGesture();Mouse(666,388);InventoryClick();InventoryClick();Check(H->Equipment[3].CatalogId==48);
    CancelInventoryGesture();Mouse(350,590);InventoryClick();InventoryClick();Check(H->Equipment[3].IsEmpty()&&H->Inventory.Num()==1);
    H->EquipFromInventory(0);
    for(int I=0;I<36;++I)H->AddToInventory(ADungeonGameMode::RollItem(36,0));
    CancelInventoryGesture();Mouse(350,590);InventoryClick();InventoryClick();Check(H->Equipment[3].CatalogId==48&&H->Inventory.Num()==36);
    Drag(350,590,486,208);Check(H->Equipment[3].CatalogId==48&&H->Inventory.Num()==36);
    Check(!H->UnequipToCell(3,FIntPoint(6,0))&&!H->UnequipToCell(-1,FIntPoint(0,0)));
    H->Inventory.Empty();H->Equip(ADungeonGameMode::RollItem(26,4));
    Drag(350,450,786,498);Check(!H->Equipment[1].IsEmpty()&&H->Inventory.IsEmpty());
    Drag(350,450,676,388);Check(H->Equipment[1].IsEmpty()&&H->Inventory.Num()==1&&H->Inventory[0].Cell==FIntPoint(3,3));
    for(int S=4;S<DungeonLootCatalog::EquipmentSlots;++S) {
        H->Inventory.Empty();FDungeonItem Empty;Empty.Slot=S;H->Equip(Empty);
        const int ID=60+(S-4)*3;H->AddToInventory(ADungeonGameMode::RollItem(ID,3,4));
        const auto P=DungeonInventoryLayout::Gear(S)+FVector2D(30,30);
        Drag(476,198,P.X,P.Y);Check(H->Equipment[S].CatalogId==ID&&H->Inventory.IsEmpty());
        CancelInventoryGesture();Mouse(P.X,P.Y);InventoryClick();InventoryClick();
        Check(H->Equipment[S].IsEmpty()&&H->Inventory.Num()==1);
        CancelInventoryGesture();Mouse(476,198);InventoryClick();InventoryClick();Check(H->Equipment[S].CatalogId==ID);
        Drag(P.X,P.Y,676,398);Check(H->Equipment[S].IsEmpty()&&H->Inventory.Num()==1&&H->Inventory[0].Cell==FIntPoint(3,3));
    }
    H->Inventory=Bag;H->Equipment=Gear;H->RebuildStats();H->Health=HP;H->Stamina=SP;H->SelectedItem=INDEX_NONE;H->InventoryMessage.Empty();CancelInventoryGesture();
    return Errors;
}
