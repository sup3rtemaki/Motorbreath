#if WITH_DEV_AUTOMATION_TESTS

#include "../PhysicsVehicle.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPhysicsVehicleMovementTest, "Motorbreath.PhysicsVehicle.Movement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPhysicsVehicleMovementTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world"), World))
    {
        return false;
    }

    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    World->InitializeActorsForPlay(FURL());
    World->BeginPlay();
    World->Tick(LEVELTICK_All, 0.1f);

    APhysicsVehicle* Vehicle = World->SpawnActor<APhysicsVehicle>();
    FDoubleProperty* Acceleration = FindFProperty<FDoubleProperty>(APhysicsVehicle::StaticClass(), TEXT("Acceleration"));
    FDoubleProperty* TargetAcceleration = FindFProperty<FDoubleProperty>(APhysicsVehicle::StaticClass(), TEXT("TargetAcceleration"));
    FDoubleProperty* TargetBrake = FindFProperty<FDoubleProperty>(APhysicsVehicle::StaticClass(), TEXT("TargetBrake"));
    FBoolProperty* Handbrake = FindFProperty<FBoolProperty>(APhysicsVehicle::StaticClass(), TEXT("bHandbrake"));
    FBoolProperty* IsDead = FindFProperty<FBoolProperty>(APhysicsVehicle::StaticClass(), TEXT("bIsDead"));
    if (!TestNotNull(TEXT("Vehicle"), Vehicle) ||
        !TestNotNull(TEXT("Acceleration property"), Acceleration) ||
        !TestNotNull(TEXT("Target acceleration property"), TargetAcceleration) ||
        !TestNotNull(TEXT("Brake property"), TargetBrake) ||
        !TestNotNull(TEXT("Handbrake property"), Handbrake) ||
        !TestNotNull(TEXT("Death property"), IsDead))
    {
        GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
        return false;
    }

    Vehicle->SetVehicleAcceleration(0.5);
    TestEqual(TEXT("An uncontrolled pawn ignores local acceleration input"),
        TargetAcceleration->GetPropertyValue_InContainer(Vehicle), 0.0);
    Vehicle->SetVehicleBrake(0.5, true);
    TestEqual(TEXT("An uncontrolled pawn ignores local brake input"),
        TargetBrake->GetPropertyValue_InContainer(Vehicle), 0.0);

    Vehicle->Server_SetAcceleration(1.0);
    Vehicle->Server_SetBrake(0.75, true);
    TestEqual(TEXT("Server receives acceleration input"), TargetAcceleration->GetPropertyValue_InContainer(Vehicle), 1.0);
    TestEqual(TEXT("Server receives brake input"), TargetBrake->GetPropertyValue_InContainer(Vehicle), 0.75);
    TestTrue(TEXT("Server receives handbrake input"), Handbrake->GetPropertyValue_InContainer(Vehicle));

    Vehicle->Tick(0.1f);
    TestTrue(TEXT("Acceleration interpolates once for each iteration of the Blueprint brake loop"),
        FMath::IsNearlyEqual(Acceleration->GetPropertyValue_InContainer(Vehicle), 0.9744, 0.001));

    IsDead->SetPropertyValue_InContainer(Vehicle, true);
    Vehicle->Server_SetAcceleration(-1.0);
    Vehicle->Tick(0.1f);
    TestTrue(TEXT("A dead vehicle stops updating acceleration"),
        FMath::IsNearlyEqual(Acceleration->GetPropertyValue_InContainer(Vehicle), 0.9744, 0.001));

    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    return true;
}

#endif
