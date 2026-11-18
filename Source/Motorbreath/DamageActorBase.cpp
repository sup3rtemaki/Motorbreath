#include "DamageActorBase.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"

ADamageActorBase::ADamageActorBase()
{
    PrimaryActorTick.bCanEverTick = true;

    CollisionBox = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionBox"));
    RootComponent = CollisionBox;
}

void ADamageActorBase::BeginPlay()
{
    Super::BeginPlay();

    // N2: Set Timer (default value 0.0)
    Timer = 0.0;
}

void ADamageActorBase::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    Timer += static_cast<double>(DeltaSeconds);
    if (MaxLifetime > 0.0 && Timer >= MaxLifetime)
    {
        DestroySelf();
    }
}

void ADamageActorBase::CauseDamage(AActor* OtherActor)
{
    if (IsValid(OtherActor) && GetWorld())
    {
        UGameplayStatics::ApplyDamage(
            OtherActor, static_cast<float>(Damage), nullptr, this, UDamageType::StaticClass());
    }
}

void ADamageActorBase::DestroySelf()
{
    Destroy();
}