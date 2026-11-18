#if WITH_DEV_AUTOMATION_TESTS

#include "../PhysicsVehicle.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPhysicsVehicleRecoveryTest, "Motorbreath.PhysicsVehicle.Recovery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPhysicsVehicleRecoveryTest::RunTest(const FString& Parameters)
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
    FDoubleProperty* Timer = FindFProperty<FDoubleProperty>(APhysicsVehicle::StaticClass(), TEXT("UpsideDownTimer"));
    FDoubleProperty* MaxTime = FindFProperty<FDoubleProperty>(APhysicsVehicle::StaticClass(), TEXT("MaxUpsideDownTime"));
    FBoolProperty* IsDead = FindFProperty<FBoolProperty>(APhysicsVehicle::StaticClass(), TEXT("bIsDead"));
    if (!TestNotNull(TEXT("Vehicle"), Vehicle) ||
        !TestNotNull(TEXT("Upside-down timer"), Timer) ||
        !TestNotNull(TEXT("Maximum upside-down time"), MaxTime) ||
        !TestNotNull(TEXT("Death property"), IsDead))
    {
        GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
        return false;
    }

    MaxTime->SetPropertyValue_InContainer(Vehicle, 1.0);
    Vehicle->SetActorRotation(FRotator(0.0, 42.0, 180.0), ETeleportType::TeleportPhysics);
    Vehicle->Tick(0.1f);
    TestTrue(TEXT("Inverted vehicle accumulates time for each brake-loop iteration"),
        FMath::IsNearlyEqual(Timer->GetPropertyValue_InContainer(Vehicle), 0.4, 0.001));

    Vehicle->SetActorRotation(FRotator(0.0, 42.0, 0.0), ETeleportType::TeleportPhysics);
    Vehicle->Tick(0.1f);
    TestTrue(TEXT("Returning upright does not reset the accumulated timer"),
        FMath::IsNearlyEqual(Timer->GetPropertyValue_InContainer(Vehicle), 0.4, 0.001));

    IsDead->SetPropertyValue_InContainer(Vehicle, true);
    Vehicle->SetActorRotation(FRotator(0.0, 42.0, 180.0), ETeleportType::TeleportPhysics);
    Vehicle->Tick(0.1f);
    TestTrue(TEXT("A dead vehicle skips recovery"),
        FMath::IsNearlyEqual(Timer->GetPropertyValue_InContainer(Vehicle), 0.4, 0.001));

    IsDead->SetPropertyValue_InContainer(Vehicle, false);
    MaxTime->SetPropertyValue_InContainer(Vehicle, 0.65);
    Vehicle->Tick(0.1f);
    TestTrue(TEXT("Recovery preserves yaw and removes roll and pitch"),
        Vehicle->GetActorRotation().Equals(FRotator(0.0, 42.0, 0.0), 0.01));
    TestTrue(TEXT("Recovery lifts the vehicle by 1000 units"),
        FMath::IsNearlyEqual(Vehicle->GetActorLocation().Z, 1000.0, 0.01));
    TestEqual(TEXT("Recovery resets the timer"), Timer->GetPropertyValue_InContainer(Vehicle), 0.0);

    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    return true;
}

#endif
