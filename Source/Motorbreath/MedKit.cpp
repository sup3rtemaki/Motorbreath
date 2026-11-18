#include "MedKit.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "PhysicsVehicle.h"

AMedKit::AMedKit()
{
	PrimaryActorTick.bCanEverTick = true;

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(CollisionBox);

	bReady = false;
	MaxSpawnTimeWait = 0.0;
}

void AMedKit::BeginPlay()
{
	Super::BeginPlay();

	if (CollisionBox)
	{
		CollisionBox->OnComponentHit.AddUniqueDynamic(this, &AMedKit::OnCollisionBoxHit);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s has no inherited collision capsule."), *GetName());
	}

	if (!StaticMesh || !GetWorld())
	{
		UE_LOG(LogTemp, Error, TEXT("%s cannot run its ground trace without a mesh and world."), *GetName());
		return;
	}

	const FVector Start = StaticMesh->GetComponentLocation();
	const FVector End = Start + FVector(0.0f, 0.0f, -500.0f);

	FHitResult OutHit;
	TArray<AActor*> ActorsToIgnore;
	const bool bHit = UKismetSystemLibrary::LineTraceSingle(
		this,
		Start,
		End,
		UEngineTypes::ConvertToTraceType(ECC_Visibility), // TraceTypeQuery1
		false,
		ActorsToIgnore,
		EDrawDebugTrace::ForOneFrame,
		OutHit,
		true,
		FLinearColor::Red,
		FLinearColor::Green,
		5.0f);

	if (bHit && OutHit.bBlockingHit)
	{
		const FRotator NewRotation = UKismetMathLibrary::MakeRotFromZ(OutHit.ImpactNormal);
		SetActorLocationAndRotation(
			OutHit.ImpactPoint, NewRotation, false, nullptr, ETeleportType::None);
	}
}

void AMedKit::Tick(float DeltaSeconds)
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
			CollisionBox->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Block);
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

void AMedKit::OnCollisionBoxHit(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, FVector, const FHitResult&)
{
	if (bReady)
	{
		APhysicsVehicle* Vehicle = Cast<APhysicsVehicle>(OtherActor);
		if (Vehicle)
		{
			CauseDamage(Vehicle);

			if (PickupEffect && StaticMesh)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					this,
					PickupEffect,
					StaticMesh->GetComponentLocation(),
					StaticMesh->GetComponentRotation(),
					FVector(10.0f, 10.0f, 20.0f),
					true,
					true,
					ENCPoolMethod::None,
					true);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("%s has no pickup effect or mesh."), *GetName());
			}

			DestroySelf();
		}
	}
}