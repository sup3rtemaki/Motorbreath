#include "MotorbreathPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Inventory.h"
#include "PhysicsVehicle.h"
#include "MotorbreathPlayerState.h"
#include "WeaponStruct.h"
#include "Blueprint/UserWidget.h"
#include "Components/ChildActorComponent.h"
#include "Components/InputComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerState.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogMotorbreathPlayerController, Log, All);

namespace
{
    FString NormalizeReflectionName(const FString& Name)
    {
        FString Normalized;
        Normalized.Reserve(Name.Len());
        for (const TCHAR Character : Name)
        {
            if (FChar::IsAlnum(Character))
            {
                Normalized.AppendChar(FChar::ToLower(Character));
            }
        }
        return Normalized;
    }

    UFunction* FindBlueprintFunction(UObject* Target, const TCHAR* FunctionName)
    {
        if (!Target)
        {
            return nullptr;
        }

        const FString ExpectedName = NormalizeReflectionName(FunctionName);
        for (TFieldIterator<UFunction> FunctionIt(Target->GetClass(), EFieldIteratorFlags::IncludeSuper);
            FunctionIt; ++FunctionIt)
        {
            if (NormalizeReflectionName(FunctionIt->GetName()) == ExpectedName)
            {
                return *FunctionIt;
            }
        }
        return nullptr;
    }

    FProperty* FindReflectedProperty(UStruct* Struct, const TCHAR* PropertyName)
    {
        if (!Struct)
        {
            return nullptr;
        }

        const FString ExpectedName = NormalizeReflectionName(PropertyName);
        for (TFieldIterator<FProperty> PropertyIt(Struct, EFieldIteratorFlags::IncludeSuper);
            PropertyIt; ++PropertyIt)
        {
            if (NormalizeReflectionName(PropertyIt->GetName()) == ExpectedName)
            {
                return *PropertyIt;
            }
        }
        return nullptr;
    }

    FProperty* FindFunctionParameter(UFunction* Function, const TCHAR* ParameterName)
    {
        FProperty* NamedParameter = FindReflectedProperty(Function, ParameterName);
        if (NamedParameter && NamedParameter->HasAnyPropertyFlags(CPF_Parm)
            && !NamedParameter->HasAnyPropertyFlags(CPF_ReturnParm))
        {
            return NamedParameter;
        }
        return nullptr;
    }

    bool InvokeBlueprintFunction(UObject* Target, const TCHAR* FunctionName)
    {
        UFunction* Function = FindBlueprintFunction(Target, FunctionName);
        if (!Function)
        {
            UE_LOG(LogMotorbreathPlayerController, Error,
                TEXT("Blueprint function %s was not found on %s."),
                FunctionName, Target ? *Target->GetClass()->GetName() : TEXT("null"));
            return false;
        }

        FStructOnScope Parameters(Function);
        Target->ProcessEvent(Function, Parameters.GetStructMemory());
        return true;
    }

    bool InvokeBlueprintFunctionWithPercent(UObject* Target, const TCHAR* FunctionName, double Percent)
    {
        UFunction* Function = FindBlueprintFunction(Target, FunctionName);
        if (!Function)
        {
            UE_LOG(LogMotorbreathPlayerController, Error,
                TEXT("Blueprint function %s was not found on %s."),
                FunctionName, Target ? *Target->GetClass()->GetName() : TEXT("null"));
            return false;
        }

        FProperty* Parameter = FindFunctionParameter(Function, TEXT("NewPercent"));
        FNumericProperty* NumericParameter = CastField<FNumericProperty>(Parameter);
        if (!NumericParameter)
        {
            UE_LOG(LogMotorbreathPlayerController, Error,
                TEXT("Blueprint function %s has no numeric NewPercent parameter."), FunctionName);
            return false;
        }

        FStructOnScope Parameters(Function);
        void* ParameterValue = NumericParameter->ContainerPtrToValuePtr<void>(Parameters.GetStructMemory());
        if (NumericParameter->IsFloatingPoint())
        {
            NumericParameter->SetFloatingPointPropertyValue(ParameterValue, Percent);
        }
        else if (NumericParameter->IsInteger())
        {
            NumericParameter->SetIntPropertyValue(ParameterValue, static_cast<int64>(Percent));
        }
        else
        {
            UE_LOG(LogMotorbreathPlayerController, Error,
                TEXT("Blueprint function %s has an unsupported NewPercent type."), FunctionName);
            return false;
        }

        Target->ProcessEvent(Function, Parameters.GetStructMemory());
        return true;
    }

    bool InvokeBlueprintFunctionWithWeapon(UObject* Target, const TCHAR* FunctionName,
        const FWeaponStruct& Weapon)
    {
        UFunction* Function = FindBlueprintFunction(Target, FunctionName);
        if (!Function)
        {
            UE_LOG(LogMotorbreathPlayerController, Error,
                TEXT("Blueprint function %s was not found on %s."),
                FunctionName, Target ? *Target->GetClass()->GetName() : TEXT("null"));
            return false;
        }

        FStructProperty* WeaponParameter = CastField<FStructProperty>(
            FindFunctionParameter(Function, TEXT("NewWeapon")));
        if (!WeaponParameter)
        {
            UE_LOG(LogMotorbreathPlayerController, Error,
                TEXT("Blueprint function %s has no struct NewWeapon parameter."), FunctionName);
            return false;
        }

        FStructOnScope Parameters(Function);
        void* DestinationStruct = WeaponParameter->ContainerPtrToValuePtr<void>(Parameters.GetStructMemory());
        const UScriptStruct* SourceStruct = FWeaponStruct::StaticStruct();
        int32 CopiedFieldCount = 0;
        for (TFieldIterator<FProperty> DestinationIt(WeaponParameter->Struct);
            DestinationIt; ++DestinationIt)
        {
            FProperty* DestinationProperty = *DestinationIt;
            FProperty* SourceProperty = FindFProperty<FProperty>(SourceStruct, DestinationProperty->GetFName());
            if (!SourceProperty || !DestinationProperty->SameType(SourceProperty))
            {
                UE_LOG(LogMotorbreathPlayerController, Error,
                    TEXT("Cannot transfer weapon field %s to Blueprint struct %s."),
                    *DestinationProperty->GetName(), *WeaponParameter->Struct->GetName());
                continue;
            }

            void* DestinationValue = DestinationProperty->ContainerPtrToValuePtr<void>(DestinationStruct);
            const void* SourceValue = SourceProperty->ContainerPtrToValuePtr<void>(&Weapon);
            DestinationProperty->CopyCompleteValue(DestinationValue, SourceValue);
            ++CopiedFieldCount;
        }

        if (CopiedFieldCount == 0)
        {
            UE_LOG(LogMotorbreathPlayerController, Error,
                TEXT("No compatible fields were found while transferring weapon data to %s."),
                *WeaponParameter->Struct->GetName());
            return false;
        }

        Target->ProcessEvent(Function, Parameters.GetStructMemory());
        return true;
    }

    UObject* GetObjectPropertyValue(UObject* Target, const TCHAR* PropertyName)
    {
        FObjectPropertyBase* Property = CastField<FObjectPropertyBase>(
            FindReflectedProperty(Target ? Target->GetClass() : nullptr, PropertyName));
        return Property ? Property->GetObjectPropertyValue_InContainer(Target) : nullptr;
    }

    bool SetObjectPropertyValue(UObject* Target, const TCHAR* PropertyName, UObject* Value)
    {
        FObjectPropertyBase* Property = CastField<FObjectPropertyBase>(
            FindReflectedProperty(Target ? Target->GetClass() : nullptr, PropertyName));
        if (!Property)
        {
            UE_LOG(LogMotorbreathPlayerController, Error,
                TEXT("Object property %s was not found on %s."),
                PropertyName, Target ? *Target->GetClass()->GetName() : TEXT("null"));
            return false;
        }
        if (Value && !Value->IsA(Property->PropertyClass))
        {
            UE_LOG(LogMotorbreathPlayerController, Error,
                TEXT("Object %s is incompatible with property %s on %s."),
                *Value->GetClass()->GetName(), PropertyName, *Target->GetClass()->GetName());
            return false;
        }

        Property->SetObjectPropertyValue_InContainer(Target, Value);
        return true;
    }

    bool SetClassPropertyValue(UObject* Target, const TCHAR* PropertyName, UClass* Value)
    {
        FClassProperty* Property = CastField<FClassProperty>(
            FindReflectedProperty(Target ? Target->GetClass() : nullptr, PropertyName));
        if (!Property)
        {
            UE_LOG(LogMotorbreathPlayerController, Error,
                TEXT("Class property %s was not found on %s."),
                PropertyName, Target ? *Target->GetClass()->GetName() : TEXT("null"));
            return false;
        }
        if (Value && !Value->IsChildOf(Property->MetaClass))
        {
            UE_LOG(LogMotorbreathPlayerController, Error,
                TEXT("Class %s is incompatible with property %s on %s."),
                *Value->GetName(), PropertyName, *Target->GetClass()->GetName());
            return false;
        }

        Property->SetObjectPropertyValue_InContainer(Target, Value);
        return true;
    }

    UClass* GetClassPropertyValue(UObject* Target, const TCHAR* PropertyName)
    {
        FClassProperty* Property = CastField<FClassProperty>(
            FindReflectedProperty(Target ? Target->GetClass() : nullptr, PropertyName));
        return Property ? Cast<UClass>(Property->GetObjectPropertyValue_InContainer(Target)) : nullptr;
    }

    bool SetBoolPropertyValue(UObject* Target, const TCHAR* PropertyName, bool Value)
    {
        FBoolProperty* Property = CastField<FBoolProperty>(
            FindReflectedProperty(Target ? Target->GetClass() : nullptr, PropertyName));
        if (!Property)
        {
            UE_LOG(LogMotorbreathPlayerController, Warning,
                TEXT("Boolean property %s was not found on %s."),
                PropertyName, Target ? *Target->GetClass()->GetName() : TEXT("null"));
            return false;
        }

        Property->SetPropertyValue_InContainer(Target, Value);
        return true;
    }

    bool InvokeBlueprintFunctionWithObject(UObject* Target, const TCHAR* FunctionName, UObject* Object)
    {
        UFunction* Function = FindBlueprintFunction(Target, FunctionName);
        if (!Function)
        {
            UE_LOG(LogMotorbreathPlayerController, Error,
                TEXT("Blueprint function %s was not found on %s."),
                FunctionName, Target ? *Target->GetClass()->GetName() : TEXT("null"));
            return false;
        }

        FProperty* Parameter = nullptr;
        for (TFieldIterator<FProperty> PropertyIt(Function); PropertyIt; ++PropertyIt)
        {
            if (PropertyIt->HasAnyPropertyFlags(CPF_Parm | CPF_OutParm)
                && !PropertyIt->HasAnyPropertyFlags(CPF_ReturnParm))
            {
                Parameter = *PropertyIt;
                break;
            }
        }

        FObjectPropertyBase* ObjectParameter = CastField<FObjectPropertyBase>(Parameter);
        if (!ObjectParameter || (Object && !Object->IsA(ObjectParameter->PropertyClass)))
        {
            UE_LOG(LogMotorbreathPlayerController, Error,
                TEXT("Blueprint function %s has no compatible object parameter."), FunctionName);
            return false;
        }

        FStructOnScope Parameters(Function);
        ObjectParameter->SetObjectPropertyValue_InContainer(Parameters.GetStructMemory(), Object);
        Target->ProcessEvent(Function, Parameters.GetStructMemory());
        return true;
    }
}

AMotorbreathPlayerController::AMotorbreathPlayerController()
{
    PrimaryActorTick.bCanEverTick = true;

    static ConstructorHelpers::FObjectFinder<UInputAction> DriveFinder(TEXT("/Game/Input/IA_Drive.IA_Drive"));
    static ConstructorHelpers::FObjectFinder<UInputAction> TurnFinder(TEXT("/Game/Input/IA_Turn.IA_Turn"));
    static ConstructorHelpers::FObjectFinder<UInputAction> FireFinder(TEXT("/Game/Input/IA_Fire.IA_Fire"));
    static ConstructorHelpers::FObjectFinder<UInputAction> HandbrakeFinder(TEXT("/Game/Input/IA_Handbrake.IA_Handbrake"));
    static ConstructorHelpers::FObjectFinder<UInputAction> SelectedWeaponFinder(
        TEXT("/Game/Input/IA_FireSelectedWeapon.IA_FireSelectedWeapon"));
    static ConstructorHelpers::FObjectFinder<UInputAction> CycleAscendingFinder(
        TEXT("/Game/Input/IA_CycleInventoryAsc.IA_CycleInventoryAsc"));
    static ConstructorHelpers::FObjectFinder<UInputAction> CycleDescendingFinder(
        TEXT("/Game/Input/IA_CycleInventoryDesc.IA_CycleInventoryDesc"));
    static ConstructorHelpers::FObjectFinder<UInputMappingContext> CarMappingFinder(
        TEXT("/Game/Input/IMC_Car.IMC_Car"));
    static ConstructorHelpers::FClassFinder<UUserWidget> HUDClassFinder(TEXT("/Game/UI/WB_InGameHUD"));

    if (DriveFinder.Succeeded())
    {
        DriveAction = DriveFinder.Object;
    }
    if (TurnFinder.Succeeded())
    {
        TurnAction = TurnFinder.Object;
    }
    if (FireFinder.Succeeded())
    {
        FireAction = FireFinder.Object;
    }
    if (HandbrakeFinder.Succeeded())
    {
        HandbrakeAction = HandbrakeFinder.Object;
    }
    if (SelectedWeaponFinder.Succeeded())
    {
        FireSelectedWeaponAction = SelectedWeaponFinder.Object;
    }
    if (CycleAscendingFinder.Succeeded())
    {
        CycleInventoryAscendingAction = CycleAscendingFinder.Object;
    }
    if (CycleDescendingFinder.Succeeded())
    {
        CycleInventoryDescendingAction = CycleDescendingFinder.Object;
    }
    if (CarMappingFinder.Succeeded())
    {
        CarMappingContext = CarMappingFinder.Object;
    }
    if (HUDClassFinder.Succeeded())
    {
        InGameHUDClass = HUDClassFinder.Class;
    }
}

void AMotorbreathPlayerController::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogMotorbreathPlayerController, Log, TEXT("Hello"));
}

void AMotorbreathPlayerController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!IsValid(PhysicsVehicle) || !IsValid(InGameHUD))
    {
        return;
    }

    InvokeBlueprintFunctionWithPercent(InGameHUD, TEXT("Event Change HealthBar Percent"),
        PhysicsVehicle->GetHealth() / 100.0);
    InvokeBlueprintFunctionWithPercent(InGameHUD, TEXT("Event Change ArmorBar Percent"),
        PhysicsVehicle->GetArmor() / 100.0);
}

void AMotorbreathPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    if (!InputComponent)
    {
        UE_LOG(LogMotorbreathPlayerController, Error, TEXT("Input component is unavailable."));
        return;
    }

    InputComponent->BindAction(TEXT("ToggleCamera"), IE_Pressed, this,
        &AMotorbreathPlayerController::HandleToggleCamera);

    UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
    if (!EnhancedInput)
    {
        UE_LOG(LogMotorbreathPlayerController, Error,
            TEXT("Player controller input component is not an Enhanced Input component."));
        return;
    }

    if (DriveAction)
    {
        EnhancedInput->BindAction(DriveAction, ETriggerEvent::Triggered, this,
            &AMotorbreathPlayerController::HandleDrive);
        EnhancedInput->BindAction(DriveAction, ETriggerEvent::Completed, this,
            &AMotorbreathPlayerController::HandleDriveCompleted);
    }
    if (TurnAction)
    {
        EnhancedInput->BindAction(TurnAction, ETriggerEvent::Triggered, this,
            &AMotorbreathPlayerController::HandleTurn);
        EnhancedInput->BindAction(TurnAction, ETriggerEvent::Completed, this,
            &AMotorbreathPlayerController::HandleTurnCompleted);
    }
    if (FireAction)
    {
        EnhancedInput->BindAction(FireAction, ETriggerEvent::Triggered, this,
            &AMotorbreathPlayerController::HandleFire);
    }
    if (HandbrakeAction)
    {
        EnhancedInput->BindAction(HandbrakeAction, ETriggerEvent::Triggered, this,
            &AMotorbreathPlayerController::HandleHandbrake);
        EnhancedInput->BindAction(HandbrakeAction, ETriggerEvent::Ongoing, this,
            &AMotorbreathPlayerController::HandleHandbrake);
        EnhancedInput->BindAction(HandbrakeAction, ETriggerEvent::Completed, this,
            &AMotorbreathPlayerController::HandleHandbrakeCompleted);
    }
    if (FireSelectedWeaponAction)
    {
        EnhancedInput->BindAction(FireSelectedWeaponAction, ETriggerEvent::Triggered, this,
            &AMotorbreathPlayerController::HandleFireSelectedWeapon);
    }
    if (CycleInventoryAscendingAction)
    {
        EnhancedInput->BindAction(CycleInventoryAscendingAction, ETriggerEvent::Started, this,
            &AMotorbreathPlayerController::HandleCycleInventoryAscending);
    }
    if (CycleInventoryDescendingAction)
    {
        EnhancedInput->BindAction(CycleInventoryDescendingAction, ETriggerEvent::Started, this,
            &AMotorbreathPlayerController::HandleCycleInventoryDescending);
    }
}

void AMotorbreathPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    UE_LOG(LogMotorbreathPlayerController, Log, TEXT("Hello - OnPossess: %s; server=%s; local=%s"),
        *GetNameSafe(InPawn), HasAuthority() ? TEXT("true") : TEXT("false"),
        IsLocalController() ? TEXT("true") : TEXT("false"));

    UE_LOG(LogMotorbreathPlayerController, Log, TEXT("%s"),
        IsLocalController() ? TEXT("IS LOCAL CONTROLLER") : TEXT("NOT LOCAL CONTROLLER"));
}

void AMotorbreathPlayerController::HandleDrive(const FInputActionValue& Value)
{
    if (APhysicsVehicle* Vehicle = GetControlledVehicle())
    {
        Vehicle->SetVehicleAcceleration(Value.Get<float>());
    }
}

void AMotorbreathPlayerController::HandleDriveCompleted(const FInputActionValue&)
{
    if (APhysicsVehicle* Vehicle = GetControlledVehicle())
    {
        Vehicle->SetVehicleAcceleration(0.0);
    }
}

void AMotorbreathPlayerController::HandleTurn(const FInputActionValue& Value)
{
    if (APhysicsVehicle* Vehicle = GetControlledVehicle())
    {
        Vehicle->Server_SetSteeringAngle(Value.Get<float>(), true);
    }
}

void AMotorbreathPlayerController::HandleTurnCompleted(const FInputActionValue&)
{
    if (APhysicsVehicle* Vehicle = GetControlledVehicle())
    {
        Vehicle->Server_SetSteeringAngle(0.0, false);
    }
}

void AMotorbreathPlayerController::HandleFire(const FInputActionValue&)
{
    if (APhysicsVehicle* Vehicle = GetControlledVehicle())
    {
        Vehicle->RequestMachineGunFire();
    }
}

void AMotorbreathPlayerController::HandleHandbrake(const FInputActionValue& Value)
{
    if (APhysicsVehicle* Vehicle = GetControlledVehicle())
    {
        Vehicle->SetVehicleBrake(Value.Get<float>(), true);
    }
}

void AMotorbreathPlayerController::HandleHandbrakeCompleted(const FInputActionValue&)
{
    if (APhysicsVehicle* Vehicle = GetControlledVehicle())
    {
        Vehicle->SetVehicleBrake(0.0, false);
    }
}

void AMotorbreathPlayerController::HandleFireSelectedWeapon(const FInputActionValue&)
{
    if (APhysicsVehicle* Vehicle = GetControlledVehicle())
    {
        Vehicle->RequestSelectedWeaponFire();
    }
}

void AMotorbreathPlayerController::HandleCycleInventoryAscending(const FInputActionValue&)
{
    if (APhysicsVehicle* Vehicle = GetControlledVehicle())
    {
        Vehicle->CycleInventoryAscending();
    }
}

void AMotorbreathPlayerController::HandleCycleInventoryDescending(const FInputActionValue&)
{
    if (APhysicsVehicle* Vehicle = GetControlledVehicle())
    {
        Vehicle->CycleInventoryDescending();
    }
}

void AMotorbreathPlayerController::HandleToggleCamera()
{
    if (APhysicsVehicle* Vehicle = GetControlledVehicle())
    {
        Vehicle->ToggleVehicleCamera();
    }
}

void AMotorbreathPlayerController::HandleWeaponChanged()
{
    if (!IsValid(InGameHUD) || !IsValid(PhysicsVehicleInventory))
    {
        return;
    }

    if (PhysicsVehicleInventory->Weapons.IsEmpty())
    {
        InvokeBlueprintFunction(InGameHUD, TEXT("Event Empty Inventory"));
        return;
    }

    if (!PhysicsVehicleInventory->Weapons.IsValidIndex(PhysicsVehicleInventory->InventoryIndex))
    {
        UE_LOG(LogMotorbreathPlayerController, Error,
            TEXT("Inventory index %d is invalid for %d weapons."),
            PhysicsVehicleInventory->InventoryIndex, PhysicsVehicleInventory->Weapons.Num());
        return;
    }

    InvokeBlueprintFunctionWithWeapon(InGameHUD, TEXT("Event Change Weapon"),
        PhysicsVehicleInventory->Weapons[PhysicsVehicleInventory->InventoryIndex]);
}

void AMotorbreathPlayerController::HandleCurrentWeaponUpdated()
{
    if (IsValid(InGameHUD))
    {
        InvokeBlueprintFunction(InGameHUD, TEXT("Event Update Current Weapon Quantity"));
    }
}

void AMotorbreathPlayerController::TryConfirmPossession()
{
    if (!IsValid(PossessedPawn))
    {
        if (!GetWorld())
        {
            UE_LOG(LogMotorbreathPlayerController, Error,
                TEXT("Cannot retry possession confirmation without a world."));
            return;
        }
        GetWorld()->GetTimerManager().SetTimer(ConfirmPossessionRetryHandle, this,
            &AMotorbreathPlayerController::TryConfirmPossession, 0.2f, false);
        return;
    }

    GetWorld()->GetTimerManager().ClearTimer(ConfirmPossessionRetryHandle);
    SetInputMode(FInputModeGameOnly());
    SetViewTargetWithBlend(PossessedPawn, 0.0f);
    PossessedPawn->EnableInput(this);

    EnsureHUD();
    AddCarMappingContext(true);

    PhysicsVehicle = Cast<APhysicsVehicle>(PossessedPawn);
    if (!PhysicsVehicle)
    {
        PhysicsVehicleInventory = nullptr;
        UE_LOG(LogMotorbreathPlayerController, Warning,
            TEXT("Possessed pawn %s is not an APhysicsVehicle."), *GetNameSafe(PossessedPawn));
        return;
    }

    SetBoolPropertyValue(PhysicsVehicle, TEXT("bControlledLocally"), true);
    PhysicsVehicleInventory = PhysicsVehicle->GetVehicleInventory();
    BindInventoryDelegates();
    UpdateRadarReferences();
}

void AMotorbreathPlayerController::EnsureHUD()
{
    if (IsValid(InGameHUD))
    {
        return;
    }
    if (!InGameHUDClass)
    {
        UE_LOG(LogMotorbreathPlayerController, Error,
            TEXT("In-game HUD class is not configured."));
        return;
    }

    InGameHUD = CreateWidget<UUserWidget>(this, InGameHUDClass);
    if (!InGameHUD)
    {
        UE_LOG(LogMotorbreathPlayerController, Error,
            TEXT("Failed to create in-game HUD widget %s."), *GetNameSafe(InGameHUDClass.Get()));
        return;
    }
    InGameHUD->AddToViewport();
}

void AMotorbreathPlayerController::AddCarMappingContext(bool bClearExistingMappings)
{
    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer
        ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()
        : nullptr;
    if (!InputSubsystem)
    {
        UE_LOG(LogMotorbreathPlayerController, Error,
            TEXT("Enhanced Input local-player subsystem is unavailable."));
        return;
    }
    if (!CarMappingContext)
    {
        UE_LOG(LogMotorbreathPlayerController, Error, TEXT("Car input mapping context is not configured."));
        return;
    }

    if (bClearExistingMappings)
    {
        InputSubsystem->ClearAllMappings();
    }
    InputSubsystem->AddMappingContext(CarMappingContext, 0);
}

void AMotorbreathPlayerController::BindInventoryDelegates()
{
    if (!IsValid(PhysicsVehicleInventory))
    {
        UE_LOG(LogMotorbreathPlayerController, Warning,
            TEXT("The possessed vehicle has no inventory component."));
        return;
    }

    PhysicsVehicleInventory->WeaponChanged.AddUniqueDynamic(this,
        &AMotorbreathPlayerController::HandleWeaponChanged);
    PhysicsVehicleInventory->UpdateCurrentWeapon.AddUniqueDynamic(this,
        &AMotorbreathPlayerController::HandleCurrentWeaponUpdated);
}

void AMotorbreathPlayerController::UpdateRadarReferences()
{
    if (!IsValid(PhysicsVehicle) || !IsValid(InGameHUD))
    {
        return;
    }

    UChildActorComponent* RadarComponent = Cast<UChildActorComponent>(
        GetObjectPropertyValue(PhysicsVehicle, TEXT("BP_RadarManager")));
    AActor* RadarActor = RadarComponent ? RadarComponent->GetChildActor() : nullptr;
    if (!RadarActor)
    {
        UE_LOG(LogMotorbreathPlayerController, Warning,
            TEXT("The vehicle radar child actor is unavailable."));
        return;
    }
    RadarManager = RadarActor;

    UObject* RadarWidget = GetObjectPropertyValue(InGameHUD, TEXT("UW_Radar"));
    if (!RadarWidget)
    {
        UE_LOG(LogMotorbreathPlayerController, Error,
            TEXT("HUD property UW_Radar is missing or unset."));
        return;
    }

    SetObjectPropertyValue(RadarWidget, TEXT("RadarManager"), RadarManager);
}

APhysicsVehicle* AMotorbreathPlayerController::GetControlledVehicle() const
{
    if (IsValid(PhysicsVehicle))
    {
        return PhysicsVehicle;
    }
    return nullptr;
}

void AMotorbreathPlayerController::Client_NotifyReady_Implementation()
{
    UGameInstance* GameInstance = GetGameInstance();
    UClass* SelectedClass = GetClassPropertyValue(GameInstance, TEXT("SelectedVehicleClass"));
    if (!SelectedClass)
    {
        UE_LOG(LogMotorbreathPlayerController, Error,
            TEXT("SelectedVehicleClass is missing or unset on the game instance."));
        return;
    }

    Server_SetSelectedVehicleClass(SelectedClass);
}

void AMotorbreathPlayerController::Server_SetSelectedVehicleClass_Implementation(
    TSubclassOf<APawn> VehicleClass)
{
    APlayerState* CurrentPlayerState = PlayerState;
    if (!CurrentPlayerState)
    {
        UE_LOG(LogMotorbreathPlayerController, Error,
            TEXT("Cannot set the selected vehicle class without a player state."));
        return;
    }
    if (AMotorbreathPlayerState* NativePlayerState = Cast<AMotorbreathPlayerState>(CurrentPlayerState))
    {
        if (!NativePlayerState->SetSelectedVehicleClass(VehicleClass))
        {
            return;
        }
    }
    else
    {
        if (!SetClassPropertyValue(CurrentPlayerState, TEXT("SelectedVehicleClass"), VehicleClass.Get()))
        {
            return;
        }
        CurrentPlayerState->ForceNetUpdate();
    }

    AGameModeBase* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode() : nullptr;
    if (!GameMode)
    {
        UE_LOG(LogMotorbreathPlayerController, Error,
            TEXT("Vehicle_GameMode is unavailable on the server."));
        return;
    }
    InvokeBlueprintFunctionWithObject(GameMode, TEXT("Server_RequestSpawnVehicle"), this);
}

void AMotorbreathPlayerController::Client_ConfirmPossession_Implementation(APawn* InPawn)
{
    PossessedPawn = InPawn;
    TryConfirmPossession();
}

void AMotorbreathPlayerController::Client_CreateHUDAndMappingContext_Implementation()
{
    InGameHUD = nullptr;
    EnsureHUD();
    AddCarMappingContext(false);
}
