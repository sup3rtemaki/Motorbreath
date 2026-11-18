#pragma once

#include "CoreMinimal.h"
#include "Missile.h"
#include "GuidedMissile.generated.h"

class UProjectileMovementComponent;

UCLASS()
class MOTORBREATH_API AGuidedMissile : public AMissile
{
    GENERATED_BODY()

public:
    AGuidedMissile();

    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Missile")
    TObjectPtr<AActor> TargetActor = nullptr;

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Missile")
    TObjectPtr<UProjectileMovementComponent> ProjectileMovement = nullptr;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Missile")
    double HomingAcceleration = 70000.0;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Missile")
    bool ShouldStopHoming();
    virtual bool ShouldStopHoming_Implementation();

    UFUNCTION(BlueprintCallable, Category = "Missile")
    void SetHomingParameters();
};