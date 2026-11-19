// BP_Missile.h
#pragma once

#include "CoreMinimal.h"
#include "DamageActorBase.h"
#include "Missile.generated.h"

class UCapsuleComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;
class UNiagaraSystem;
class UPrimitiveComponent;

/**
 * Missile actor. Converted from the BP_Missile Blueprint EventGraph.
 */
UCLASS()
class MOTORBREATH_API AMissile : public ADamageActorBase
{
    GENERATED_BODY()

public:
    AMissile();

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Missile")
    TObjectPtr<UProjectileMovementComponent> ProjectileMovement = nullptr;

protected:
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION()
    void OnCollisionBoxHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    virtual void PostInitializeComponents() override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Missile")
    TObjectPtr<UStaticMeshComponent> StaticMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Missile")
    TObjectPtr<UStaticMeshComponent> RearGlowCylinder;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Missile|VFX")
    TObjectPtr<UNiagaraSystem> ExplosionSystem;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Missile|VFX")
    TObjectPtr<UNiagaraSystem> TriangleFragsSystem;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Missile|VFX")
    TObjectPtr<UNiagaraSystem> SmokeSystem;
};