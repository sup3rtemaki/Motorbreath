#include "ItemSpawner.h"

#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Item.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogItemSpawner, Log, All);

namespace
{
bool SetNumericProperty(UObject* Object, FName PropertyName, double Value)
{
    FNumericProperty* Property = Object
        ? CastField<FNumericProperty>(FindFProperty<FProperty>(Object->GetClass(), PropertyName))
        : nullptr;
    if (!Property)
    {
        return false;
    }

    void* ValueAddress = Property->ContainerPtrToValuePtr<void>(Object);
    if (Property->IsFloatingPoint())
    {
        Property->SetFloatingPointPropertyValue(ValueAddress, Value);
    }
    else
    {
        Property->SetIntPropertyValue(ValueAddress, static_cast<int64>(Value));
    }
    return true;
}
}

AItemSpawner::AItemSpawner()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));
    SpawnPoint->SetupAttachment(SceneRoot);
}

void AItemSpawner::BeginPlay()
{
    Super::BeginPlay();

    if (!IsValid(Item))
    {
        UE_LOG(LogItemSpawner, Error, TEXT("%s has no configured item actor."), *GetName());
        SetActorTickEnabled(false);
        return;
    }

    if (AItem* WeaponItem = Cast<AItem>(Item))
    {
        CachedWeaponItem = WeaponItem;
        ItemType = EItemType::WeaponItem;
        UE_LOG(LogItemSpawner, Verbose, TEXT("%s uses source item %s at %s (hidden: %s)."),
            *GetName(), *WeaponItem->GetName(), *WeaponItem->GetActorLocation().ToString(),
            WeaponItem->IsHidden() ? TEXT("true") : TEXT("false"));
        if (!ItemClass)
        {
            ItemClass = WeaponItem->GetClass();
        }
    }
    else if (MedKitClass && Item->IsA(MedKitClass.Get()))
    {
        ItemType = EItemType::MedKit;
        MaxTime = MedKitRespawnTime;
    }
    else if (RepairKitClass && Item->IsA(RepairKitClass.Get()))
    {
        ItemType = EItemType::RepairKit;
        MaxTime = RepairKitRespawnTime;
    }
    else
    {
        UE_LOG(LogItemSpawner, Error, TEXT("%s has an unsupported item actor class '%s'."),
            *GetName(), *Item->GetClass()->GetName());
        SetActorTickEnabled(false);
        return;
    }

    bConfigured = true;
    SpawnedItem = SpawnItemByType(ItemType);
}

void AItemSpawner::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bConfigured || IsValid(SpawnedItem))
    {
        return;
    }

    if (Timer > MaxTime)
    {
        Timer = 0.0;
        SpawnedItem = SpawnItemByType(ItemType);
    }
    else
    {
        Timer += DeltaSeconds;
    }
}

AActor* AItemSpawner::SpawnItemByType(EItemType Type)
{
    if (!GetWorld() || !SpawnPoint)
    {
        UE_LOG(LogItemSpawner, Error, TEXT("%s cannot spawn an item without a world and spawn point."),
            *GetName());
        return nullptr;
    }

    const FTransform SpawnTransform = SpawnPoint->GetComponentTransform();

    switch (Type)
    {
    case EItemType::WeaponItem:
        return SpawnWeaponItem(SpawnTransform);
    case EItemType::MedKit:
        return SpawnMedKit(SpawnTransform);
    case EItemType::RepairKit:
        return SpawnRepairKit(SpawnTransform);
    case EItemType::Default:
    default:
        UE_LOG(LogItemSpawner, Error, TEXT("%s has unsupported item type %d."),
            *GetName(), static_cast<int32>(Type));
        return nullptr;
    }
}

AActor* AItemSpawner::SpawnWeaponItem(const FTransform& SpawnTransform)
{
    if (!ItemClass || !CachedWeaponItem)
    {
        UE_LOG(LogItemSpawner, Error, TEXT("%s cannot spawn a weapon item without a class and source item."),
            *GetName());
        return nullptr;
    }

    AItem* NewItem = GetWorld()->SpawnActorDeferred<AItem>(
        ItemClass, SpawnTransform, nullptr, nullptr,
        ESpawnActorCollisionHandlingMethod::Undefined);
    if (!NewItem)
    {
        UE_LOG(LogItemSpawner, Error, TEXT("%s failed to spawn weapon item class '%s'."),
            *GetName(), *ItemClass->GetName());
        return nullptr;
    }

    NewItem->Item = CachedWeaponItem->Item;
    NewItem->FinishSpawning(SpawnTransform);
    if (IsValid(NewItem) && NewItem->Capsule)
    {
        const UCapsuleComponent* Capsule = NewItem->Capsule;
        UE_LOG(LogItemSpawner, Verbose,
            TEXT("%s spawned %s: capsule at %s, radius %.1f, half-height %.1f, collision %d, object %d, vehicle response %d, overlap events %s."),
            *GetName(), *NewItem->GetName(), *Capsule->GetComponentLocation().ToString(),
            Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight(),
            static_cast<int32>(Capsule->GetCollisionEnabled()), static_cast<int32>(Capsule->GetCollisionObjectType()),
            static_cast<int32>(Capsule->GetCollisionResponseToChannel(ECC_Vehicle)),
            Capsule->GetGenerateOverlapEvents() ? TEXT("true") : TEXT("false"));
    }
    return NewItem;
}

AActor* AItemSpawner::SpawnMedKit(const FTransform& SpawnTransform)
{
    return SpawnConfiguredKit(MedKitClass, MedKitDamage, SpawnTransform, TEXT("med kit"));
}

AActor* AItemSpawner::SpawnRepairKit(const FTransform& SpawnTransform)
{
    return SpawnConfiguredKit(RepairKitClass, RepairKitDamage, SpawnTransform, TEXT("repair kit"));
}

AActor* AItemSpawner::SpawnConfiguredKit(TSubclassOf<AActor> KitClass, double Damage,
    const FTransform& SpawnTransform, const TCHAR* KitType)
{
    if (!KitClass)
    {
        UE_LOG(LogItemSpawner, Error, TEXT("%s cannot spawn %s without a configured class."),
            *GetName(), KitType);
        return nullptr;
    }

    AActor* NewKit = GetWorld()->SpawnActorDeferred<AActor>(
        KitClass, SpawnTransform, nullptr, nullptr,
        ESpawnActorCollisionHandlingMethod::Undefined);
    if (!NewKit)
    {
        UE_LOG(LogItemSpawner, Error, TEXT("%s failed to spawn %s class '%s'."),
            *GetName(), KitType, *KitClass->GetName());
        return nullptr;
    }

    const bool bHasSpawnWait = SetNumericProperty(NewKit, TEXT("MaxSpawnTimeWait"), HealItemMaxSpawnTimeWait);
    const bool bHasDamage = SetNumericProperty(NewKit, TEXT("Damage"), Damage);
    if (!bHasSpawnWait || !bHasDamage)
    {
        UE_LOG(LogItemSpawner, Error,
            TEXT("%s class '%s' must expose numeric MaxSpawnTimeWait and Damage properties."),
            *KitType, *KitClass->GetName());
        NewKit->Destroy();
        return nullptr;
    }

    NewKit->FinishSpawning(SpawnTransform);
    return NewKit;
}
