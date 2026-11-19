#include "GuidedMissile.h"

#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInterface.h"
#include "PhysicsVehicle.h"
#include "UObject/ConstructorHelpers.h"

AGuidedMissile::AGuidedMissile()
{
    ProjectileMovement->MaxSpeed = 70000.0f;

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> GuidedMaterialFinder(
        TEXT("/Engine/ArtTools/RenderToTexture/Materials/Debug/M_Emissive_Color_0-0-127.M_Emissive_Color_0-0-127"));
    if (GuidedMaterialFinder.Succeeded())
    {
        StaticMesh->SetMaterial(0, GuidedMaterialFinder.Object);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Could not load the guided missile material."));
    }
}

void AGuidedMissile::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (IsActorBeingDestroyed() || Timer <= 0.1)
    {
        return;
    }

    APhysicsVehicle* TargetVehicle = IsValid(TargetActor)
        ? Cast<APhysicsVehicle>(TargetActor.Get())
        : nullptr;
    if (TargetVehicle && !TargetVehicle->IsDead())
    {
        if (!ProjectileMovement)
        {
            UE_LOG(LogTemp, Error, TEXT("%s has no projectile movement component."), *GetName());
            return;
        }

        if (ProjectileMovement->bIsHomingProjectile)
        {
            if (ShouldStopHoming())
            {
                ProjectileMovement->bIsHomingProjectile = false;
                TargetActor = nullptr;
            }
            else if (StaticMesh &&
                ProjectileMovement->HomingTargetComponent.IsValid())
            {
                const FVector Start = StaticMesh->GetComponentLocation();
                const FVector Target = ProjectileMovement->HomingTargetComponent->GetComponentLocation();
                SetActorRotation(
                    UKismetMathLibrary::FindLookAtRotation(Start, Target), ETeleportType::None);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("%s cannot rotate toward its homing target."), *GetName());
            }
        }
        else
        {
            SetHomingParameters();
        }
        return;
    }

    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(EObjectTypeQuery::ObjectTypeQuery5);

    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(GetInstigator());

    FHitResult OutHit;
    const FVector TraceStart = GetActorLocation();
    const FVector TraceEnd = TraceStart + GetActorForwardVector() * 5000.0;
    const bool bHit = UKismetSystemLibrary::CapsuleTraceSingleForObjects(
        this,
        TraceStart,
        TraceEnd,
        5000.0f,
        0.0f,
        ObjectTypes,
        false,
        ActorsToIgnore,
        EDrawDebugTrace::None,
        OutHit,
        true,
        FLinearColor::Red,
        FLinearColor::Green,
        5.0f);

    APhysicsVehicle* HitVehicle = bHit ? Cast<APhysicsVehicle>(OutHit.GetActor()) : nullptr;
    if (HitVehicle && !HitVehicle->IsDead())
    {
        if (StaticMesh)
        {
            StaticMesh->SetSimulatePhysics(false);
        }

        TargetActor = HitVehicle;
        SetHomingParameters();
    }
}

bool AGuidedMissile::ShouldStopHoming_Implementation()
{
    if (!IsValid(TargetActor))
    {
        UE_LOG(LogTemp, Error, TEXT("%s cannot evaluate homing without a valid target."), *GetName());
        return true;
    }

    const FVector TargetLocation = TargetActor->GetActorLocation();
    const FRotator RelativeRotation =
        UKismetMathLibrary::FindRelativeLookAtRotation(GetActorTransform(), TargetLocation);
    return FMath::Abs(RelativeRotation.Pitch) > 90.0 ||
        FMath::Abs(RelativeRotation.Yaw) > 90.0 ||
        FVector::Dist(GetActorLocation(), TargetLocation) > 25000.0;
}

void AGuidedMissile::SetHomingParameters()
{
    if (!ProjectileMovement)
    {
        UE_LOG(LogTemp, Error, TEXT("%s has no projectile movement component."), *GetName());
        return;
    }

    if (!IsValid(TargetActor))
    {
        UE_LOG(LogTemp, Error, TEXT("%s cannot set homing parameters without a valid target."), *GetName());
        return;
    }

    const TArray<UActorComponent*> TargetComponents =
        TargetActor->GetComponentsByTag(USceneComponent::StaticClass(), TEXT("TargetPointTag"));
    if (TargetComponents.IsEmpty())
    {
        return;
    }

    ProjectileMovement->HomingTargetComponent = CastChecked<USceneComponent>(
        TargetComponents[FMath::RandRange(0, TargetComponents.Num() - 1)]);
    ProjectileMovement->bIsHomingProjectile = true;
    ProjectileMovement->HomingAccelerationMagnitude = static_cast<float>(HomingAcceleration);
}