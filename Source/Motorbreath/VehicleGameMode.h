#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "VehicleGameMode.generated.h"

class APlayerController;

UCLASS()
class MOTORBREATH_API AVehicleGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    virtual void PostLogin(APlayerController* NewPlayer) override;

    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Vehicle")
    void Server_RequestSpawnVehicle(APlayerController* PlayerControllerRef);

private:
    void ConfirmVehiclePossession(APlayerController* PlayerController, APawn* SpawnedPawn);
};
