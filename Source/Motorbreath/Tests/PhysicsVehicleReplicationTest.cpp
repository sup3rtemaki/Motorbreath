#if WITH_DEV_AUTOMATION_TESTS

#include "../PhysicsVehicle.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPhysicsVehicleReplicationTest, "Motorbreath.PhysicsVehicle.Replication",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPhysicsVehicleReplicationTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world"), World))
    {
        return false;
    }

    APhysicsVehicle* Vehicle = World->SpawnActor<APhysicsVehicle>();
    FStructProperty* TargetLocation = FindFProperty<FStructProperty>(APhysicsVehicle::StaticClass(), TEXT("TargetLocation"));
    FStructProperty* TargetRotation = FindFProperty<FStructProperty>(APhysicsVehicle::StaticClass(), TEXT("TargetRotation"));
    FStructProperty* InterpTargetLocation = FindFProperty<FStructProperty>(APhysicsVehicle::StaticClass(), TEXT("InterpTargetLocation"));
    FStructProperty* InterpTargetRotation = FindFProperty<FStructProperty>(APhysicsVehicle::StaticClass(), TEXT("InterpTargetRotation"));
    FStructProperty* ReplicatedLocation = FindFProperty<FStructProperty>(APhysicsVehicle::StaticClass(), TEXT("ReplicatedLocation"));
    FStructProperty* ReplicatedRotation = FindFProperty<FStructProperty>(APhysicsVehicle::StaticClass(), TEXT("ReplicatedRotation"));
    if (!TestNotNull(TEXT("Vehicle"), Vehicle) ||
        !TestNotNull(TEXT("Target location"), TargetLocation) ||
        !TestNotNull(TEXT("Target rotation"), TargetRotation) ||
        !TestNotNull(TEXT("Interpolated target location"), InterpTargetLocation) ||
        !TestNotNull(TEXT("Interpolated target rotation"), InterpTargetRotation) ||
        !TestNotNull(TEXT("Multicast location"), ReplicatedLocation) ||
        !TestNotNull(TEXT("Multicast rotation"), ReplicatedRotation))
    {
        World->DestroyWorld(false);
        return false;
    }

    const FVector NewTargetLocation(100.0, 200.0, 300.0);
    const FRotator NewTargetRotation(10.0, 20.0, 30.0);
    *TargetLocation->ContainerPtrToValuePtr<FVector>(Vehicle) = NewTargetLocation;
    *TargetRotation->ContainerPtrToValuePtr<FRotator>(Vehicle) = NewTargetRotation;
    Vehicle->ProcessEvent(Vehicle->FindFunctionChecked(TEXT("OnRep_TargetLocation")), nullptr);
    Vehicle->ProcessEvent(Vehicle->FindFunctionChecked(TEXT("OnRep_TargetRotation")), nullptr);
    TestTrue(TEXT("Uncontrolled pawn copies replicated location to interpolation target"),
        *InterpTargetLocation->ContainerPtrToValuePtr<FVector>(Vehicle) == NewTargetLocation);
    TestTrue(TEXT("Uncontrolled pawn copies replicated rotation to interpolation target"),
        *InterpTargetRotation->ContainerPtrToValuePtr<FRotator>(Vehicle) == NewTargetRotation);

    const FVector MulticastLocation(400.0, 500.0, 600.0);
    const FRotator MulticastRotation(40.0, 50.0, 60.0);
    Vehicle->Multicast_UpdateTransform(MulticastLocation, MulticastRotation);
    TestTrue(TEXT("Multicast records the received location"),
        *ReplicatedLocation->ContainerPtrToValuePtr<FVector>(Vehicle) == MulticastLocation);
    TestTrue(TEXT("Multicast records the received rotation"),
        *ReplicatedRotation->ContainerPtrToValuePtr<FRotator>(Vehicle) == MulticastRotation);

    World->DestroyWorld(false);
    return true;
}

#endif
