#pragma once

#include "CoreMinimal.h"
#include "ItemTypes.generated.h"

UENUM(BlueprintType)
enum class EItemType : uint8
{
    WeaponItem UMETA(DisplayName = "Weapon Item"),
    MedKit UMETA(DisplayName = "Med Kit"),
    RepairKit UMETA(DisplayName = "Repair Kit"),
    Default UMETA(DisplayName = "Default")
};
