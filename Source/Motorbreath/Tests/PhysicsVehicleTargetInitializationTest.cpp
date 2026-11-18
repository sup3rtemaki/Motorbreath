#if WITH_DEV_AUTOMATION_TESTS

#include "../PhysicsVehicle.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPhysicsVehicleTargetInitializationTest,
    "Motorbreath.PhysicsVehicle.TargetInitialization",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPhysicsVehicleTargetInitializationTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world"), World))
    {
        return false;
    }

    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);

    APhysicsVehicle* FirstVehicle = World->SpawnActor<APhysicsVehicle>();
    APhysicsVehicle* SecondVehicle = World->SpawnActor<APhysicsVehicle>();
    APhysicsVehicle* ThirdVehicle = World->SpawnActor<APhysicsVehicle>();
    FArrayProperty* TargetListProperty = FindFProperty<FArrayProperty>(
        APhysicsVehicle::StaticClass(), TEXT("TargetList"));
    FObjectPropertyBase* TargetListInnerProperty = TargetListProperty
        ? CastField<FObjectPropertyBase>(TargetListProperty->Inner)
        : nullptr;
    FObjectPropertyBase* NearestTargetProperty = CastField<FObjectPropertyBase>(
        FindFProperty<FProperty>(APhysicsVehicle::StaticClass(), TEXT("NearestTarget")));
    if (!TestNotNull(TEXT("First vehicle"), FirstVehicle) ||
        !TestNotNull(TEXT("Second vehicle"), SecondVehicle) ||
        !TestNotNull(TEXT("Third vehicle"), ThirdVehicle) ||
        !TestNotNull(TEXT("Target list property"), TargetListProperty) ||
        !TestNotNull(TEXT("Target list element property"), TargetListInnerProperty) ||
        !TestNotNull(TEXT("Nearest target property"), NearestTargetProperty))
    {
        GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
        return false;
    }

    const auto GetTargetCount = [TargetListProperty](APhysicsVehicle* Vehicle)
    {
        FScriptArrayHelper TargetArray(TargetListProperty,
            TargetListProperty->ContainerPtrToValuePtr<void>(Vehicle));
        return TargetArray.Num();
    };
    const auto ContainsTarget = [TargetListProperty, TargetListInnerProperty](
        APhysicsVehicle* Vehicle, AActor* Target)
    {
        FScriptArrayHelper TargetArray(TargetListProperty,
            TargetListProperty->ContainerPtrToValuePtr<void>(Vehicle));
        for (int32 Index = 0; Index < TargetArray.Num(); ++Index)
        {
            if (TargetListInnerProperty->GetObjectPropertyValue(TargetArray.GetRawPtr(Index)) == Target)
            {
                return true;
            }
        }
        return false;
    };

    SecondVehicle->SetActorLocation(FVector(100.0, 0.0, 0.0));
    ThirdVehicle->SetActorLocation(FVector(1000.0, 0.0, 0.0));

    World->InitializeActorsForPlay(FURL());
    World->BeginPlay();

    TestEqual(TEXT("Vehicle network update frequency matches the Blueprint"), FirstVehicle->NetUpdateFrequency, 66.0f);
    TestEqual(TEXT("Vehicle minimum network update frequency matches the Blueprint"),
        FirstVehicle->MinNetUpdateFrequency, 33.0f);

    TestEqual(TEXT("First vehicle targets the other vehicles"), GetTargetCount(FirstVehicle), 2);
    TestFalse(TEXT("First vehicle excludes itself"), ContainsTarget(FirstVehicle, FirstVehicle));
    TestTrue(TEXT("First vehicle includes the second"), ContainsTarget(FirstVehicle, SecondVehicle));
    TestTrue(TEXT("First vehicle includes the third"), ContainsTarget(FirstVehicle, ThirdVehicle));

    TestEqual(TEXT("Second vehicle targets the other vehicles"), GetTargetCount(SecondVehicle), 2);
    TestFalse(TEXT("Second vehicle excludes itself"), ContainsTarget(SecondVehicle, SecondVehicle));
    TestTrue(TEXT("Second vehicle includes the first"), ContainsTarget(SecondVehicle, FirstVehicle));
    TestTrue(TEXT("Second vehicle includes the third"), ContainsTarget(SecondVehicle, ThirdVehicle));

    TestEqual(TEXT("Third vehicle targets the other vehicles"), GetTargetCount(ThirdVehicle), 2);
    TestFalse(TEXT("Third vehicle excludes itself"), ContainsTarget(ThirdVehicle, ThirdVehicle));
    TestTrue(TEXT("Third vehicle includes the first"), ContainsTarget(ThirdVehicle, FirstVehicle));
    TestTrue(TEXT("Third vehicle includes the second"), ContainsTarget(ThirdVehicle, SecondVehicle));

    FirstVehicle->Tick(0.016f);
    TestEqual(TEXT("Tick caches the nearest vehicle"),
        Cast<AActor>(NearestTargetProperty->GetObjectPropertyValue_InContainer(FirstVehicle)),
        static_cast<AActor*>(SecondVehicle));

    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    return true;
}

#endif
