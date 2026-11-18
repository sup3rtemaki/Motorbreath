#if WITH_DEV_AUTOMATION_TESTS

#include "../PhysicsVehicle.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPhysicsVehicleSuspensionTest, "Motorbreath.PhysicsVehicle.Suspension",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPhysicsVehicleSuspensionTest::RunTest(const FString& Parameters)
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
    UStaticMeshComponent* Wheel = Vehicle ? FindObject<UStaticMeshComponent>(Vehicle, TEXT("WheelMesh_FR")) : nullptr;
    FBoolProperty* Grounded = FindFProperty<FBoolProperty>(APhysicsVehicle::StaticClass(), TEXT("bGroundedFR"));
    FBoolProperty* IsDead = FindFProperty<FBoolProperty>(APhysicsVehicle::StaticClass(), TEXT("bIsDead"));
    FDoubleProperty* Force = FindFProperty<FDoubleProperty>(APhysicsVehicle::StaticClass(), TEXT("ForceFR"));
    FDoubleProperty* NormalizedSpeed = FindFProperty<FDoubleProperty>(APhysicsVehicle::StaticClass(), TEXT("NormalizedSpeed"));
    FDoubleProperty* RamTimer = FindFProperty<FDoubleProperty>(APhysicsVehicle::StaticClass(), TEXT("RamTimer"));
    AActor* Ground = World->SpawnActor<AActor>();
    if (!TestNotNull(TEXT("Vehicle"), Vehicle) ||
        !TestNotNull(TEXT("Wheel"), Wheel) ||
        !TestNotNull(TEXT("Grounded property"), Grounded) ||
        !TestNotNull(TEXT("Death property"), IsDead) ||
        !TestNotNull(TEXT("Force property"), Force) ||
        !TestNotNull(TEXT("Speed property"), NormalizedSpeed) ||
        !TestNotNull(TEXT("Timer property"), RamTimer) ||
        !TestNotNull(TEXT("Ground actor"), Ground))
    {
        GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
        return false;
    }

    Vehicle->Tick(0.1f);
    TestFalse(TEXT("Wheel without ground is airborne"), Grounded->GetPropertyValue_InContainer(Vehicle));
    TestTrue(TEXT("Airborne wheel hangs at rest distance minus radius"),
        FMath::IsNearlyEqual(Wheel->GetComponentLocation().Z, 0.0, 0.01));
    TestTrue(TEXT("Stationary normalized speed uses the Blueprint minimum"),
        FMath::IsNearlyEqual(NormalizedSpeed->GetPropertyValue_InContainer(Vehicle), 0.000002, 0.0000001));

    UBoxComponent* GroundBox = NewObject<UBoxComponent>(Ground);
    Ground->SetRootComponent(GroundBox);
    GroundBox->SetBoxExtent(FVector(1000.0, 1000.0, 10.0));
    GroundBox->SetCollisionProfileName(TEXT("BlockAll"));
    GroundBox->RegisterComponent();
    GroundBox->SetWorldLocation(FVector(0.0, 0.0, -25.0));
    Vehicle->Tick(0.1f);
    TestTrue(TEXT("Wheel detects the ground"), Grounded->GetPropertyValue_InContainer(Vehicle));
    TestTrue(TEXT("Wheel center is one radius above the hit"),
        FMath::IsNearlyEqual(Wheel->GetComponentLocation().Z, 20.0, 0.1));
    TestTrue(TEXT("Suspension force uses compression and spring strength"),
        FMath::IsNearlyEqual(Force->GetPropertyValue_InContainer(Vehicle), 20000.0, 1.0));

    GroundBox->SetWorldLocation(FVector(0.0, 0.0, -200.0));
    Vehicle->Tick(0.1f);
    TestFalse(TEXT("Wheel becomes airborne after losing contact"), Grounded->GetPropertyValue_InContainer(Vehicle));
    TestEqual(TEXT("Airborne suspension force is reset"), Force->GetPropertyValue_InContainer(Vehicle), 0.0);

    IsDead->SetPropertyValue_InContainer(Vehicle, true);
    RamTimer->SetPropertyValue_InContainer(Vehicle, 9.95);
    GroundBox->SetWorldLocation(FVector(0.0, 0.0, -25.0));
    Vehicle->Tick(0.1f);
    TestFalse(TEXT("A dead vehicle skips suspension updates"), Grounded->GetPropertyValue_InContainer(Vehicle));
    TestTrue(TEXT("Variable calculation continues for a dead vehicle"),
        FMath::IsNearlyEqual(RamTimer->GetPropertyValue_InContainer(Vehicle), 10.05, 0.001));
    Vehicle->Tick(0.1f);
    TestTrue(TEXT("Ram timer stops increasing after reaching ten seconds"),
        FMath::IsNearlyEqual(RamTimer->GetPropertyValue_InContainer(Vehicle), 10.05, 0.001));

    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    return true;
}

#endif
