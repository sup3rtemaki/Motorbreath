// Missile.cpp
#include "Missile.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

AMissile::AMissile()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionBox->InitCapsuleSize(33.844398f, 33.844398f);
	CollisionBox->SetCollisionProfileName(TEXT("BlockAll"));

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(CollisionBox);
	ProjectileMovement->InitialSpeed = 10000.0f;
	ProjectileMovement->MaxSpeed = 50000.0f;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bRotationFollowsVelocity = false;
	ProjectileMovement->bInitialVelocityInLocalSpace = true;

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(CollisionBox);
	StaticMesh->SetRelativeLocation(FVector(-80.0, 0.0, 0.0));

	RearGlowCylinder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RearGlowCylinder"));
	RearGlowCylinder->SetupAttachment(StaticMesh);
	RearGlowCylinder->SetRelativeLocation(FVector(-50.0, 0.0, 0.0));
	RearGlowCylinder->SetRelativeRotation(FRotator(90.0, 540.0, 540.0));
	RearGlowCylinder->SetRelativeScale3D(FVector(0.6, 0.6, 0.1));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MissileMeshFinder(
		TEXT("/Game/Models/Weapons/Rocket/RocketSimple.RocketSimple"));
	if (MissileMeshFinder.Succeeded())
	{
		StaticMesh->SetStaticMesh(MissileMeshFinder.Object);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Could not load the missile static mesh."));
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MissileMaterialFinder(
		TEXT("/Engine/ArtTools/RenderToTexture/Materials/Debug/M_Emissive_Color_127-127-0.M_Emissive_Color_127-127-0"));
	if (MissileMaterialFinder.Succeeded())
	{
		StaticMesh->SetMaterial(0, MissileMaterialFinder.Object);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Could not load the missile material."));
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> GlowMeshFinder(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (GlowMeshFinder.Succeeded())
	{
		RearGlowCylinder->SetStaticMesh(GlowMeshFinder.Object);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Could not load the missile rear glow mesh."));
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlowMaterialFinder(
		TEXT("/Game/Materials/M_MissileRearGlow.M_MissileRearGlow"));
	if (GlowMaterialFinder.Succeeded())
	{
		RearGlowCylinder->SetMaterial(0, GlowMaterialFinder.Object);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Could not load the missile rear glow material."));
	}

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