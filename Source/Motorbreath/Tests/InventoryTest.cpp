#if WITH_DEV_AUTOMATION_TESTS

#include "../Inventory.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryFlowTest, "Motorbreath.Inventory.AddCycleSubtract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInventoryFlowTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world"), World))
    {
        return false;
    }

    AActor* Owner = World->SpawnActor<AActor>();
    UInventory* Inventory = Owner ? NewObject<UInventory>(Owner) : nullptr;
    if (Inventory)
    {
        Owner->AddInstanceComponent(Inventory);
        Inventory->RegisterComponent();
    }
    if (!TestNotNull(TEXT("Inventory component"), Inventory))
    {
        World->DestroyWorld(false);
        return false;
    }

    FWeaponStruct FirstItem;
    FirstItem.Id = 1;
    FirstItem.Name = TEXT("First");
    FirstItem.Quantity = 2;
    FirstItem.Weapon = APawn::StaticClass();
    Inventory->AddItem(FirstItem);
    TestEqual(TEXT("First item is inserted"), Inventory->Weapons.Num(), 1);
    TestEqual(TEXT("Inventory size follows the array"), Inventory->MaxInventorySize, 1);
    if (Inventory->Weapons.IsEmpty())
    {
        World->DestroyWorld(false);
        return false;
    }

    FWeaponStruct DuplicateItem = FirstItem;
    DuplicateItem.Name = TEXT("Duplicate");
    DuplicateItem.Quantity = 3;
    Inventory->AddItem(DuplicateItem);
    TestEqual(TEXT("Matching IDs stack into one slot"), Inventory->Weapons.Num(), 1);
    TestEqual(TEXT("Stacked quantity is combined"), Inventory->Weapons[0].Quantity, 5);
    TestEqual(TEXT("Existing item metadata is retained"), Inventory->Weapons[0].Name, FirstItem.Name);
    TestFalse(TEXT("AddItem resets its found-item state"), Inventory->bFoundItem);

    FWeaponStruct SecondItem = FirstItem;
    SecondItem.Id = 2;
    SecondItem.Name = TEXT("Second");
    SecondItem.Quantity = 1;
    Inventory->AddItem(SecondItem);
    TestEqual(TEXT("Different IDs create separate slots"), Inventory->Weapons.Num(), 2);

    Inventory->CycleInventoryAscending();
    TestEqual(TEXT("Ascending selection advances"), Inventory->InventoryIndex, 1);
    Inventory->CycleInventoryAscending();
    TestEqual(TEXT("Ascending selection wraps"), Inventory->InventoryIndex, 0);
    Inventory->CycleInventoryDescending();
    TestEqual(TEXT("Descending selection wraps"), Inventory->InventoryIndex, 1);

    Inventory->InventoryIndex = 0;
    Inventory->SubtractItem();
    TestEqual(TEXT("SubtractItem decrements a nonempty stack"), Inventory->Weapons[0].Quantity, 4);
    Inventory->Weapons[0].Quantity = 1;
    Inventory->SubtractItem();
    TestEqual(TEXT("An exhausted item is removed"), Inventory->Weapons.Num(), 1);
    TestEqual(TEXT("Inventory size updates after removal"), Inventory->MaxInventorySize, 1);
    TestEqual(TEXT("Selection resets after removal"), Inventory->InventoryIndex, 0);

    Inventory->Weapons[0].Quantity = 1;
    Inventory->SubtractItem();
    TestEqual(TEXT("Removing the last item empties the inventory"), Inventory->Weapons.Num(), 0);
    TestEqual(TEXT("Empty inventory size is zero"), Inventory->MaxInventorySize, 0);
    Inventory->CycleInventoryDescending();
    TestEqual(TEXT("Descending an empty inventory keeps index valid"), Inventory->InventoryIndex, 0);
    Inventory->CycleInventoryAscending();
    TestEqual(TEXT("Ascending an empty inventory keeps index valid"), Inventory->InventoryIndex, 0);

    World->DestroyWorld(false);
    return true;
}

#endif
