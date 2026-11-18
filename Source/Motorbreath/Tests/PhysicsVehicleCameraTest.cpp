#if WITH_DEV_AUTOMATION_TESTS

#include "../PhysicsVehicle.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "GameFramework/SpringArmComponent.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPhysicsVehicleCameraTest, "Motorbreath.PhysicsVehicle.Camera",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPhysicsVehicleCameraTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world"), World))
    {
        return false;
    }

    APhysicsVehicle* Vehicle = World->SpawnActor<APhysicsVehicle>();
    USpringArmComponent* FrontArm = Vehicle ? FindObject<USpringArmComponent>(Vehicle, TEXT("SpringArm")) : nullptr;
    USpringArmComponent* RearArm = Vehicle ? FindObject<USpringArmComponent>(Vehicle, TEXT("RearSpringArm")) : nullptr;
    UCameraComponent* FrontCamera = Vehicle ? FindObject<UCameraComponent>(Vehicle, TEXT("ChaseCamera")) : nullptr;
    UCameraComponent* RearCamera = Vehicle ? FindObject<UCameraComponent>(Vehicle, TEXT("RearChaseCamera")) : nullptr;
    UCameraComponent* InCarCamera = Vehicle ? FindObject<UCameraComponent>(Vehicle, TEXT("InCarCamera")) : nullptr;
    if (!TestNotNull(TEXT("Vehicle"), Vehicle) ||
        !TestNotNull(TEXT("Front spring arm"), FrontArm) ||
        !TestNotNull(TEXT("Rear spring arm"), RearArm) ||
        !TestNotNull(TEXT("Front chase camera"), FrontCamera) ||
        !TestNotNull(TEXT("Rear chase camera"), RearCamera) ||
        !TestNotNull(TEXT("In-car camera"), InCarCamera))
    {
        World->DestroyWorld(false);
        return false;
    }

    TestTrue(TEXT("Front chase camera is initially active"), FrontCamera->IsActive());
    TestFalse(TEXT("Rear chase camera is initially inactive"), RearCamera->IsActive());
    TestFalse(TEXT("In-car camera is initially inactive"), InCarCamera->IsActive());
    TestTrue(TEXT("Front camera is attached to its spring arm"), FrontCamera->GetAttachParent() == FrontArm);
    TestTrue(TEXT("Rear camera is attached to its spring arm"), RearCamera->GetAttachParent() == RearArm);

    Vehicle->ToggleVehicleCamera();
    TestFalse(TEXT("Toggle deactivates the front camera"), FrontCamera->IsActive());
    TestTrue(TEXT("Toggle activates the rear camera"), RearCamera->IsActive());
    Vehicle->ToggleVehicleCamera();
    TestTrue(TEXT("Second toggle restores the front camera"), FrontCamera->IsActive());
    TestFalse(TEXT("Second toggle deactivates the rear camera"), RearCamera->IsActive());

    UInputComponent* Input = NewObject<UInputComponent>(Vehicle);
    Vehicle->SetupPlayerInputComponent(Input);
    TestEqual(TEXT("Mouse look binds both configured axes"), Input->AxisBindings.Num(), 2);
    TestEqual(TEXT("Camera toggle binds the configured action"), Input->GetNumActionBindings(), 1);

    World->DestroyWorld(false);
    return true;
}

#endif
