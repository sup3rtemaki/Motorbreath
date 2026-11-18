#include "VehicleGameMode.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MotorbreathPlayerController.h"
#include "MotorbreathPlayerState.h"
#include "PhysicsVehicle.h"
#include "TimerManager.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogVehicleGameMode, Log, All);

namespace
{
    FBoolProperty* FindSpawnFlag(APlayerState* PlayerState)
    {
        FBoolProperty* Property = FindFProperty<FBoolProperty>(
            PlayerState->GetClass(), TEXT("bHasRequestedOrSpawnedPawn"));
        return Property ? Property : FindFProperty<FBoolProperty>(
            PlayerState->GetClass(), TEXT("bHasRequestedOrSpawnedPawn "));
    }

    bool InvokeControllerEvent(APlayerController* Controller, const TCHAR* Name, APawn* PawnArgument)
    {
        UFunction* Function = Controller->FindFunction(FName(Name));
        if (!Function)
        {
            const FString NameWithSpace = FString(Name) + TEXT(" ");
            Function = Controller->FindFunction(FName(*NameWithSpace));
        }
        if (!Function)
        {
            UE_LOG(LogVehicleGameMode, Error, TEXT("Controller %s has no event %s."),
                *GetNameSafe(Controller), Name);
            return false;
        }

        FStructOnScope Parameters(Function);
        if (PawnArgument)
        {
            FObjectPropertyBase* PawnParameter = nullptr;
            for (TFieldIterator<FProperty> It(Function); It; ++It)
            {
                if (It->HasAnyPropertyFlags(CPF_Parm) && !It->HasAnyPropertyFlags(CPF_ReturnParm))
                {
                    PawnParameter = CastField<FObjectPropertyBase>(*It);
                    break;
                }
            }
            if (!PawnParameter || !PawnArgument->IsA(PawnParameter->PropertyClass))
            {
                UE_LOG(LogVehicleGameMode, Error, TEXT("Event %s has no compatible pawn parameter."), Name);
                return false;
            }
            PawnParameter->SetObjectPropertyValue_InContainer(Parameters.GetStructMemory(), PawnArgument);
        }
        Controller->ProcessEvent(Function, Parameters.GetStructMemory());
        return true;
    }

    void PrintSpawnMessage(UObject* WorldContext, const FString& Message)
    {
        UKismetSystemLibrary::PrintString(
            WorldContext, Message, true, true, FLinearColor(0.0f, 0.66f, 1.0f, 1.0f), 10.0f);
    }

    FString SpawnDetails(APlayerController* Controller, APawn* Pawn)
    {
        return FString::Printf(TEXT("%s - SpawnActor: %s - Owner: %s - Controller Name: %s"),
            *UKismetSystemLibrary::GetDisplayName(Controller->GetPawn()),
            *UKismetSystemLibrary::GetDisplayName(Pawn),
            *UKismetSystemLibrary::GetDisplayName(Pawn->GetOwner()),
            *UKismetSystemLibrary::GetDisplayName(Controller));
    }
}

void AVehicleGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (AMotorbreathPlayerController* NativeController = Cast<AMotorbreathPlayerController>(NewPlayer))
    {
        NativeController->Client_NotifyReady();
    }
    else if (IsValid(NewPlayer))
    {
        InvokeControllerEvent(NewPlayer, TEXT("Client_NotifyReady"), nullptr);
    }
    else
    {
        UE_LOG(LogVehicleGameMode, Error, TEXT("Cannot notify an invalid player controller."));
    }
}

void AVehicleGameMode::Server_RequestSpawnVehicle_Implementation(APlayerController* PlayerControllerRef)
{
    if (!IsValid(PlayerControllerRef))
    {
        UE_LOG(LogVehicleGameMode, Error, TEXT("Cannot spawn a vehicle without a valid player controller."));
        return;
    }

    APlayerState* PlayerState = PlayerControllerRef->GetPlayerState<APlayerState>();
    if (!IsValid(PlayerState))
    {
        UE_LOG(LogVehicleGameMode, Error, TEXT("%s has no valid player state."), *GetNameSafe(PlayerControllerRef));
        return;
    }

    FBoolProperty* SpawnFlag = FindSpawnFlag(PlayerState);
    FClassProperty* SelectedClassProperty = FindFProperty<FClassProperty>(
        PlayerState->GetClass(), TEXT("SelectedVehicleClass"));
    if (!SpawnFlag || !SelectedClassProperty)
    {
        UE_LOG(LogVehicleGameMode, Error, TEXT("%s is missing vehicle spawn properties."), *GetNameSafe(PlayerState));
        return;
    }

    if (SpawnFlag->GetPropertyValue_InContainer(PlayerState))
    {
        PrintSpawnMessage(this, FString::Printf(
            TEXT("[Vehicle_GameMode] Skipping spawn, already spawned for PC:%s"),
            *UKismetSystemLibrary::GetDisplayName(PlayerControllerRef)));
        return;
    }
    SpawnFlag->SetPropertyValue_InContainer(PlayerState, true);

    PrintSpawnMessage(this, FString::Printf(TEXT("[Vehicle_GameMode] Begin RequestSpawn - PC = %sPS = %s"),
        *UKismetSystemLibrary::GetDisplayName(PlayerControllerRef),
        *UKismetSystemLibrary::GetDisplayName(PlayerState)));

    UClass* SelectedClass = Cast<UClass>(SelectedClassProperty->GetObjectPropertyValue_InContainer(PlayerState));
    const bool bHasSelectedClass = IsValid(SelectedClass);
    UClass* SpawnClass = bHasSelectedClass ? SelectedClass : DefaultPawnClass.Get();
    if (!SpawnClass || !SpawnClass->IsChildOf(APawn::StaticClass()))
    {
        UE_LOG(LogVehicleGameMode, Error, TEXT("Invalid pawn class %s for %s."),
            *GetNameSafe(SpawnClass), *GetNameSafe(PlayerControllerRef));
        return;
    }

    if (bHasSelectedClass)
    {
        PrintSpawnMessage(this, FString::Printf(TEXT("[Vehicle_GameMode] SPAWNING: %s"),
            *UKismetSystemLibrary::GetClassDisplayName(SpawnClass)));
    }
    else
    {
        PrintSpawnMessage(this, TEXT("[BP_VehicleGameMode]  SPAWNING DEFAULT"));
    }

    AActor* Start = K2_FindPlayerStart(nullptr, TEXT(""));
    if (!IsValid(Start))
    {
        UE_LOG(LogVehicleGameMode, Error, TEXT("Could not find a player start for %s."), *GetNameSafe(PlayerControllerRef));
        return;
    }

    FActorSpawnParameters SpawnParameters;
    if (bHasSelectedClass)
    {
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    }
    APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(SpawnClass, Start->GetActorTransform(), SpawnParameters);
    if (!IsValid(SpawnedPawn))
    {
        UE_LOG(LogVehicleGameMode, Error, TEXT("Failed to spawn pawn class %s."), *GetNameSafe(SpawnClass));
        return;
    }

    if (!bHasSelectedClass)
    {
        PlayerControllerRef->Possess(SpawnedPawn);
        return;
    }

    if (!SpawnedPawn->IsA<APhysicsVehicle>())
    {
        UClass* LegacyVehicleClass = LoadClass<APawn>(
            nullptr, TEXT("/Game/Blueprints/PhysicsVehicles/BP_PhysicsVehicle.BP_PhysicsVehicle_C"));
        if (!LegacyVehicleClass || !SpawnedPawn->IsA(LegacyVehicleClass))
        {
            UE_LOG(LogVehicleGameMode, Error, TEXT("Spawned pawn %s is not a physics vehicle."), *GetNameSafe(SpawnedPawn));
            return;
        }
    }

    SpawnedPawn->SetOwner(PlayerControllerRef);
    SpawnedPawn->ForceNetUpdate();
    PlayerControllerRef->ForceNetUpdate();
    PrintSpawnMessage(this, TEXT("[Vehicle_GameMode] PlayerController Controlled pawn: ") +
        SpawnDetails(PlayerControllerRef, SpawnedPawn));
    PlayerControllerRef->Possess(SpawnedPawn);
    SpawnedPawn->EnableInput(PlayerControllerRef);
    PrintSpawnMessage(this, TEXT("[Vehicle_GameMode] PlayerController Controlled pawn after Possess: ") +
        SpawnDetails(PlayerControllerRef, SpawnedPawn));

    const TWeakObjectPtr<APlayerController> WeakController = PlayerControllerRef;
    const TWeakObjectPtr<APawn> WeakPawn = SpawnedPawn;
    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateWeakLambda(this,
        [this, WeakController, WeakPawn]()
        {
            ConfirmVehiclePossession(WeakController.Get(), WeakPawn.Get());
        }), 0.3f, false);
}

void AVehicleGameMode::ConfirmVehiclePossession(APlayerController* PlayerController, APawn* SpawnedPawn)
{
    if (!IsValid(PlayerController) || !IsValid(SpawnedPawn))
    {
        UE_LOG(LogVehicleGameMode, Error, TEXT("Cannot confirm possession: controller or pawn is no longer valid."));
        return;
    }

    if (AMotorbreathPlayerController* NativeController = Cast<AMotorbreathPlayerController>(PlayerController))
    {
        NativeController->Client_ConfirmPossession(SpawnedPawn);
    }
    else
    {
        InvokeControllerEvent(PlayerController, TEXT("Client_ConfirmPossession"), SpawnedPawn);
    }
}
