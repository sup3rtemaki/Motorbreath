#if WITH_DEV_AUTOMATION_TESTS

#include "../PhysicsVehicle.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPhysicsVehicleSteeringTest, "Motorbreath.PhysicsVehicle.Steering",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPhysicsVehicleSteeringTest::RunTest(const FString& Parameters)
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
    USceneComponent* FrontRight = Vehicle ? FindObject<USceneComponent>(Vehicle, TEXT("Wheel_FR_Pos")) : nullptr;
    USceneComponent* FrontLeft = Vehicle ? FindObject<USceneComponent>(Vehicle, TEXT("Wheel_FL_Pos")) : nullptr;
    UStaticMeshComponent* FrontRightMesh = Vehicle ? FindObject<UStaticMeshComponent>(Vehicle, TEXT("WheelMesh_FR")) : nullptr;
    FBoolProperty* IsDead = FindFProperty<FBoolProperty>(APhysicsVehicle::StaticClass(), TEXT("bIsDead"));
    if (!TestNotNull(TEXT("Vehicle"), Vehicle) ||
        !TestNotNull(TEXT("Right wheel"), FrontRight) ||
        !TestNotNull(TEXT("Left wheel"), FrontLeft) ||
        !TestNotNull(TEXT("Right wheel mesh"), FrontRightMesh) ||
        !TestNotNull(TEXT("Death state"), IsDead))
    {
        GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
        return false;
    }

    Vehicle->Server_SetSteeringAngle(1.0, true);
    Vehicle->Tick(0.1f);
    TestTrue(TEXT("Right front wheel steers toward the target"),
        FMath::IsNearlyEqual(FrontRight->GetRelativeRotation().Yaw, 27.0, 0.01));
    TestTrue(TEXT("Both front wheels steer equally"),
        FrontRight->GetRelativeRotation().Equals(FrontLeft->GetRelativeRotation(), 0.01));

    IsDead->SetPropertyValue_InContainer(Vehicle, true);
    Vehicle->Server_SetSteeringAngle(-1.0, true);
    Vehicle->Tick(0.1f);
    TestTrue(TEXT("A dead vehicle stops updating steering"),
        FMath::IsNearlyEqual(FrontRight->GetRelativeRotation().Yaw, 27.0, 0.01));

    IsDead->SetPropertyValue_InContainer(Vehicle, false);
    Vehicle->Server_SetSteeringAngle(0.6, true);
    const FQuat PreviousRotation = FrontRightMesh->GetRelativeRotation().Quaternion();
    Vehicle->AddActorWorldOffset(FrontRight->GetForwardVector() * 10.0, false, nullptr, ETeleportType::TeleportPhysics);
    Vehicle->Tick(0.1f);
    const FQuat ExpectedRotation = PreviousRotation * FRotator(-10.0, 0.0, 0.0).Quaternion();
    TestTrue(TEXT("Wheel rolls according to its forward displacement"),
        FrontRightMesh->GetRelativeRotation().Quaternion().Equals(ExpectedRotation, 0.001));

    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    return true;
}

#endif
