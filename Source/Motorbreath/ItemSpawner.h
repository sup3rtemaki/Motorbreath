#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemTypes.h"
#include "ItemSpawner.generated.h"

class AItem;
class USceneComponent;

UCLASS()
class MOTORBREATH_API AItemSpawner : public AActor
{
    GENERATED_BODY()

public:
    AItemSpawner();

    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner|Components")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner|Components")
    TObjectPtr<USceneComponent> SpawnPoint;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Item")
    TObjectPtr<AActor> Item;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner|Classes")
    TSubclassOf<AItem> ItemClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner|Classes")
    TSubclassOf<AActor> MedKitClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner|Classes")
    TSubclassOf<AActor> RepairKitClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Respawn",
        meta = (ClampMin = "0.0", Units = "s"))
    double MedKitRespawnTime = 90.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Respawn",
        meta = (ClampMin = "0.0", Units = "s"))
    double RepairKitRespawnTime = 60.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Heal Items")
    double HealItemMaxSpawnTimeWait = 2.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Heal Items")
    double MedKitDamage = 20.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Heal Items")
    double RepairKitDamage = 35.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Respawn",
        meta = (ClampMin = "0.0", Units = "s"))
    double MaxTime = 30.0;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Spawner|Item")
    EItemType ItemType = EItemType::WeaponItem;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Spawner|Item")
    TObjectPtr<AActor> SpawnedItem;

protected:
    virtual void BeginPlay() override;

private:
    AActor* SpawnItemByType(EItemType Type);
    AActor* SpawnWeaponItem(const FTransform& SpawnTransform);
    AActor* SpawnMedKit(const FTransform& SpawnTransform);
    AActor* SpawnRepairKit(const FTransform& SpawnTransform);
    AActor* SpawnConfiguredKit(TSubclassOf<AActor> KitClass, double Damage,
        const FTransform& SpawnTransform, const TCHAR* KitType);

    UPROPERTY(Transient)
    TObjectPtr<AItem> CachedWeaponItem;

    double Timer = 0.0;
    bool bConfigured = false;
};
