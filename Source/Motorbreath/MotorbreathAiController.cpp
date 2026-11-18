#include "MotorbreathAiController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "Components/SplineComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "Inventory.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NavMesh/RecastNavMesh.h"
#include "PhysicsVehicle.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"

// Behavior Tree assets still read the same-named variables declared by the Blueprint subclass.
static bool SetControllerBoolProperties(UObject* Object, FName PropertyName, bool Value)
{
    if (!Object)
    {
        return false;
    }

    bool bPropertySet = false;
    for (TFieldIterator<FProperty> It(Object->GetClass(), EFieldIteratorFlags::IncludeSuper); It; ++It)
    {
        if (It->GetFName() == PropertyName)
        {
            if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(*It))
            {
                BoolProperty->SetPropertyValue_InContainer(Object, Value);
                bPropertySet = true;
            }
        }
    }
    return bPropertySet;
}

static bool SetControllerObjectProperties(UObject* Object, FName PropertyName, UObject* Value)
{
    if (!Object)
    {
        return false;
    }

    bool bPropertySet = false;
    for (TFieldIterator<FProperty> It(Object->GetClass(), EFieldIteratorFlags::IncludeSuper); It; ++It)
    {
        if (It->GetFName() == PropertyName)
        {
            if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(*It))
            {
                ObjectProperty->SetObjectPropertyValue_InContainer(Object, Value);
                bPropertySet = true;
            }
        }
    }
    return bPropertySet;
}

AMotorbreathAiController::AMotorbreathAiController()
{
    PrimaryActorTick.bCanEverTick = true;

    static ConstructorHelpers::FClassFinder<AActor> SplineClassFinder(
        TEXT("/Game/Blueprints/SplineTools/BP_AiSpline"));
    if (SplineClassFinder.Succeeded())
    {
        AiSplineClass = SplineClassFinder.Class;
    }

    static ConstructorHelpers::FClassFinder<AActor> ItemClassFinder(
        TEXT("/Game/Blueprints/Inventory/BP_Item"));
    if (ItemClassFinder.Succeeded())
    {
        ItemClass = ItemClassFinder.Class;
    }

    static ConstructorHelpers::FClassFinder<AActor> MedKitClassFinder(
        TEXT("/Game/Blueprints/DamageActors/Heal/BP_MedKit"));
    if (MedKitClassFinder.Succeeded())
    {
        MedKitClass = MedKitClassFinder.Class;
    }

    static ConstructorHelpers::FClassFinder<AActor> RepairKitClassFinder(
        TEXT("/Game/Blueprints/DamageActors/Heal/BP_RepairKit"));
    if (RepairKitClassFinder.Succeeded())
    {
        RepairKitClass = RepairKitClassFinder.Class;
    }

    static ConstructorHelpers::FObjectFinder<UBehaviorTree> BehaviorTreeFinder(
        TEXT("/Game/AI/BT_Enemy.BT_Enemy"));
    if (BehaviorTreeFinder.Succeeded())
    {
        BehaviorTreeAsset = BehaviorTreeFinder.Object;
    }
}

void AMotorbreathAiController::BeginPlay()
{
    Super::BeginPlay();

    if (!AiSplineClass)
    {
        UE_LOG(LogTemp, Error, TEXT("%s could not load /Game/Blueprints/SplineTools/BP_AiSpline."),
            *GetName());
    }
    if (!ItemClass)
    {
        UE_LOG(LogTemp, Error, TEXT("%s could not load /Game/Blueprints/Inventory/BP_Item."),
            *GetName());
    }
    if (!MedKitClass)
    {
        UE_LOG(LogTemp, Error, TEXT("%s could not load /Game/Blueprints/DamageActors/Heal/BP_MedKit."),
            *GetName());
    }
    if (!RepairKitClass)
    {
        UE_LOG(LogTemp, Error, TEXT("%s could not load /Game/Blueprints/DamageActors/Heal/BP_RepairKit."),
            *GetName());
    }

    if (GetWorld())
    {
        GetWorldTimerManager().SetTimer(
            GetItemsTimerHandle, this, &AMotorbreathAiController::GetAllItemsInMap, 1.0f, true);
        GetWorldTimerManager().SetTimer(
            AggressivenessTimerHandle, this, &AMotorbreathAiController::DecideAggressiveness, 10.0f, true);
    }

    AActor* SplineActor = AiSplineClass
        ? UGameplayStatics::GetActorOfClass(this, AiSplineClass)
        : nullptr;
    Spline = SplineActor ? SplineActor->FindComponentByClass<USplineComponent>() : nullptr;
    SetControllerObject(GET_MEMBER_NAME_CHECKED(AMotorbreathAiController, Spline), Spline.Get());
    if (!Spline)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s could not find the BP_AiSpline spline component."),
            *GetName());
    }

    DynamicSpline = Cast<USplineComponent>(
        AddComponentByClass(USplineComponent::StaticClass(), false, FTransform::Identity, false));
    SetControllerObject(
        GET_MEMBER_NAME_CHECKED(AMotorbreathAiController, DynamicSpline), DynamicSpline.Get());
    if (!DynamicSpline)
    {
        UE_LOG(LogTemp, Error, TEXT("%s failed to create its dynamic spline component."), *GetName());
    }
    else if (Spline)
    {
        const int32 LastPointIndex = Spline->GetNumberOfSplinePoints() - 1;
        for (int32 PointIndex = 0; PointIndex <= LastPointIndex; ++PointIndex)
        {
            DynamicSpline->AddSplinePoint(
                Spline->GetLocationAtSplinePoint(PointIndex, ESplineCoordinateSpace::World),
                ESplineCoordinateSpace::World, false);
        }
        DynamicSpline->UpdateSpline();
    }

    ControlledCar = Cast<APhysicsVehicle>(GetPawn());
    SetControllerObject(
        GET_MEMBER_NAME_CHECKED(AMotorbreathAiController, ControlledCar), ControlledCar.Get());
    if (!ControlledCar)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s has no controlled PhysicsVehicle at BeginPlay."), *GetName());
    }

    AActor* NavMeshActor = UGameplayStatics::GetActorOfClass(this, ARecastNavMesh::StaticClass());
    RecastNavMesh = Cast<ARecastNavMesh>(NavMeshActor);
    SetControllerObject(
        GET_MEMBER_NAME_CHECKED(AMotorbreathAiController, RecastNavMesh), RecastNavMesh.Get());
    if (!RecastNavMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s could not find a RecastNavMesh actor."), *GetName());
    }

    if (ControlledCar && BehaviorTreeAsset)
    {
        if (!RunBehaviorTree(BehaviorTreeAsset.Get()))
        {
            UE_LOG(LogTemp, Error, TEXT("%s failed to start /Game/AI/BT_Enemy."), *GetName());
        }
    }
    else if (!BehaviorTreeAsset)
    {
        UE_LOG(LogTemp, Error, TEXT("%s could not load /Game/AI/BT_Enemy."), *GetName());
    }
}

void AMotorbreathAiController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    APhysicsVehicle* Car = Cast<APhysicsVehicle>(
        GetControllerObject(GET_MEMBER_NAME_CHECKED(AMotorbreathAiController, ControlledCar)));
    if (!Car)
    {
        return;
    }

    if (GetControllerBool(
        GET_MEMBER_NAME_CHECKED(AMotorbreathAiController, bSkipEnemyDetection), false))
    {
        Car->SetDetectedChaseTarget(false);
        UpdateRandomMovement();
        return;
    }

    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(EObjectTypeQuery::ObjectTypeQuery5);

    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(Car);

    FHitResult HitResult;
    const bool bHit = UKismetSystemLibrary::SphereTraceSingleForObjects(
        this,
        FVector::ZeroVector,
        Car->GetActorLocation(),
        10000.0f,
        ObjectTypes,
        false,
        ActorsToIgnore,
        EDrawDebugTrace::None,
        HitResult,
        true);

    if (!HitResult.GetActor())
    {
        Car->SetDetectedChaseTarget(false);
        UpdateRandomMovement();
        return;
    }

    APhysicsVehicle* DetectedVehicle = Cast<APhysicsVehicle>(HitResult.GetActor());
    if (!DetectedVehicle || DetectedVehicle->IsDead())
    {
        Car->SetDetectedChaseTarget(false);
        UpdateRandomMovement();
        return;
    }

    if (Timer > TimerRandomLimit)
    {
        TimerRandomLimit = FMath::FRandRange(MinTime, MaxTime);
        Timer = 0.0;
        if (GetControllerObject(
            GET_MEMBER_NAME_CHECKED(AMotorbreathAiController, ChaseTargetActor)) != DetectedVehicle)
        {
            ChaseTargetActor = DetectedVehicle;
            SetControllerObject(
                GET_MEMBER_NAME_CHECKED(AMotorbreathAiController, ChaseTargetActor), DetectedVehicle);
        }
    }
    else
    {
        Timer += DeltaSeconds;
    }

    Car->SetChaseActorLocation(HitResult.GetActor()->GetActorLocation());
    Car->SetDetectedChaseTarget(bHit);
}

void AMotorbreathAiController::GetAllItemsInMap()
{
    ItemsInMap.Reset();
    if (!GetWorld())
    {
        return;
    }

    auto AddItemsOfClass = [this](TSubclassOf<AActor> ActorClass, int32 Priority)
    {
        if (!ActorClass)
        {
            return;
        }

        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsOfClass(this, ActorClass, FoundActors);
        for (AActor* Actor : FoundActors)
        {
            FItemPriorityEntry& Entry = ItemsInMap.AddDefaulted_GetRef();
            Entry.Item = Actor;
            Entry.Priority = Priority;
        }
    };

    AddItemsOfClass(ItemClass, 2);

    if (CheckHp())
    {
        AddItemsOfClass(MedKitClass, 0);
    }

    if (CheckArmor())
    {
        AddItemsOfClass(RepairKitClass, 1);
    }

    TArray<FItemPriorityEntry> SortedItems;
    SortedItems.Reserve(ItemsInMap.Num());
    for (const FItemPriorityEntry& Entry : ItemsInMap)
    {
        int32 InsertIndex = 0;
        while (InsertIndex < SortedItems.Num() &&
            Entry.Priority >= SortedItems[InsertIndex].Priority)
        {
            ++InsertIndex;
        }
        SortedItems.Insert(Entry, InsertIndex);
    }
    ItemsInMap = MoveTemp(SortedItems);
    SyncItemsInMapWithBlueprintVariable();
}

void AMotorbreathAiController::DecideAggressiveness()
{
    APhysicsVehicle* Car = Cast<APhysicsVehicle>(
        GetControllerObject(GET_MEMBER_NAME_CHECKED(AMotorbreathAiController, ControlledCar)));
    if (!Car)
    {
        return;
    }

    const bool bShouldSkipDetection = FMath::FRandRange(0.0, 100.0) < 90.0;
    SetControllerBool(
        GET_MEMBER_NAME_CHECKED(AMotorbreathAiController, bSkipEnemyDetection), bShouldSkipDetection);
    if (bShouldSkipDetection)
    {
        Car->SetDetectedChaseTarget(false);
        if (UInventory* Inventory = Car->GetVehicleInventory();
            Inventory && Inventory->Weapons.Num() == 0)
        {
            SetControllerBool(
                GET_MEMBER_NAME_CHECKED(AMotorbreathAiController, bUsingRandomItemDynamicSpline), true);
        }
    }

    UKismetSystemLibrary::PrintString(
        this,
        FString::Printf(TEXT("%s <---> %s"),
            *Car->GetHumanReadableName(),
            bShouldSkipDetection ? TEXT("True") : TEXT("False")),
        true,
        true,
        Car->GetPlayerColor(),
        2.0f);
}

void AMotorbreathAiController::SyncItemsInMapWithBlueprintVariable()
{
    // BTT_CalculateRandomItemLocation consumes this Blueprint-defined struct array.
    FArrayProperty* ArrayProperty = FindFProperty<FArrayProperty>(GetClass(), TEXT("ItemsInMap"));
    FStructProperty* StructProperty = ArrayProperty
        ? CastField<FStructProperty>(ArrayProperty->Inner)
        : nullptr;
    FObjectPropertyBase* ItemProperty = StructProperty
        ? FindFProperty<FObjectPropertyBase>(StructProperty->Struct, TEXT("Item"))
        : nullptr;
    FIntProperty* PriorityProperty = StructProperty
        ? FindFProperty<FIntProperty>(StructProperty->Struct, TEXT("Priority"))
        : nullptr;

    if (!ArrayProperty || !StructProperty || !ItemProperty || !PriorityProperty)
    {
        if (!bReportedInvalidItemsInMapProperty)
        {
            UE_LOG(LogTemp, Error,
                TEXT("%s requires a Blueprint ItemsInMap array with Item and Priority fields."),
                *GetName());
            bReportedInvalidItemsInMapProperty = true;
        }
        return;
    }

    FScriptArrayHelper ArrayHelper(
        ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(this));
    ArrayHelper.EmptyValues();
    for (const FItemPriorityEntry& Entry : ItemsInMap)
    {
        void* StructValue = ArrayHelper.GetRawPtr(ArrayHelper.AddValue());
        ItemProperty->SetObjectPropertyValue_InContainer(StructValue, Entry.Item.Get());
        PriorityProperty->SetPropertyValue_InContainer(StructValue, Entry.Priority);
    }
}

void AMotorbreathAiController::UpdateRandomMovement()
{
    if (GetControllerBool(
            GET_MEMBER_NAME_CHECKED(AMotorbreathAiController, bUsingRandomPointDynamicSpline), false) ||
        GetControllerBool(
            GET_MEMBER_NAME_CHECKED(AMotorbreathAiController, bUsingRandomItemDynamicSpline), false))
    {
        return;
    }

    if (FMath::FRandRange(0.0, 100.0) >= 5.0)
    {
        SetControllerBool(
            GET_MEMBER_NAME_CHECKED(AMotorbreathAiController, bUsingRandomPointDynamicSpline), false);
        SetControllerBool(
            GET_MEMBER_NAME_CHECKED(AMotorbreathAiController, bUsingRandomItemDynamicSpline), false);
        SetControllerBool(
            GET_MEMBER_NAME_CHECKED(AMotorbreathAiController, bSkipEnemyDetection), false);
        return;
    }

    if (FMath::FRandRange(0.0, 100.0) < 10.0)
    {
        SetControllerBool(
            GET_MEMBER_NAME_CHECKED(AMotorbreathAiController, bUsingRandomPointDynamicSpline), true);
        SetControllerBool(
            GET_MEMBER_NAME_CHECKED(AMotorbreathAiController, bUsingRandomItemDynamicSpline), false);
    }
    else
    {
        SetControllerBool(
            GET_MEMBER_NAME_CHECKED(AMotorbreathAiController, bUsingRandomItemDynamicSpline), true);
        SetControllerBool(
            GET_MEMBER_NAME_CHECKED(AMotorbreathAiController, bUsingRandomPointDynamicSpline), false);
    }
    SetControllerBool(
        GET_MEMBER_NAME_CHECKED(AMotorbreathAiController, bIsCloseToDestination), true);
}

bool AMotorbreathAiController::GetControllerBool(FName PropertyName, bool DefaultValue) const
{
    for (TFieldIterator<FProperty> It(GetClass(), EFieldIteratorFlags::IncludeSuper); It; ++It)
    {
        if (It->GetFName() == PropertyName)
        {
            if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(*It))
            {
                return BoolProperty->GetPropertyValue_InContainer(this);
            }
        }
    }

    return DefaultValue;
}

void AMotorbreathAiController::SetControllerBool(FName PropertyName, bool Value)
{
    if (!SetControllerBoolProperties(this, PropertyName, Value))
    {
        UE_LOG(LogTemp, Error, TEXT("%s is missing Boolean property %s."),
            *GetName(), *PropertyName.ToString());
    }
}

UObject* AMotorbreathAiController::GetControllerObject(FName PropertyName) const
{
    for (TFieldIterator<FProperty> It(GetClass(), EFieldIteratorFlags::IncludeSuper); It; ++It)
    {
        if (It->GetFName() == PropertyName)
        {
            if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(*It))
            {
                return ObjectProperty->GetObjectPropertyValue_InContainer(this);
            }
        }
    }

    return nullptr;
}

void AMotorbreathAiController::SetControllerObject(FName PropertyName, UObject* Value)
{
    if (!SetControllerObjectProperties(this, PropertyName, Value))
    {
        UE_LOG(LogTemp, Error, TEXT("%s is missing Object property %s."),
            *GetName(), *PropertyName.ToString());
    }
}

bool AMotorbreathAiController::CheckHp() const
{
    const APhysicsVehicle* Car = Cast<APhysicsVehicle>(
        GetControllerObject(GET_MEMBER_NAME_CHECKED(AMotorbreathAiController, ControlledCar)));
    return Car &&
        Car->GetMaxHealth() > 0.0 &&
        Car->GetHealth() / Car->GetMaxHealth() < 0.35;
}

bool AMotorbreathAiController::CheckArmor() const
{
    const APhysicsVehicle* Car = Cast<APhysicsVehicle>(
        GetControllerObject(GET_MEMBER_NAME_CHECKED(AMotorbreathAiController, ControlledCar)));
    return Car &&
        Car->GetMaxArmor() > 0.0 &&
        Car->GetArmor() / Car->GetMaxArmor() < 0.2;
}
