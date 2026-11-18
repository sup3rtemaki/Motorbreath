#include "ImpulseBlast.h"

#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ExplosiveTarget.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "PhysicsVehicle.h"
#include "UObject/UnrealType.h"

namespace
{
    bool SetFloatingPointProperty(UObject* Target, const TCHAR* PropertyName, double Value)
    {
        FNumericProperty* Property = Target
            ? FindFProperty<FNumericProperty>(Target->GetClass(), PropertyName)
            : nullptr;
        if (!Property || !Property->IsFloatingPoint())
        {
            UE_LOG(LogTemp, Error, TEXT("Floating-point property %s was not found on %s."),
                PropertyName, *GetNameSafe(Target));
            return false;
        }

        Property->SetFloatingPointPropertyValue(
            Property->ContainerPtrToValuePtr<void>(Target), Value);
        return true;
    }
}

AImpulseBlast::AImpulseBlast()
{
    StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
    StaticMesh->SetupAttachment(CollisionBox);
}

bool AImpulseBlast::ConfigureSpawnedBlast(AActor* SpawnedActor, double InBlastPower, double InDamage)
{
    if (AImpulseBlast* NativeBlast = Cast<AImpulseBlast>(SpawnedActor))
    {
        NativeBlast->BlastPower = InBlastPower;
        NativeBlast->Damage = InDamage;
        return true;
    }

    // Keep the existing Blueprint class usable until it is reparented to AImpulseBlast.
    const bool bBlastPowerSet =
        SetFloatingPointProperty(SpawnedActor, TEXT("BlastPower"), InBlastPower);
    const bool bDamageSet = SetFloatingPointProperty(SpawnedActor, TEXT("Damage"), InDamage);
    return bBlastPowerSet && bDamageSet;
}

void AImpulseBlast::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    if (!CollisionBox)
    {
        UE_LOG(LogTemp, Error, TEXT("%s has no collision capsule."), *GetName());
        return;
    }

    CollisionBox->OnComponentHit.AddUniqueDynamic(this, &AImpulseBlast::OnCollisionBoxHit);
    CollisionBox->OnComponentBeginOverlap.AddUniqueDynamic(
        this, &AImpulseBlast::OnCollisionBoxBeginOverlap);
}

void AImpulseBlast::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (IsActorBeingDestroyed())
    {
        return;
    }

    if (Timer >= 0.1 || !IsValid(StaticMesh))
    {
        DestroySelf();
        return;
    }

    if (!CollisionBox)
    {
        UE_LOG(LogTemp, Error, TEXT("%s has no collision capsule."), *GetName());
        DestroySelf();
        return;
    }

    TArray<AActor*> OverlappingActors;
    CollisionBox->GetOverlappingActors(OverlappingActors, AActor::StaticClass());

    for (AActor* OverlappingActor : OverlappingActors)
    {
        UPrimitiveComponent* PhysicsComponent = nullptr;
        if (APhysicsVehicle* Vehicle = Cast<APhysicsVehicle>(OverlappingActor))
        {
            PhysicsComponent = Cast<UPrimitiveComponent>(Vehicle->GetRootComponent());
            if (!PhysicsComponent)
            {
                UE_LOG(LogTemp, Error, TEXT("%s has no primitive root component."), *Vehicle->GetName());
                continue;
            }
        }
        else if (AExplosiveTarget* Target = Cast<AExplosiveTarget>(OverlappingActor))
        {
            PhysicsComponent = Cast<UPrimitiveComponent>(Target->GetRootComponent());
            if (!PhysicsComponent)
            {
                UE_LOG(LogTemp, Error, TEXT("%s has no primitive root component."), *Target->GetName());
                continue;
            }
        }
        else
        {
            continue;
        }

        PhysicsComponent->AddRadialImpulse(
            CollisionBox->GetComponentLocation(),
            CollisionBox->GetUnscaledCapsuleRadius(),
            static_cast<float>(BlastPower),
            ERadialImpulseFalloff::RIF_Constant,
            false);
    }
}

void AImpulseBlast::OnCollisionBoxHit(UPrimitiveComponent*, AActor* OtherActor,
    UPrimitiveComponent* OtherComponent, FVector, const FHitResult&)
{
    if (!CollisionBox)
    {
        UE_LOG(LogTemp, Error, TEXT("%s has no collision capsule."), *GetName());
    }
    else if (!OtherComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("%s hit an actor without a primitive component."), *GetName());
    }
    else
    {
        OtherComponent->AddRadialImpulse(
            CollisionBox->GetComponentLocation(),
            CollisionBox->GetUnscaledCapsuleRadius(),
            static_cast<float>(BlastPower),
            ERadialImpulseFalloff::RIF_Linear,
            false);
    }

    if (!IsValid(OtherActor))
    {
        return;
    }

    const bool bAlreadyAffected = AffectedActors.ContainsByPredicate(
        [OtherActor](const TObjectPtr<AActor>& Actor) { return Actor.Get() == OtherActor; });
    if (!bAlreadyAffected)
    {
        CauseDamage(OtherActor);
        AffectedActors.Add(OtherActor);
    }
}

void AImpulseBlast::OnCollisionBoxBeginOverlap(UPrimitiveComponent*, AActor* OtherActor,
    UPrimitiveComponent*, int32, bool, const FHitResult&)
{
    AExplosiveTarget* Target = Cast<AExplosiveTarget>(OtherActor);
    if (!Target)
    {
        return;
    }

    UPrimitiveComponent* TargetComponent =
        Cast<UPrimitiveComponent>(Target->GetRootComponent());
    if (!TargetComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("%s has no primitive root component."), *Target->GetName());
        return;
    }
    if (!CollisionBox)
    {
        UE_LOG(LogTemp, Error, TEXT("%s has no collision capsule."), *GetName());
        return;
    }

    TargetComponent->AddRadialImpulse(
        CollisionBox->GetComponentLocation(),
        CollisionBox->GetUnscaledCapsuleRadius(),
        static_cast<float>(BlastPower),
        ERadialImpulseFalloff::RIF_Linear,
        false);

    UKismetSystemLibrary::PrintString(
        this,
        UKismetStringLibrary::Conv_BoolToString(TargetComponent->IsSimulatingPhysics(NAME_None)),
        true,
        true,
        FLinearColor(0.0f, 0.66f, 1.0f, 1.0f),
        2.0f,
        NAME_None);
}