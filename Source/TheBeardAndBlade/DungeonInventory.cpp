#include "DungeonActors.h"
#include "DungeonInventoryLayout.h"
using namespace DungeonInventoryLayout;
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

bool ADungeonHero::MoveBagItem(int32 Index,FIntPoint Cell)
{
    if(!Inventory.IsValidIndex(Index)||!CanPlace(Cell,Inventory[Index].Item.Size(),Index)) return false;
    Inventory[Index].Cell=Cell; SelectedItem=Index; InventoryMessage.Empty(); return true;
}
void ADungeonHUD::CancelInventoryGesture()
{
    DragItem=LastClickedItem=DragGear=LastClickedGear=INDEX_NONE; bDragging=false; LastClickTime=-1;
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
        if(!FromGear)for(int Slot=0;Slot<4;++Slot)
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
    if(!FromGear)for(int Slot=0;Slot<4;++Slot) if(GearAt(DragMouse)==Slot)
    {
        Valid=Item.Slot==Slot;Box(Gear(Slot).X,Gear(Slot).Y,80,80,Valid?FLinearColor(0,.4f,.1f,.45f):FLinearColor(.7f,0,0,.45f));
    }
    Sprite(Item.BagArt(),P.X+3,P.Y+3,Size.X*60-8,Size.Y*60-8,FLinearColor(1,1,1,.8f));
}
int32 ADungeonHUD::VerifyInventoryGestures(ADungeonHero* H)
{
    auto* PC=GetOwningPlayerController();if(!PC||!H||!H->IsInventoryOpen()) return 1;
    const auto Bag=H->Inventory;const auto Gear=H->Equipment;const float HP=H->Health,SP=H->Stamina;
    int Errors=0;
    auto Check=[&](bool OK){if(!OK)++Errors;};
    auto Mouse=[&](float X,float Y){PC->SetMouseLocation(FMath::RoundToInt(Offset.X+X*Scale),FMath::RoundToInt(Offset.Y+Y*Scale));};
    auto Drag=[&](float X,float Y,float TX,float TY){CancelInventoryGesture();Mouse(X,Y);InventoryClick();Mouse(TX,TY);UpdateInventoryDrag(H);};
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
    Drag(350,590,666,388);
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
    Drag(350,450,666,378);Check(H->Equipment[1].IsEmpty()&&H->Inventory.Num()==1&&H->Inventory[0].Cell==FIntPoint(3,3));
    H->Inventory=Bag;H->Equipment=Gear;H->RebuildStats();H->Health=HP;H->Stamina=SP;H->SelectedItem=INDEX_NONE;H->InventoryMessage.Empty();CancelInventoryGesture();
    return Errors;
}
