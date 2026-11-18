#if WITH_DEV_AUTOMATION_TESTS

#include "../PhysicsVehicle.h"

#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPhysicsVehicleTargetTest, "Motorbreath.PhysicsVehicle.FindNearestTarget",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPhysicsVehicleTargetTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world"), World))
    {
        return false;
    }

    APhysicsVehicle* Vehicle = World->SpawnActor<APhysicsVehicle>();
    APhysicsVehicle* FarTarget = World->SpawnActor<APhysicsVehicle>();
    APhysicsVehicle* NearTarget = World->SpawnActor<APhysicsVehicle>();
    FArrayProperty* TargetList = FindFProperty<FArrayProperty>(APhysicsVehicle::StaticClass(), TEXT("TargetList"));
    FObjectPropertyBase* TargetProperty = TargetList ? CastField<FObjectPropertyBase>(TargetList->Inner) : nullptr;
    if (!TestNotNull(TEXT("Vehicle"), Vehicle) ||
        !TestNotNull(TEXT("Far target"), FarTarget) ||
        !TestNotNull(TEXT("Near target"), NearTarget) ||
        !TestNotNull(TEXT("Target list property"), TargetList) ||
        !TestNotNull(TEXT("Target list element property"), TargetProperty))
    {
        World->DestroyWorld(false);
        return false;
    }

    FarTarget->SetActorLocation(FVector(300.0, 0.0, 0.0));
    NearTarget->SetActorLocation(FVector(100.0, 0.0, 0.0));

    double Distance = -1.0;
    AActor* NearestActor = NearTarget;
    Vehicle->FindNearestTarget(Distance, NearestActor);
    TestNull(TEXT("Empty list has no nearest actor"), NearestActor);
    TestEqual(TEXT("Empty list returns zero distance"), Distance, 0.0);

    FScriptArrayHelper TargetArray(TargetList, TargetList->ContainerPtrToValuePtr<void>(Vehicle));
    TargetProperty->SetObjectPropertyValue(TargetArray.GetRawPtr(TargetArray.AddValue()), FarTarget);
    TargetProperty->SetObjectPropertyValue(TargetArray.GetRawPtr(TargetArray.AddValue()), NearTarget);

    Vehicle->FindNearestTarget(Distance, NearestActor);
    TestTrue(TEXT("Nearest target is selected independent of list order"), NearestActor == NearTarget);
    TestEqual(TEXT("Distance is measured from the vehicle origin"), Distance, 100.0);

    World->DestroyWorld(false);
    return true;
}

#endif
