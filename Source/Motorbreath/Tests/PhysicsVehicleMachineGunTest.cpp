#if WITH_DEV_AUTOMATION_TESTS

#include "../PhysicsVehicle.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPhysicsVehicleMachineGunTest, "Motorbreath.PhysicsVehicle.MachineGun",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPhysicsVehicleMachineGunTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world"), World))
    {
        return false;
    }

    APhysicsVehicle* Vehicle = World->SpawnActor<APhysicsVehicle>();
    FDoubleProperty* MachineGunTimer = FindFProperty<FDoubleProperty>(
        APhysicsVehicle::StaticClass(), TEXT("MachineGunTimer"));
    FDoubleProperty* MaxMachineGunTime = FindFProperty<FDoubleProperty>(
        APhysicsVehicle::StaticClass(), TEXT("MaxMachineGunTime"));
    FBoolProperty* bFireMachinegun = FindFProperty<FBoolProperty>(
        APhysicsVehicle::StaticClass(), TEXT("bFireMachinegun"));
    FClassProperty* BulletClassProperty = FindFProperty<FClassProperty>(
        APhysicsVehicle::StaticClass(), TEXT("MachineGunBulletClass"));
    if (!TestNotNull(TEXT("Vehicle"), Vehicle) ||
        !TestNotNull(TEXT("Machine gun timer"), MachineGunTimer) ||
        !TestNotNull(TEXT("Maximum machine gun cooldown"), MaxMachineGunTime) ||
        !TestNotNull(TEXT("Machine gun firing flag"), bFireMachinegun) ||
        !TestNotNull(TEXT("Bullet class property"), BulletClassProperty))
    {
        World->DestroyWorld(false);
        return false;
    }

    UClass* BulletClass = Cast<UClass>(BulletClassProperty->GetObjectPropertyValue_InContainer(Vehicle));
    if (!TestNotNull(TEXT("Bullet class asset"), BulletClass))
    {
        World->DestroyWorld(false);
        return false;
    }

    Vehicle->RequestMachineGunFire();
    MachineGunTimer->SetPropertyValue_InContainer(Vehicle, 0.05);
    Vehicle->FireMachinegun();
    TestTrue(TEXT("Fire request remains queued during cooldown"),
        bFireMachinegun->GetPropertyValue_InContainer(Vehicle));

    MachineGunTimer->SetPropertyValue_InContainer(Vehicle, 0.0);
    Vehicle->FireMachinegun();
    TestFalse(TEXT("Successful fire clears the queued request"),
        bFireMachinegun->GetPropertyValue_InContainer(Vehicle));
    TestEqual(TEXT("Successful fire starts the cooldown"),
        MachineGunTimer->GetPropertyValue_InContainer(Vehicle),
        MaxMachineGunTime->GetPropertyValue_InContainer(Vehicle));

    TArray<AActor*> Bullets;
    UGameplayStatics::GetAllActorsOfClass(World, BulletClass, Bullets);
    TestEqual(TEXT("Exactly one bullet is spawned after cooldown"), Bullets.Num(), 1);
    if (Bullets.Num() == 1)
    {
        FNumericProperty* DamageProperty = CastField<FNumericProperty>(
            FindFProperty<FProperty>(BulletClass, TEXT("Damage")));
        TestNotNull(TEXT("Spawned bullet damage property"), DamageProperty);
        if (DamageProperty)
        {
            TestEqual(TEXT("Machine gun bullet receives Blueprint damage value"),
                DamageProperty->GetFloatingPointPropertyValue(
                    DamageProperty->ContainerPtrToValuePtr<void>(Bullets[0])),
                2.0);
        }
    }

    World->DestroyWorld(false);
    return true;
}

#endif
