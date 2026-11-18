#include "Bullet.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

ABullet::ABullet()
{
    StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
    StaticMesh->SetupAttachment(CollisionBox);

    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> HitEffectFinder(
        TEXT("/Game/VFX/NS_BulletHit.NS_BulletHit"));
    if (HitEffectFinder.Succeeded())
    {
        HitEffect = HitEffectFinder.Object;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Could not load the bullet hit Niagara system."));
    }
}

void ABullet::BeginPlay()
{
    Super::BeginPlay();

    if (StaticMesh)
    {
        StaticMesh->SetWorldScale3D(FVector(0.05f, 0.05f, 0.5f));
    }

    if (CollisionBox)
    {
        CollisionBox->OnComponentHit.AddUniqueDynamic(this, &ABullet::OnCollisionBoxHit);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("%s has no inherited collision capsule."), *GetName());
    }
}

void ABullet::OnCollisionBoxHit(UPrimitiveComponent*, AActor* OtherActor,
    UPrimitiveComponent*, FVector, const FHitResult&)
{
    CauseDamage(OtherActor);

    if (HitEffect)
    {
        const FVector SpawnLocation = StaticMesh
            ? StaticMesh->GetComponentLocation()
            : GetActorLocation();
        const FRotator SpawnRotation = CollisionBox
            ? CollisionBox->GetComponentRotation()
            : GetActorRotation();

        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            this,
            HitEffect,
            SpawnLocation,
            SpawnRotation,
            FVector(0.5f, 0.5f, 0.5f),
            true,
            true,
            ENCPoolMethod::None,
            true);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("%s has no hit Niagara system."), *GetName());
    }

    DestroySelf();
}