#if WITH_DEV_AUTOMATION_TESTS

#include "../PhysicsVehicle.h"

#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPhysicsVehicleHandlingTest, "Motorbreath.PhysicsVehicle.Handling",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPhysicsVehicleHandlingTest::RunTest(const FString& Parameters)
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
    World->Tick(LEVELTICK_All, 0.1f);

    USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Blueprints/PhysicsVehicles/F1_Body.F1_Body"));
    UPhysicsAsset* PhysicsAsset = LoadObject<UPhysicsAsset>(nullptr, TEXT("/Game/Blueprints/PhysicsVehicles/F1_Body_PhysicsAsset.F1_Body_PhysicsAsset"));
    APhysicsVehicle* Vehicle = World->SpawnActorDeferred<APhysicsVehicle>(APhysicsVehicle::StaticClass(), FTransform::Identity);
    USkeletalMeshComponent* Chassis = Vehicle ? FindObject<USkeletalMeshComponent>(Vehicle, TEXT("PhysicsMesh")) : nullptr;
    AActor* Ground = World->SpawnActor<AActor>();
    FDoubleProperty* Acceleration = FindFProperty<FDoubleProperty>(APhysicsVehicle::StaticClass(), TEXT("Acceleration"));
    FDoubleProperty* Slip = FindFProperty<FDoubleProperty>(APhysicsVehicle::StaticClass(), TEXT("WheelSlipAmount"));
    FBoolProperty* IsDead = FindFProperty<FBoolProperty>(APhysicsVehicle::StaticClass(), TEXT("bIsDead"));
    FBoolProperty* GroundedRR = FindFProperty<FBoolProperty>(APhysicsVehicle::StaticClass(), TEXT("bGroundedRR"));
    FObjectPropertyBase* DampingProperty = FindFProperty<FObjectPropertyBase>(APhysicsVehicle::StaticClass(), TEXT("AngularDamping"));
    if (!TestNotNull(TEXT("Skeletal mesh"), Mesh) ||
        !TestNotNull(TEXT("Physics asset"), PhysicsAsset) ||
        !TestNotNull(TEXT("Vehicle"), Vehicle) ||
        !TestNotNull(TEXT("Chassis"), Chassis) ||
        !TestNotNull(TEXT("Ground actor"), Ground) ||
        !TestNotNull(TEXT("Acceleration property"), Acceleration) ||
        !TestNotNull(TEXT("Slip property"), Slip) ||
        !TestNotNull(TEXT("Death property"), IsDead) ||
        !TestNotNull(TEXT("Rear contact property"), GroundedRR) ||
        !TestNotNull(TEXT("Damping curve property"), DampingProperty))
    {
        GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
        return false;
    }

    Chassis->SetSkeletalMesh(Mesh);
    Chassis->SetPhysicsAsset(PhysicsAsset);
    Vehicle->FinishSpawning(FTransform::Identity);
    if (!TestNotNull(TEXT("Chassis physics body"), Chassis->GetBodyInstance()))
    {
        GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
        return false;
    }
    Chassis->SetAllBodiesSimulatePhysics(true);
    Chassis->SetEnableGravity(false);

    UCurveFloat* DampingCurve = NewObject<UCurveFloat>(Vehicle);
    DampingCurve->FloatCurve.AddKey(0.0f, 2.0f);
    DampingProperty->SetObjectPropertyValue_InContainer(Vehicle, DampingCurve);
    Vehicle->Tick(0.1f);
    TestFalse(TEXT("Rear wheel starts airborne"), GroundedRR->GetPropertyValue_InContainer(Vehicle));
    TestTrue(TEXT("Airborne angular damping is 0.05"),
        FMath::IsNearlyEqual(Chassis->GetAngularDamping(), 0.05f, 0.001f));
    TestEqual(TEXT("Stationary rear wheels have no slip"), Slip->GetPropertyValue_InContainer(Vehicle), 0.0);

    UBoxComponent* GroundBox = NewObject<UBoxComponent>(Ground);
    Ground->SetRootComponent(GroundBox);
    GroundBox->SetBoxExtent(FVector(1000.0, 1000.0, 10.0));
    GroundBox->SetCollisionProfileName(TEXT("BlockAll"));
    GroundBox->RegisterComponent();
    GroundBox->SetWorldLocation(FVector(0.0, 0.0, -25.0));
    Acceleration->SetPropertyValue_InContainer(Vehicle, 1.0);
    Vehicle->Tick(0.1f);
    TestTrue(TEXT("Rear wheel detects ground"), GroundedRR->GetPropertyValue_InContainer(Vehicle));
    TestTrue(TEXT("High acceleration at low speed reduces damping"),
        FMath::IsNearlyEqual(Chassis->GetAngularDamping(), 0.4f, 0.001f));

    Acceleration->SetPropertyValue_InContainer(Vehicle, 0.5);
    Vehicle->Tick(0.1f);
    TestTrue(TEXT("Normal grounded damping uses the curve"),
        FMath::IsNearlyEqual(Chassis->GetAngularDamping(), 2.0f, 0.001f));

    Chassis->SetPhysicsLinearVelocity(Chassis->GetRightVector() * 100.0);
    Acceleration->SetPropertyValue_InContainer(Vehicle, 0.5);
    Vehicle->Tick(0.1f);
    TestTrue(TEXT("Rear slip uses lateral velocity and acceleration"),
        FMath::IsNearlyEqual(Slip->GetPropertyValue_InContainer(Vehicle), 50.0, 0.1));

    IsDead->SetPropertyValue_InContainer(Vehicle, true);
    GroundBox->SetWorldLocation(FVector(0.0, 0.0, -200.0));
    Vehicle->Tick(0.1f);
    TestTrue(TEXT("A dead vehicle stops updating damping"),
        FMath::IsNearlyEqual(Chassis->GetAngularDamping(), 2.0f, 0.001f));

    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    return true;
}

#endif
