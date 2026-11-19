#include "Item.h"

#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsVehicle.h"

DEFINE_LOG_CATEGORY_STATIC(LogItemPickup, Log, All);

AItem::AItem()
{
    PrimaryActorTick.bCanEverTick = true;

    Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
    SetRootComponent(Capsule);
    Capsule->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    Capsule->SetGenerateOverlapEvents(true);
    Capsule->OnComponentBeginOverlap.AddDynamic(this, &AItem::OnCapsuleBeginOverlap);

    StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
    StaticMesh->SetupAttachment(Capsule);
}

void AItem::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    if (Item.Mesh && StaticMesh)
    {
        StaticMesh->SetStaticMesh(Item.Mesh);
    }
}

void AItem::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (IsValid(StaticMesh))
    {
        StaticMesh->AddLocalRotation(FRotator(0.0, 1.0, 0.0), false, nullptr, ETeleportType::None);
    }
}

void AItem::OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
    const FHitResult& SweepResult)
{
    UE_LOG(LogItemPickup, Verbose, TEXT("%s overlaps actor %s, component %s."),
        *GetName(), *GetNameSafe(OtherActor), *GetNameSafe(OtherComp));
    APhysicsVehicle* Vehicle = Cast<APhysicsVehicle>(OtherActor);
    if (!IsValid(Vehicle))
    {
        return;
    }

    Vehicle->AddWeapon(Item);
    Destroy();
}
