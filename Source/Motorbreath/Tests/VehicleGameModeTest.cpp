#if WITH_DEV_AUTOMATION_TESTS

#include "../MotorbreathPlayerState.h"
#include "../PhysicsVehicle.h"
#include "../VehicleGameMode.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVehicleGameModeSpawnTest, "Motorbreath.GameMode.VehicleSpawn",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVehicleGameModeSpawnTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world"), World))
    {
        return false;
    }

    AVehicleGameMode* GameMode = World->SpawnActor<AVehicleGameMode>();
    APlayerStart* PlayerStart = World->SpawnActor<APlayerStart>();
    APlayerController* DefaultController = World->SpawnActor<APlayerController>();
    AMotorbreathPlayerState* DefaultState = World->SpawnActor<AMotorbreathPlayerState>();
    APlayerController* VehicleController = World->SpawnActor<APlayerController>();
    AMotorbreathPlayerState* VehicleState = World->SpawnActor<AMotorbreathPlayerState>();
    if (!TestNotNull(TEXT("Game mode"), GameMode) ||
        !TestNotNull(TEXT("Player start"), PlayerStart) ||
        !TestNotNull(TEXT("Default controller"), DefaultController) ||
        !TestNotNull(TEXT("Default player state"), DefaultState) ||
        !TestNotNull(TEXT("Vehicle controller"), VehicleController) ||
        !TestNotNull(TEXT("Vehicle player state"), VehicleState))
    {
        World->DestroyWorld(false);
        return false;
    }

    GameMode->DefaultPawnClass = APawn::StaticClass();
    DefaultController->PlayerState = DefaultState;
    GameMode->Server_RequestSpawnVehicle(DefaultController);
    APawn* DefaultPawn = DefaultController->GetPawn();
    TestNotNull(TEXT("Missing selected class spawns and possesses the default pawn"), DefaultPawn);
    TestTrue(TEXT("Default spawn sets the duplicate-request flag"), DefaultState->bHasRequestedOrSpawnedPawn);
    GameMode->Server_RequestSpawnVehicle(DefaultController);
    TestTrue(TEXT("A repeated default request does not replace the pawn"), DefaultController->GetPawn() == DefaultPawn);

    VehicleController->PlayerState = VehicleState;
    VehicleState->SetSelectedVehicleClass(APhysicsVehicle::StaticClass());
    GameMode->Server_RequestSpawnVehicle(VehicleController);
    APawn* VehiclePawn = VehicleController->GetPawn();
    TestTrue(TEXT("Selected vehicle is spawned and possessed"), VehiclePawn && VehiclePawn->IsA<APhysicsVehicle>());
    TestTrue(TEXT("Selected vehicle is owned by its controller"), VehiclePawn && VehiclePawn->GetOwner() == VehicleController);
    TestTrue(TEXT("Selected vehicle spawn sets the duplicate-request flag"), VehicleState->bHasRequestedOrSpawnedPawn);
    GameMode->Server_RequestSpawnVehicle(VehicleController);
    TestTrue(TEXT("A repeated vehicle request does not replace the pawn"), VehicleController->GetPawn() == VehiclePawn);

    World->DestroyWorld(false);
    return true;
}

#endif
