#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponStruct.h"
#include "Inventory.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWeaponChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUpdateCurrentWeaponSignature);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MOTORBREATH_API UInventory : public UActorComponent
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category = "Inventory")
    FWeaponChangedSignature WeaponChanged;

    UPROPERTY(BlueprintAssignable, Category = "Inventory")
    FUpdateCurrentWeaponSignature UpdateCurrentWeapon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    TArray<FWeaponStruct> Weapons;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    int32 InventoryIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    int32 MaxInventorySize = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    bool bFoundItem = false;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void AddItem(const FWeaponStruct& NewItem);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void CycleInventoryAscending();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void CycleInventoryDescending();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void SubtractItem();

protected:
    virtual void BeginPlay() override;
};
