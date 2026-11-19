#if WITH_DEV_AUTOMATION_TESTS

#include "../GuidedMissile.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuidedMissileHomingTest, "Motorbreath.Damage.GuidedMissileHoming",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuidedMissileLaunchTest, "Motorbreath.Damage.GuidedMissileLaunch",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGuidedMissileLaunchTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world"), World))
    {
        return false;
    }

    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    World->InitializeActorsForPlay(FURL());
    World->BeginPlay();

    AGuidedMissile* Missile = World->SpawnActor<AGuidedMissile>(
        AGuidedMissile::StaticClass(), FVector(0.0, 0.0, 1000.0), FRotator::ZeroRotator);
    if (!TestNotNull(TEXT("Guided missile"), Missile))
    {
        GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
        return false;
    }

    UStaticMeshComponent* Body = FindObject<UStaticMeshComponent>(Missile, TEXT("StaticMesh"));
    UStaticMeshComponent* Glow = FindObject<UStaticMeshComponent>(Missile, TEXT("RearGlowCylinder"));
    UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Missile->GetRootComponent());
    if (TestNotNull(TEXT("Missile mesh component"), Body) &&
        TestNotNull(TEXT("Rear glow component"), Glow) &&
        TestNotNull(TEXT("Missile collision capsule"), Capsule))
    {
        TestEqual(TEXT("Missile uses its visible model"),
            GetNameSafe(Body->GetStaticMesh()), FString(TEXT("RocketSimple")));
        TestEqual(TEXT("Guided missile uses its blue material"),
            GetNameSafe(Body->GetMaterial(0)), FString(TEXT("M_Emissive_Color_0-0-127")));
        TestEqual(TEXT("Rear glow uses a cylinder"),
            GetNameSafe(Glow->GetStaticMesh()), FString(TEXT("Cylinder")));
        TestEqual(TEXT("Rear glow uses its emissive material"),
            GetNameSafe(Glow->GetMaterial(0)), FString(TEXT("M_MissileRearGlow")));
        TestTrue(TEXT("Missile capsule blocks world objects"),
            Capsule->GetCollisionResponseToChannel(ECC_WorldDynamic) == ECR_Block);
    }

    Missile->SetActorEnableCollision(false);
    const FVector Start = Missile->GetActorLocation();
    World->Tick(LEVELTICK_All, 0.01f);
    const FVector Displacement = Missile->GetActorLocation() - Start;
    TestTrue(TEXT("Missile launches forward at the configured speed"),
        FMath::IsNearlyEqual(Displacement.X, 100.0, 1.0));
    TestTrue(TEXT("Missile flies straight without dropping"),
        FMath::IsNearlyZero(Displacement.Y, 0.1) && FMath::IsNearlyZero(Displacement.Z, 0.1));

    AActor* Barrier = World->SpawnActor<AActor>();
    if (TestNotNull(TEXT("Missile barrier"), Barrier))
    {
        UBoxComponent* BarrierBox = NewObject<UBoxComponent>(Barrier);
        Barrier->AddInstanceComponent(BarrierBox);
        Barrier->SetRootComponent(BarrierBox);
        BarrierBox->SetBoxExtent(FVector(10.0, 100.0, 100.0));
        BarrierBox->SetCollisionProfileName(TEXT("BlockAllDynamic"));
        BarrierBox->RegisterComponent();
        Barrier->SetActorLocation(Start + FVector(300.0, 0.0, 0.0));
        Missile->SetActorEnableCollision(true);
        World->Tick(LEVELTICK_All, 0.03f);
        TestTrue(TEXT("Missile is destroyed on impact"), Missile->IsActorBeingDestroyed());
    }

    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    return true;
}

bool FGuidedMissileHomingTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world"), World))
    {
        return false;
    }

    AGuidedMissile* Missile = World->SpawnActor<AGuidedMissile>();
    AActor* Target = World->SpawnActor<AActor>();
    if (!TestNotNull(TEXT("Guided missile"), Missile) ||
        !TestNotNull(TEXT("Target"), Target) ||
        !TestNotNull(TEXT("Projectile movement"), Missile ? Missile->ProjectileMovement.Get() : nullptr))
    {
        World->DestroyWorld(false);
        return false;
    }

    USceneComponent* TargetPoint = NewObject<USceneComponent>(Target);
    Target->AddInstanceComponent(TargetPoint);
    Target->SetRootComponent(TargetPoint);
    TargetPoint->RegisterComponent();
    Target->SetActorLocation(FVector(1000.0, 0.0, 0.0));
    Missile->TargetActor = Target;
    Missile->HomingAcceleration = 1234.0;
    Missile->ProjectileMovement->bIsHomingProjectile = false;

    Missile->SetHomingParameters();
    TestFalse(TEXT("Untagged targets do not enable homing"),
        Missile->ProjectileMovement->bIsHomingProjectile);

    TargetPoint->ComponentTags.Add(TEXT("TargetPointTag"));
    Missile->SetHomingParameters();
    TestTrue(TEXT("A tagged target enables homing"),
        Missile->ProjectileMovement->bIsHomingProjectile);
    TestTrue(TEXT("Homing uses the tagged component"),
        Missile->ProjectileMovement->HomingTargetComponent.Get() == TargetPoint);
    TestEqual(TEXT("Homing uses the configured acceleration"),
        Missile->ProjectileMovement->HomingAccelerationMagnitude, 1234.0f);

    TestFalse(TEXT("A nearby target in front keeps homing"), Missile->ShouldStopHoming());
    Target->SetActorLocation(FVector(-1000.0, 0.0, 0.0));
    TestTrue(TEXT("A target behind stops homing"), Missile->ShouldStopHoming());
    Target->SetActorLocation(FVector(25000.0, 0.0, 0.0));
    TestFalse(TEXT("A target at the distance limit keeps homing"), Missile->ShouldStopHoming());
    Target->SetActorLocation(FVector(25001.0, 0.0, 0.0));
    TestTrue(TEXT("A distant target stops homing"), Missile->ShouldStopHoming());

    World->DestroyWorld(false);
    return true;
}

#endif
