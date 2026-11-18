// Missile.cpp
#include "Missile.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

AMissile::AMissile()
{
	PrimaryActorTick.bCanEverTick = true;

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(CollisionBox);

	RearGlowCylinder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RearGlowCylinder"));
	RearGlowCylinder->SetupAttachment(StaticMesh);

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> ExplosionFinder(TEXT("/Game/VFX/NS_Explosion.NS_Explosion"));
	if (ExplosionFinder.Succeeded())
	{
		ExplosionSystem = ExplosionFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> FragsFinder(TEXT("/Game/VFX/NS_TriangleFrags.NS_TriangleFrags"));
	if (FragsFinder.Succeeded())
	{
		TriangleFragsSystem = FragsFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> SmokeFinder(TEXT("/Game/VFX/NS_Smoke.NS_Smoke"));
	if (SmokeFinder.Succeeded())
	{
		SmokeSystem = SmokeFinder.Object;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Could not load the missile smoke Niagara system."));
	}
}

void AMissile::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (CollisionBox)
	{
		CollisionBox->OnComponentHit.AddUniqueDynamic(this, &AMissile::OnCollisionBoxHit);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s has no inherited collision capsule."), *GetName());
	}
}

void AMissile::Tick(float DeltaSeconds)
{
	// N5 -> N6: Event Tick -> Parent: Tick
	Super::Tick(DeltaSeconds);

	if (IsActorBeingDestroyed())
	{
		return;
	}

	if (IsValid(RearGlowCylinder) && SmokeSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			SmokeSystem,
			RearGlowCylinder->GetComponentLocation(),
			FRotator::ZeroRotator,
			FVector(1.0f, 1.0f, 1.0f),
			true,
			true,
			ENCPoolMethod::AutoRelease,
			true
		);
	}
}

void AMissile::OnCollisionBoxHit(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, FVector, const FHitResult&)
{
	CauseDamage(OtherActor);

	const FVector MeshLocation = StaticMesh ? StaticMesh->GetComponentLocation() : GetActorLocation();
	const FRotator CollisionRotation = CollisionBox
		? CollisionBox->GetComponentRotation()
		: FRotator::ZeroRotator;

	if (ExplosionSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			ExplosionSystem,
			MeshLocation,
			CollisionRotation,
			FVector::OneVector,
			true,
			true,
			ENCPoolMethod::None,
			true);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s has no explosion Niagara system."), *GetName());
	}

	if (TriangleFragsSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			TriangleFragsSystem,
			MeshLocation,
			CollisionRotation,
			FVector::OneVector,
			true,
			true,
			ENCPoolMethod::None,
			true);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s has no fragment Niagara system."), *GetName());
	}

	DestroySelf();
}