#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "ExplosiveTarget.generated.h"

class AController;
class UDamageType;
class UNiagaraComponent;
class UNiagaraSystem;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class MOTORBREATH_API AExplosiveTarget : public AActor
{
    GENERATED_BODY()

public:
    AExplosiveTarget();

    virtual void Tick(float DeltaSeconds) override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target|Components")
    TObjectPtr<UStaticMeshComponent> Cylinder;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target|Components")
    TObjectPtr<UNiagaraComponent> Flame;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target|Health")
    double Hp = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target|Health")
    double MaxHp = 100.0;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Target|Effects")
    TObjectPtr<UNiagaraSystem> ExplosionSystem;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Target|Effects")
    TSubclassOf<AActor> ImpulseBlastClass;

private:
    UFUNCTION()
    void HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
        AController* InstigatedBy, AActor* DamageCauser);

    void Detonate();

    FTimerHandle DetonationTimerHandle;
    bool bDetonationScheduled = false;
};
