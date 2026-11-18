#include "MotorbreathPlayerState.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogMotorbreathPlayerState, Log, All);

static bool SetSelectedVehicleClassProperties(UObject* Object, FName PropertyName, UClass* Value)
{
    if (!Object)
    {
        return false;
    }

    bool bPropertySet = false;
    for (TFieldIterator<FProperty> It(Object->GetClass(), EFieldIteratorFlags::IncludeSuper); It; ++It)
    {
        if (It->GetFName() != PropertyName)
        {
            continue;
        }

        FClassProperty* ClassProperty = CastField<FClassProperty>(*It);
        if (!ClassProperty)
        {
            continue;
        }
        if (Value && !Value->IsChildOf(ClassProperty->MetaClass))
        {
            UE_LOG(LogMotorbreathPlayerState, Error,
                TEXT("Class %s is incompatible with %s on %s."),
                *Value->GetName(), *PropertyName.ToString(), *Object->GetClass()->GetName());
            continue;
        }

        ClassProperty->SetObjectPropertyValue_InContainer(Object, Value);
        bPropertySet = true;
    }

    return bPropertySet;
}

TSubclassOf<APawn> AMotorbreathPlayerState::GetSelectedVehicleClass() const
{
    return SelectedVehicleClass;
}

bool AMotorbreathPlayerState::SetSelectedVehicleClass(TSubclassOf<APawn> VehicleClass)
{
    if (!HasAuthority())
    {
        UE_LOG(LogMotorbreathPlayerState, Error,
            TEXT("SelectedVehicleClass can only be changed by the server."));
        return false;
    }

    SelectedVehicleClass = VehicleClass;
    if (!SetSelectedVehicleClassProperties(
        this, GET_MEMBER_NAME_CHECKED(AMotorbreathPlayerState, SelectedVehicleClass), VehicleClass.Get()))
    {
        UE_LOG(LogMotorbreathPlayerState, Error,
            TEXT("%s is missing the SelectedVehicleClass class property."), *GetName());
        return false;
    }

    ForceNetUpdate();
    return true;
}

void AMotorbreathPlayerState::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AMotorbreathPlayerState, SelectedVehicleClass);
}

void AMotorbreathPlayerState::OnRep_SelectedVehicleClass()
{
    SyncSelectedVehicleClassProperties();

    const FString ClassDisplayName = SelectedVehicleClass
        ? UKismetSystemLibrary::GetClassDisplayName(SelectedVehicleClass.Get())
        : TEXT("None");
    UKismetSystemLibrary::PrintString(
        this,
        FString::Printf(TEXT("[BP_PlayerState] PLAYERSTATE: %s"), *ClassDisplayName),
        true,
        true,
        FLinearColor(0.0f, 0.66f, 1.0f, 1.0f),
        10.0f);
}

void AMotorbreathPlayerState::SyncSelectedVehicleClassProperties()
{
    if (!SetSelectedVehicleClassProperties(
        this, GET_MEMBER_NAME_CHECKED(AMotorbreathPlayerState, SelectedVehicleClass),
        SelectedVehicleClass.Get()))
    {
        UE_LOG(LogMotorbreathPlayerState, Error,
            TEXT("%s is missing the SelectedVehicleClass class property."), *GetName());
    }
}
