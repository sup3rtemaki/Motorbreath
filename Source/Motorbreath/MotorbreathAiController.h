#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "TimerManager.h"
#include "MotorbreathAiController.generated.h"

class APhysicsVehicle;
class ARecastNavMesh;
class UBehaviorTree;
class USplineComponent;

UCLASS(Blueprintable)
class MOTORBREATH_API AMotorbreathAiController : public AAIController
{
    GENERATED_BODY()

public:
    AMotorbreathAiController();

    virtual void Tick(float DeltaSeconds) override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "AI")
    TObjectPtr<USplineComponent> Spline;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "AI")
    TObjectPtr<USplineComponent> DynamicSpline;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "AI")
    TObjectPtr<APhysicsVehicle> ControlledCar;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "AI")
    TObjectPtr<ARecastNavMesh> RecastNavMesh;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "AI")
    TObjectPtr<APhysicsVehicle> ChaseTargetActor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    bool bUsingRandomPointDynamicSpline = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    bool bUsingRandomItemDynamicSpline = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    bool bIsCloseToDestination = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    bool bSkipEnemyDetection = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    double Timer = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    double TimerRandomLimit = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Timing", meta = (ClampMin = "0.0"))
    double MinTime = 1.5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Timing", meta = (ClampMin = "0.0"))
    double MaxTime = 3.0;

private:
    struct FItemPriorityEntry
    {
        TWeakObjectPtr<AActor> Item;
        int32 Priority = 0;
    };

    void GetAllItemsInMap();
    void DecideAggressiveness();
    void UpdateRandomMovement();
    void SyncItemsInMapWithBlueprintVariable();
    bool GetControllerBool(FName PropertyName, bool DefaultValue) const;
    void SetControllerBool(FName PropertyName, bool Value);
    UObject* GetControllerObject(FName PropertyName) const;
    void SetControllerObject(FName PropertyName, UObject* Value);
    bool CheckHp() const;
    bool CheckArmor() const;

    UPROPERTY(Transient)
    TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

    UPROPERTY(EditDefaultsOnly, Category = "AI|Assets")
    TSubclassOf<AActor> AiSplineClass;

    UPROPERTY(EditDefaultsOnly, Category = "AI|Assets")
    TSubclassOf<AActor> ItemClass;

    UPROPERTY(EditDefaultsOnly, Category = "AI|Assets")
    TSubclassOf<AActor> MedKitClass;

    UPROPERTY(EditDefaultsOnly, Category = "AI|Assets")
    TSubclassOf<AActor> RepairKitClass;

    TArray<FItemPriorityEntry> ItemsInMap;
    FTimerHandle GetItemsTimerHandle;
    FTimerHandle AggressivenessTimerHandle;
    bool bReportedInvalidItemsInMapProperty = false;
};
