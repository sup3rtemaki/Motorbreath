#include "ExplosiveTarget.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "ImpulseBlast.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogExplosiveTarget, Log, All);

AExplosiveTarget::AExplosiveTarget()
{
    PrimaryActorTick.bCanEverTick = true;

    Cylinder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cylinder"));
    RootComponent = Cylinder;
    Cylinder->SetSimulatePhysics(true);
    Cylinder->SetGenerateOverlapEvents(true);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> TargetMaterialFinder(
        TEXT("/Game/Materials/M_Target.M_Target"));
    if (CylinderMeshFinder.Succeeded())
    {
        Cylinder->SetStaticMesh(CylinderMeshFinder.Object);
    }
    else
    {
        UE_LOG(LogExplosiveTarget, Error, TEXT("Could not load the basic cylinder mesh."));
    }
    if (TargetMaterialFinder.Succeeded())
    {
        Cylinder->SetMaterial(0, TargetMaterialFinder.Object);
    }
    else
    {
        UE_LOG(LogExplosiveTarget, Error, TEXT("Could not load the target material."));
    }

    Flame = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Flame"));
    Flame->SetupAttachment(Cylinder);
    Flame->SetAutoActivate(false);

    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> FlameSystemFinder(
        TEXT("/Game/VFX/NS_Flame.NS_Flame"));
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> ExplosionSystemFinder(
        TEXT("/Game/VFX/NS_Explosion.NS_Explosion"));
    static ConstructorHelpers::FClassFinder<AActor> ImpulseBlastFinder(
        TEXT("/Game/Blueprints/DamageActors/AreaDamage/BP_ImpulseBlast"));

    if (FlameSystemFinder.Succeeded())
    {
        Flame->SetAsset(FlameSystemFinder.Object);
    }
    else
    {
        UE_LOG(LogExplosiveTarget, Error, TEXT("Could not load the flame Niagara system."));
    }
    if (ExplosionSystemFinder.Succeeded())
    {
        ExplosionSystem = ExplosionSystemFinder.Object;
    }
    if (ImpulseBlastFinder.Succeeded())
    {
        ImpulseBlastClass = ImpulseBlastFinder.Class;
    }
}

void AExplosiveTarget::BeginPlay()
{
    Super::BeginPlay();
    Hp = MaxHp;

    if (!GetClass()->IsFunctionImplementedInScript(FName(TEXT("ReceiveAnyDamage"))))
    {
        OnTakeAnyDamage.AddUniqueDynamic(this, &AExplosiveTarget::HandleAnyDamage);
    }
}

void AExplosiveTarget::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (Hp > 0.0 || bDetonationScheduled)
    {
        return;
    }

    bDetonationScheduled = true;
    if (Flame)
    {
        Flame->Activate(false);
    }
    else
    {
        UE_LOG(LogExplosiveTarget, Error, TEXT("Flame component is unavailable."));
    }

    const float Delay = FMath::FRandRange(0.75f, 2.0f);
    GetWorldTimerManager().SetTimer(DetonationTimerHandle, this,
        &AExplosiveTarget::Detonate, Delay, false);
}

void AExplosiveTarget::HandleAnyDamage(AActor*, float Damage, const UDamageType*,
    AController*, AActor*)
{
    Hp -= static_cast<double>(Damage);
}

void AExplosiveTarget::Detonate()
{
    if (!Cylinder)
    {
        UE_LOG(LogExplosiveTarget, Error, TEXT("Cylinder component is unavailable."));
        Destroy();
        return;
    }

    const FVector Location = Cylinder->GetComponentLocation();
    const FVector Scale = FVector::OneVector * FMath::FRandRange(0.75f, 2.0f);
    if (ExplosionSystem)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ExplosionSystem, Location,
            FRotator::ZeroRotator, Scale, true, true, ENCPoolMethod::None, true);
    }
    else
    {
        UE_LOG(LogExplosiveTarget, Error, TEXT("Explosion Niagara system is not configured."));
    }

    if (!ImpulseBlastClass)
    {
        UE_LOG(LogExplosiveTarget, Error, TEXT("Impulse blast class is not configured."));
        Destroy();
        return;
    }
    if (!GetWorld())
    {
        UE_LOG(LogExplosiveTarget, Error, TEXT("Cannot spawn the impulse blast without a world."));
        Destroy();
        return;
    }

    const FTransform SpawnTransform = Cylinder->GetComponentTransform();
    AActor* Blast = GetWorld()->SpawnActorDeferred<AActor>(ImpulseBlastClass, SpawnTransform,
        nullptr, nullptr, ESpawnActorCollisionHandlingMethod::Undefined);
    if (!Blast)
    {
        UE_LOG(LogExplosiveTarget, Error, TEXT("Failed to spawn impulse blast class %s."),
            *GetNameSafe(ImpulseBlastClass.Get()));
        Destroy();
        return;
    }

    const bool bConfigured = AImpulseBlast::ConfigureSpawnedBlast(Blast, 50000.0, 40.0);
    if (!bConfigured)
    {
        UE_LOG(LogExplosiveTarget, Error, TEXT("Could not configure the spawned impulse blast."));
    }
    Blast->FinishSpawning(SpawnTransform);
    if (!bConfigured)
    {
        Blast->Destroy();
    }
    Destroy();
}
