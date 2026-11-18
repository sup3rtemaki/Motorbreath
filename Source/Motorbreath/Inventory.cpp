#include "Inventory.h"

#include "Kismet/KismetSystemLibrary.h"

void UInventory::BeginPlay()
{
    Super::BeginPlay();
    InventoryIndex = 0;
}

void UInventory::AddItem(const FWeaponStruct& NewItem)
{
    bFoundItem = false;
    for (FWeaponStruct& ExistingItem : Weapons)
    {
        if (ExistingItem.Id == NewItem.Id)
        {
            ExistingItem.Quantity += NewItem.Quantity;
            bFoundItem = true;
            break;
        }
    }

    if (bFoundItem)
    {
        UKismetSystemLibrary::PrintString(this,
            FString(TEXT("Updated   ")) + NewItem.Name,
            true, true, FLinearColor(0.0f, 0.66f, 1.0f, 1.0f), 2.0f, NAME_None);
    }
    else
    {
        Weapons.Add(NewItem);
        MaxInventorySize = Weapons.Num();
        UKismetSystemLibrary::PrintString(this,
            FString(TEXT("Picked  ")) + NewItem.Name,
            true, true, FLinearColor(0.0f, 0.66f, 1.0f, 1.0f), 2.0f, NAME_None);
    }

    UpdateCurrentWeapon.Broadcast();
    WeaponChanged.Broadcast();
    bFoundItem = false;
}

void UInventory::CycleInventoryAscending()
{
    ++InventoryIndex;
    if (InventoryIndex >= MaxInventorySize)
    {
        InventoryIndex = 0;
    }

    WeaponChanged.Broadcast();
}

void UInventory::CycleInventoryDescending()
{
    --InventoryIndex;
    if (InventoryIndex < 0)
    {
        InventoryIndex = MaxInventorySize > 0 ? MaxInventorySize - 1 : 0;
    }

    WeaponChanged.Broadcast();
}

void UInventory::SubtractItem()
{
    if (!Weapons.IsValidIndex(InventoryIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot subtract an item from an invalid inventory index."));
        return;
    }

    FWeaponStruct& SelectedItem = Weapons[InventoryIndex];
    --SelectedItem.Quantity;
    if (SelectedItem.Quantity > 0)
    {
        return;
    }

    Weapons.RemoveAt(InventoryIndex);
    MaxInventorySize = Weapons.Num();
    InventoryIndex = 0;
}
