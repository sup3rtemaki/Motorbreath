#pragma once

#include "CoreMinimal.h"
#include "WeaponStruct.generated.h"

class UTexture2D;
class UStaticMesh;

USTRUCT(BlueprintType)
struct MOTORBREATH_API FWeaponStruct
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    int32 Id = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    int32 Quantity = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    TSubclassOf<AActor> Weapon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    TObjectPtr<UTexture2D> Thumbnail;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    TObjectPtr<UStaticMesh> Mesh;
};
