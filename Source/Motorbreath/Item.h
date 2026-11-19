#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponStruct.h"
#include "Item.generated.h"

class UCapsuleComponent;
class UPrimitiveComponent;
class UStaticMeshComponent;

UCLASS()
class MOTORBREATH_API AItem : public AActor
{
    GENERATED_BODY()

public:
    AItem();

    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FWeaponStruct Item;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
    TObjectPtr<UStaticMeshComponent> StaticMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
    TObjectPtr<UCapsuleComponent> Capsule;

protected:
    UFUNCTION()
    void OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
        const FHitResult& SweepResult);
};
