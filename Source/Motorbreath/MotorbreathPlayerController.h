#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "MotorbreathPlayerController.generated.h"

class APhysicsVehicle;
class UChildActorComponent;
class UInputAction;
class UInputMappingContext;
class UInventory;
class UUserWidget;
struct FInputActionValue;

UCLASS()
class MOTORBREATH_API AMotorbreathPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AMotorbreathPlayerController();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupInputComponent() override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void AcknowledgePossession(APawn* InPawn) override;

    UFUNCTION(BlueprintCallable, Client, Reliable, Category = "Player Controller")
    void Client_NotifyReady();

    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Player Controller")
    void Server_SetSelectedVehicleClass(TSubclassOf<APawn> VehicleClass);

    UFUNCTION(BlueprintCallable, Client, Reliable, Category = "Player Controller")
    void Client_ConfirmPossession(APawn* InPawn);

    UFUNCTION(BlueprintCallable, Client, Reliable, Category = "Player Controller")
    void Client_CreateHUDAndMappingContext();

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> DriveAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> TurnAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> FireAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> HandbrakeAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> FireSelectedWeaponAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> CycleInventoryAscendingAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> CycleInventoryDescendingAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> CarMappingContext;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    TSubclassOf<UUserWidget> InGameHUDClass;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Player Controller")
    TObjectPtr<UUserWidget> InGameHUD;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Player Controller")
    TObjectPtr<APawn> PossessedPawn;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Player Controller")
    TObjectPtr<APhysicsVehicle> PhysicsVehicle;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Player Controller")
    TObjectPtr<UInventory> PhysicsVehicleInventory;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Player Controller")
    TObjectPtr<AActor> RadarManager;

private:
    void HandleDrive(const FInputActionValue& Value);
    void HandleDriveCompleted(const FInputActionValue& Value);
    void HandleTurn(const FInputActionValue& Value);
    void HandleTurnCompleted(const FInputActionValue& Value);
    void HandleFire(const FInputActionValue& Value);
    void HandleHandbrake(const FInputActionValue& Value);
    void HandleHandbrakeCompleted(const FInputActionValue& Value);
    void HandleFireSelectedWeapon(const FInputActionValue& Value);
    void HandleCycleInventoryAscending(const FInputActionValue& Value);
    void HandleCycleInventoryDescending(const FInputActionValue& Value);
    void HandleToggleCamera();

    UFUNCTION()
    void HandleWeaponChanged();

    UFUNCTION()
    void HandleCurrentWeaponUpdated();

    void InitializeLocalVehicle(APawn* InPawn);
    void TryConfirmPossession();
    void EnsureHUD();
    void AddCarMappingContext(bool bClearExistingMappings);
    void BindInventoryDelegates();
    void UpdateRadarReferences();
    APhysicsVehicle* GetControlledVehicle() const;

    void Server_SetSelectedVehicleClass_Implementation(TSubclassOf<APawn> VehicleClass);

    void Client_NotifyReady_Implementation();

    void Client_ConfirmPossession_Implementation(APawn* InPawn);

    void Client_CreateHUDAndMappingContext_Implementation();

    FTimerHandle ConfirmPossessionRetryHandle;
};
