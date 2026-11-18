#include "Mine.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "PhysicsVehicle.h"
#include "ImpulseBlast.h"

AMine::AMine()
{
    PrimaryActorTick.bCanEverTick = true;

    StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
    StaticMesh->SetupAttachment(CollisionBox);
}

void AMine::BeginPlay()
{
    Super::BeginPlay();

    if (CollisionBox)
    {
        CollisionBox->OnComponentHit.AddUniqueDynamic(this, &AMine::OnCollisionBoxHit);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("%s has no inherited collision capsule."), *GetName());
    }

    if (!StaticMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("%s has no static mesh for its ground trace."), *GetName());
        return;
    }

    const FVector Start = StaticMesh->GetComponentLocation();
    FHitResult Hit;
    const bool bHit = UKismetSystemLibrary::LineTraceSingle(
        this, Start, Start + FVector(0.0, 0.0, -500.0),
        ETraceTypeQuery::TraceTypeQuery1, false, TArray<AActor*>(),
        EDrawDebugTrace::ForOneFrame, Hit, true);
    if (!bHit)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s ground trace missed; applying the Blueprint's empty hit result."), *GetName());
    }
    SetActorLocationAndRotation(
        Hit.ImpactPoint, UKismetMathLibrary::MakeRotFromZ(Hit.ImpactNormal),
        false, nullptr, ETeleportType::None);
}

void AMine::NotifyActorBeginOverlap(AActor* OtherActor)
{
    Super::NotifyActorBeginOverlap(OtherActor);
}

void AMine::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (IsActorBeingDestroyed())
    {
        return;
    }

    if (Timer > MaxSpawnTimeWait)
    {
        bReady = true;
        if (CollisionBox)
        {
            CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndProbe);
        }
    }
    else
    {
        bReady = false;
        if (CollisionBox)
        {
            CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    }

}

void AMine::OnCollisionBoxHit(UPrimitiveComponent*, AActor* OtherActor,
    UPrimitiveComponent*, FVector, const FHitResult&)
{
    if (!bReady)
    {
        return;
    }

    APhysicsVehicle* Vehicle = Cast<APhysicsVehicle>(OtherActor);
    if (!Vehicle)
    {
        return;
    }

    CauseDamage(Vehicle);

    UNiagaraSystem* ExplosionSystem = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/VFX/NS_Explosion.NS_Explosion"));
    const FVector MeshLocation = StaticMesh ? StaticMesh->GetComponentLocation() : GetActorLocation();
    const FRotator MeshRotation = StaticMesh ? StaticMesh->GetComponentRotation() : GetActorRotation();

    if (ExplosionSystem)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            this, ExplosionSystem, MeshLocation, MeshRotation, FVector(10.0f), true, true,
            ENCPoolMethod::None, true);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Could not load the mine explosion Niagara system."));
    }

    UNiagaraSystem* FragsSystem = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/VFX/NS_TriangleFragsBig.NS_TriangleFragsBig"));
    if (FragsSystem)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            this, FragsSystem, MeshLocation, MeshRotation, FVector::OneVector, true, true,
            ENCPoolMethod::None, true);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Could not load the mine fragments Niagara system."));
    }

    UClass* BlastClass = LoadClass<AActor>(
        nullptr, TEXT("/Game/Blueprints/DamageActors/AreaDamage/BP_ImpulseBlast.BP_ImpulseBlast_C"));
    if (!BlastClass)
    {
        UE_LOG(LogTemp, Error, TEXT("Could not load the impulse blast Blueprint class."));
    }
    else if (!GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot spawn the impulse blast without a world."));
    }
    else
    {
        const FTransform SpawnTransform = CollisionBox ? CollisionBox->GetComponentTransform() : GetActorTransform();

        AActor* Blast = GetWorld()->SpawnActorDeferred<AActor>(
            BlastClass, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::Undefined);
        if (Blast)
        {
            const bool bConfigured = AImpulseBlast::ConfigureSpawnedBlast(Blast, 5000.0, 5.0);
            if (!bConfigured)
            {
                UE_LOG(LogTemp, Error, TEXT("Could not configure the spawned impulse blast."));
            }
            Blast->FinishSpawning(SpawnTransform);
            if (!bConfigured)
            {
                Blast->Destroy();
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to spawn the impulse blast."));
        }
    }
    DestroySelf();
}