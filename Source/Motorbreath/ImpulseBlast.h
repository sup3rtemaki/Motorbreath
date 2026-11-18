// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DamageActorBase.h"
#include "ImpulseBlast.generated.h"

class UStaticMeshComponent;
class UPrimitiveComponent;

UCLASS(Blueprintable)
class MOTORBREATH_API AImpulseBlast : public ADamageActorBase
{
    GENERATED_BODY()

public:
    AImpulseBlast();

    virtual void Tick(float DeltaSeconds) override;

    static bool ConfigureSpawnedBlast(AActor* SpawnedActor, double InBlastPower, double InDamage);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blast")
    double BlastPower = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Blast")
    TObjectPtr<UStaticMeshComponent> StaticMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Blast")
    TArray<TObjectPtr<AActor>> AffectedActors;

protected:
    virtual void PostInitializeComponents() override;

    UFUNCTION()
    void OnCollisionBoxHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    UFUNCTION()
    void OnCollisionBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
        const FHitResult& SweepResult);
};
