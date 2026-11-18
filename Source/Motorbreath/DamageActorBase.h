#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DamageActorBase.generated.h"

class UCapsuleComponent;

UCLASS(Blueprintable)
class MOTORBREATH_API ADamageActorBase : public AActor
{
    GENERATED_BODY()

public:
    ADamageActorBase();

    UFUNCTION(BlueprintCallable, Category = "Damage")
    void CauseDamage(AActor* OtherActor);

    UFUNCTION(BlueprintCallable, Category = "Damage")
    void DestroySelf();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    double Damage = 0.0;

    /** Called every frame. */
    virtual void Tick(float DeltaSeconds) override;

protected:
    /** Called when the game starts or when spawned. */
    virtual void BeginPlay() override;

    /** Elapsed time accumulated since BeginPlay. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    double Timer = 0.0;

    /** Maximum lifetime before this actor destroys itself. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    double MaxLifetime = 0.0;

    /** Collision capsule shared by damage-actor subclasses. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage")
    TObjectPtr<UCapsuleComponent> CollisionBox;
};