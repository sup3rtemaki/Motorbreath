#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "MotorbreathPlayerState.generated.h"

class APawn;

UCLASS(Blueprintable)
class MOTORBREATH_API AMotorbreathPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Player State")
    TSubclassOf<APawn> GetSelectedVehicleClass() const;

    UFUNCTION(BlueprintCallable, Category = "Player State")
    bool SetSelectedVehicleClass(TSubclassOf<APawn> VehicleClass);

    UPROPERTY(BlueprintReadWrite, Category = "Player State")
    bool bHasRequestedOrSpawnedPawn = false;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    UPROPERTY(ReplicatedUsing = OnRep_SelectedVehicleClass, BlueprintReadOnly, Category = "Player State")
    TSubclassOf<APawn> SelectedVehicleClass;

    UFUNCTION()
    void OnRep_SelectedVehicleClass();

private:
    void SyncSelectedVehicleClassProperties();
};
