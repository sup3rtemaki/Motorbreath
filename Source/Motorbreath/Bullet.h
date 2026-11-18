#pragma once

#include "CoreMinimal.h"
#include "DamageActorBase.h"
#include "Bullet.generated.h"

class UStaticMeshComponent;
class UNiagaraSystem;
class UPrimitiveComponent;

UCLASS()
class MOTORBREATH_API ABullet : public ADamageActorBase
{
    GENERATED_BODY()

public:
    ABullet();

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnCollisionBoxHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bullet")
    TObjectPtr<UStaticMeshComponent> StaticMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
    TObjectPtr<UNiagaraSystem> HitEffect;
};