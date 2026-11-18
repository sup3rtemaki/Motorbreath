// ---- BP_Mine.h ----
#pragma once

#include "CoreMinimal.h"
#include "DamageActorBase.h"
#include "Mine.generated.h"

class UStaticMeshComponent;
class UNiagaraSystem;
class UPrimitiveComponent;

UCLASS()
class MOTORBREATH_API AMine : public ADamageActorBase
{
    GENERATED_BODY()

public:
    AMine();

protected:
    /** Called when the game starts or when spawned. */
    virtual void BeginPlay() override;

    /** Called every frame to update the ready state. */
    virtual void Tick(float DeltaSeconds) override;

    /** Overlap event (parent call only, the original Blueprint node chain was disabled). */
    virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

    /** Bound to CollisionBox->OnComponentHit. Spawns VFX, causes damage, spawns an impulse blast and destroys the mine. */
    UFUNCTION()
    void OnCollisionBoxHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

public:
    /** Whether the mine has become active and can trigger on hit. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mine")
    bool bReady = false;

    /** Time to wait before the mine becomes ready. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mine")
    double MaxSpawnTimeWait = 0.0;

    /** Visual mesh of the mine. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> StaticMesh;
};