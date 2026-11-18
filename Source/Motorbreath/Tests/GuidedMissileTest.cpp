#if WITH_DEV_AUTOMATION_TESTS

#include "../GuidedMissile.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuidedMissileHomingTest, "Motorbreath.Damage.GuidedMissileHoming",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

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
