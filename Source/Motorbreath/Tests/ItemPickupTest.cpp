#if WITH_DEV_AUTOMATION_TESTS

#include "../Inventory.h"
#include "../Item.h"
#include "../PhysicsVehicle.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemPickupTest, "Motorbreath.Inventory.ItemPickup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FItemPickupTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world"), World))
    {
        return false;
    }

    AItem* ItemActor = World->SpawnActor<AItem>();
    APhysicsVehicle* Vehicle = World->SpawnActor<APhysicsVehicle>();
    AActor* NonVehicleActor = World->SpawnActor<AActor>();
    UInventory* Inventory = Vehicle ? Vehicle->FindComponentByClass<UInventory>() : nullptr;
    if (!TestNotNull(TEXT("Item actor"), ItemActor) ||
        !TestNotNull(TEXT("Vehicle"), Vehicle) ||
        !TestNotNull(TEXT("Non-vehicle actor"), NonVehicleActor) ||
        !TestNotNull(TEXT("Vehicle inventory"), Inventory) ||
        !TestNotNull(TEXT("Item overlap capsule"), ItemActor ? ItemActor->Capsule.Get() : nullptr))
    {
        World->DestroyWorld(false);
        return false;
    }

    ItemActor->Item.Id = 7;
    ItemActor->Item.Name = TEXT("Test weapon");
    ItemActor->Item.Quantity = 3;
    ItemActor->Item.Weapon = APawn::StaticClass();

    const FHitResult SweepResult;
    ItemActor->Capsule->OnComponentBeginOverlap.Broadcast(
        ItemActor->Capsule, NonVehicleActor, nullptr, 0, false, SweepResult);
    TestFalse(TEXT("Non-vehicle overlap does not consume the pickup"),
        ItemActor->IsActorBeingDestroyed());
    TestEqual(TEXT("Non-vehicle overlap grants no weapons"), Inventory->Weapons.Num(), 0);

    ItemActor->Capsule->OnComponentBeginOverlap.Broadcast(
        ItemActor->Capsule, Vehicle, nullptr, 0, false, SweepResult);
    TestTrue(TEXT("Vehicle overlap destroys the pickup"), ItemActor->IsActorBeingDestroyed());
    TestEqual(TEXT("Pickup adds one inventory entry"), Inventory->Weapons.Num(), 1);
    if (Inventory->Weapons.Num() == 1)
    {
        TestEqual(TEXT("Pickup preserves weapon ID"), Inventory->Weapons[0].Id, 7);
        TestEqual(TEXT("Pickup preserves item quantity"), Inventory->Weapons[0].Quantity, 3);
    }

    World->DestroyWorld(false);
    return true;
}

#endif
