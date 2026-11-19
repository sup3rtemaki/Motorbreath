#if WITH_DEV_AUTOMATION_TESTS

#include "../PhysicsVehicle.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPhysicsVehicleSelectedWeaponTest, "Motorbreath.PhysicsVehicle.SelectedWeapon",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPhysicsVehicleSelectedWeaponTest::RunTest(const FString& Parameters)
{
    UClass* MineClass = StaticLoadClass(AActor::StaticClass(), nullptr,
        TEXT("/Game/Blueprints/DamageActors/Dropped/BP_Mine.BP_Mine_C"));
    UClass* MissileClass = StaticLoadClass(AActor::StaticClass(), nullptr,
        TEXT("/Game/Blueprints/DamageActors/Missiles/BP_Missile.BP_Missile_C"));
    if (!TestNotNull(TEXT("Mine Blueprint class"), MineClass) ||
        !TestNotNull(TEXT("Missile Blueprint class"), MissileClass))
    {
        return false;
    }

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world"), World))
    {
        return false;
    }

    APhysicsVehicle* Vehicle = World->SpawnActor<APhysicsVehicle>();
    UInventory* Inventory = Vehicle ? Vehicle->FindComponentByClass<UInventory>() : nullptr;
    FBoolProperty* bFireSelectedWeaponProperty = FindFProperty<FBoolProperty>(
        APhysicsVehicle::StaticClass(), TEXT("bFireSelectedWeapon"));
    FDoubleProperty* WeaponTimerProperty = FindFProperty<FDoubleProperty>(
        APhysicsVehicle::StaticClass(), TEXT("WeaponTimer"));
    FDoubleProperty* MaxWeaponTimeProperty = FindFProperty<FDoubleProperty>(
        APhysicsVehicle::StaticClass(), TEXT("MaxWeaponTime"));
    FArrayProperty* TargetListProperty = FindFProperty<FArrayProperty>(
        APhysicsVehicle::StaticClass(), TEXT("TargetList"));
    FObjectPropertyBase* TargetListInnerProperty = TargetListProperty
        ? CastField<FObjectPropertyBase>(TargetListProperty->Inner)
        : nullptr;
    FObjectPropertyBase* DroppedSpawnPointProperty = CastField<FObjectPropertyBase>(
        FindFProperty<FProperty>(APhysicsVehicle::StaticClass(), TEXT("DroppedSpawnPoint")));
    FObjectPropertyBase* MissileSpawnPointProperty = CastField<FObjectPropertyBase>(
        FindFProperty<FProperty>(APhysicsVehicle::StaticClass(), TEXT("MissileSpawnPoint")));
    USceneComponent* DroppedSpawnPoint = Vehicle && DroppedSpawnPointProperty
        ? Cast<USceneComponent>(DroppedSpawnPointProperty->GetObjectPropertyValue_InContainer(Vehicle))
        : nullptr;
    USceneComponent* MissileSpawnPoint = Vehicle && MissileSpawnPointProperty
        ? Cast<USceneComponent>(MissileSpawnPointProperty->GetObjectPropertyValue_InContainer(Vehicle))
        : nullptr;
    if (!TestNotNull(TEXT("Spawned vehicle"), Vehicle) ||
        !TestNotNull(TEXT("Vehicle inventory"), Inventory) ||
        !TestNotNull(TEXT("Selected weapon fire flag"), bFireSelectedWeaponProperty) ||
        !TestNotNull(TEXT("Weapon timer"), WeaponTimerProperty) ||
        !TestNotNull(TEXT("Maximum weapon cooldown"), MaxWeaponTimeProperty) ||
        !TestNotNull(TEXT("Target list property"), TargetListProperty) ||
        !TestNotNull(TEXT("Target list element property"), TargetListInnerProperty) ||
        !TestNotNull(TEXT("Dropped spawn point"), DroppedSpawnPoint) ||
        !TestNotNull(TEXT("Missile spawn point"), MissileSpawnPoint))
    {
        World->DestroyWorld(false);
        return false;
    }

    FWeaponStruct MineItem;
    MineItem.Id = 1;
    MineItem.Name = TEXT("Mine");
    MineItem.Quantity = 1;
    MineItem.Weapon = MineClass;
    Vehicle->AddWeapon(MineItem);
    bFireSelectedWeaponProperty->SetPropertyValue_InContainer(Vehicle, true);
    Vehicle->FireSelectedWeapon();

    TArray<AActor*> SpawnedMines;
    UGameplayStatics::GetAllActorsOfClass(World, MineClass, SpawnedMines);
    TestEqual(TEXT("Selected mine is spawned"), SpawnedMines.Num(), 1);
    TestEqual(TEXT("Successful fire starts the selected weapon cooldown"),
        WeaponTimerProperty->GetPropertyValue_InContainer(Vehicle),
        MaxWeaponTimeProperty->GetPropertyValue_InContainer(Vehicle));
    TestFalse(TEXT("Successful fire clears the queued request"),
        bFireSelectedWeaponProperty->GetPropertyValue_InContainer(Vehicle));
    TestEqual(TEXT("Fired mine is consumed"), Inventory->Weapons.Num(), 0);
    if (SpawnedMines.Num() == 1)
    {
        TestTrue(TEXT("Mine spawns at DroppedSpawnPoint"),
            SpawnedMines[0]->GetActorLocation().Equals(
                DroppedSpawnPoint->GetComponentLocation(), 0.1));
    }

    AActor* Target = World->SpawnActor<AActor>();
    if (!TestNotNull(TEXT("Missile target"), Target))
    {
        World->DestroyWorld(false);
        return false;
    }
    Target->SetActorLocation(FVector(1000.0, 0.0, 0.0));
    FScriptArrayHelper TargetArray(TargetListProperty,
        TargetListProperty->ContainerPtrToValuePtr<void>(Vehicle));
    const int32 TargetIndex = TargetArray.AddValue();
    TargetListInnerProperty->SetObjectPropertyValue(TargetArray.GetRawPtr(TargetIndex), Target);

    FWeaponStruct MissileItem;
    MissileItem.Id = 2;
    MissileItem.Name = TEXT("Missile");
    MissileItem.Quantity = 2;
    MissileItem.Weapon = MissileClass;
    Vehicle->AddWeapon(MissileItem);
    WeaponTimerProperty->SetPropertyValue_InContainer(Vehicle, 0.0);
    bFireSelectedWeaponProperty->SetPropertyValue_InContainer(Vehicle, true);
    Vehicle->FireSelectedWeapon();

    TArray<AActor*> SpawnedMissiles;
    UGameplayStatics::GetAllActorsOfClass(World, MissileClass, SpawnedMissiles);
    TestEqual(TEXT("Selected missile is spawned"), SpawnedMissiles.Num(), 1);
    if (SpawnedMissiles.Num() == 1)
    {
        FObjectPropertyBase* TargetActorProperty = CastField<FObjectPropertyBase>(
            FindFProperty<FProperty>(MissileClass, TEXT("TargetActor")));
        TestNotNull(TEXT("Missile target property"), TargetActorProperty);
        if (TargetActorProperty)
        {
            TestEqual(TEXT("Nearest target is assigned to the missile"),
                TargetActorProperty->GetObjectPropertyValue_InContainer(SpawnedMissiles[0]),
                static_cast<UObject*>(Target));
        }
        TestTrue(TEXT("Missile spawns at MissileSpawnPoint"),
            SpawnedMissiles[0]->GetActorLocation().Equals(
                MissileSpawnPoint->GetComponentLocation(), 0.1));
    }
    if (!TestEqual(TEXT("One missile stack remains"), Inventory->Weapons.Num(), 1))
    {
        World->DestroyWorld(false);
        return false;
    }
    TestEqual(TEXT("Firing consumes one missile"), Inventory->Weapons[0].Quantity, 1);

    Vehicle->RequestSelectedWeaponFire();
    Vehicle->FireSelectedWeapon();
    WeaponTimerProperty->SetPropertyValue_InContainer(Vehicle, 0.0);
    Vehicle->FireSelectedWeapon();
    UGameplayStatics::GetAllActorsOfClass(World, MissileClass, SpawnedMissiles);
    TestEqual(TEXT("A request during cooldown does not fire after cooldown"), SpawnedMissiles.Num(), 1);
    if (TestEqual(TEXT("A request during cooldown keeps the missile stack"), Inventory->Weapons.Num(), 1))
    {
        TestEqual(TEXT("A request during cooldown does not consume ammunition"), Inventory->Weapons[0].Quantity, 1);
    }

    World->DestroyWorld(false);
    return true;
}

#endif
