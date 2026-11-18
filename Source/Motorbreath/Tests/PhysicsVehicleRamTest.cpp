#if WITH_DEV_AUTOMATION_TESTS

#include "../PhysicsVehicle.h"

#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPhysicsVehicleRamTest, "Motorbreath.PhysicsVehicle.Ram",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPhysicsVehicleRamTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world"), World))
    {
        return false;
    }

    APhysicsVehicle* Attacker = World->SpawnActor<APhysicsVehicle>();
    APhysicsVehicle* Target = World->SpawnActor<APhysicsVehicle>();
    FDoubleProperty* RamTimerProperty = FindFProperty<FDoubleProperty>(
        APhysicsVehicle::StaticClass(), TEXT("RamTimer"));
    FDoubleProperty* NormalizedSpeedProperty = FindFProperty<FDoubleProperty>(
        APhysicsVehicle::StaticClass(), TEXT("NormalizedSpeed"));
    FBoolProperty* bIsDeadProperty = FindFProperty<FBoolProperty>(
        APhysicsVehicle::StaticClass(), TEXT("bIsDead"));
    UBoxComponent* FrontCollisionBox = Attacker
        ? Attacker->FindComponentByClass<UBoxComponent>()
        : nullptr;
    if (!TestNotNull(TEXT("Attacking vehicle"), Attacker) ||
        !TestNotNull(TEXT("Target vehicle"), Target) ||
        !TestNotNull(TEXT("Ram timer"), RamTimerProperty) ||
        !TestNotNull(TEXT("Normalized speed"), NormalizedSpeedProperty) ||
        !TestNotNull(TEXT("Dead state"), bIsDeadProperty) ||
        !TestNotNull(TEXT("Front collision box"), FrontCollisionBox))
    {
        World->DestroyWorld(false);
        return false;
    }

    Target->SetActorLocation(FVector(1000.0, 0.0, 0.0));
    NormalizedSpeedProperty->SetPropertyValue_InContainer(Attacker, 0.5);
    RamTimerProperty->SetPropertyValue_InContainer(Attacker, 1.0);
    Attacker->HandleFrontCollisionOverlap(
        FrontCollisionBox, Target, nullptr, 0, false, FHitResult());
    TestEqual(TEXT("Ram cooldown prevents a second impact"),
        RamTimerProperty->GetPropertyValue_InContainer(Attacker), 1.0);

    RamTimerProperty->SetPropertyValue_InContainer(Attacker, 2.0);
    NormalizedSpeedProperty->SetPropertyValue_InContainer(Attacker, 0.3);
    Attacker->HandleFrontCollisionOverlap(
        FrontCollisionBox, Target, nullptr, 0, false, FHitResult());
    TestEqual(TEXT("Low speed does not trigger a ram"),
        RamTimerProperty->GetPropertyValue_InContainer(Attacker), 2.0);

    NormalizedSpeedProperty->SetPropertyValue_InContainer(Attacker, 0.5);
    bIsDeadProperty->SetPropertyValue_InContainer(Attacker, true);
    Attacker->HandleFrontCollisionOverlap(
        FrontCollisionBox, Target, nullptr, 0, false, FHitResult());
    TestEqual(TEXT("Dead vehicles cannot ram"),
        RamTimerProperty->GetPropertyValue_InContainer(Attacker), 2.0);

    bIsDeadProperty->SetPropertyValue_InContainer(Attacker, false);
    Attacker->HandleFrontCollisionOverlap(
        FrontCollisionBox, Target, nullptr, 0, false, FHitResult());
    TestEqual(TEXT("Successful ram starts the ram cooldown"),
        RamTimerProperty->GetPropertyValue_InContainer(Attacker), 0.0);

    World->DestroyWorld(false);
    return true;
}

#endif
