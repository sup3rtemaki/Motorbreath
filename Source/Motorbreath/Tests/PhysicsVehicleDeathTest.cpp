#if WITH_DEV_AUTOMATION_TESTS

#include "../PhysicsVehicle.h"

#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "NiagaraComponent.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPhysicsVehicleDeathTest, "Motorbreath.PhysicsVehicle.Die",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPhysicsVehicleDeathTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world"), World))
    {
        return false;
    }

    APhysicsVehicle* Vehicle = World->SpawnActor<APhysicsVehicle>();
    FBoolProperty* IsDead = FindFProperty<FBoolProperty>(APhysicsVehicle::StaticClass(), TEXT("bIsDead"));
    UNiagaraComponent* MainFire = Vehicle
        ? FindObject<UNiagaraComponent>(Vehicle, TEXT("NS_MainFire"))
        : nullptr;
    if (!TestNotNull(TEXT("Vehicle"), Vehicle) ||
        !TestNotNull(TEXT("Death property"), IsDead) ||
        !TestNotNull(TEXT("Main fire component"), MainFire))
    {
        World->DestroyWorld(false);
        return false;
    }

    Vehicle->Die();
    TestTrue(TEXT("Die marks the vehicle as dead"), IsDead->GetPropertyValue_InContainer(Vehicle));
    TestTrue(TEXT("Die activates the main fire system"), MainFire->IsActive());

    World->DestroyWorld(false);
    return true;
}

#endif
