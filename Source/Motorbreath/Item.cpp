#include "Item.h"

#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsVehicle.h"

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
    APhysicsVehicle* Vehicle = Cast<APhysicsVehicle>(OtherActor);
    if (!IsValid(Vehicle))
    {
        return;
    }

    Vehicle->AddWeapon(Item);
    Destroy();
}
