// ---- BP_MedKit.h ----
#pragma once

#include "CoreMinimal.h"
#include "DamageActorBase.h"
#include "MedKit.generated.h"

class UNiagaraSystem;
class UStaticMeshComponent;
class UPrimitiveComponent;

UCLASS()
class MOTORBREATH_API AMedKit : public ADamageActorBase
{
    GENERATED_BODY()

public:
    AMedKit();

protected:
    virtual void BeginPlay() override;

    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION()
    void OnCollisionBoxHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "MedKit")
    TObjectPtr<UStaticMeshComponent> StaticMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MedKit")
    bool bReady = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MedKit")
    double MaxSpawnTimeWait = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MedKit")
    TObjectPtr<UNiagaraSystem> PickupEffect;
};