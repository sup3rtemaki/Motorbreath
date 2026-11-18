#if WITH_DEV_AUTOMATION_TESTS

#include "../PhysicsVehicle.h"

#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "NiagaraComponent.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPhysicsVehicleSkidEffectsTest, "Motorbreath.PhysicsVehicle.SkidEffects",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPhysicsVehicleSkidEffectsTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world"), World))
    {
        return false;
    }

    APhysicsVehicle* Vehicle = World->SpawnActor<APhysicsVehicle>();
    UNiagaraComponent* SkidMarks = Vehicle ? FindObject<UNiagaraComponent>(Vehicle, TEXT("NS_Skidmark_RR")) : nullptr;
    UNiagaraComponent* SkidSmoke = Vehicle ? FindObject<UNiagaraComponent>(Vehicle, TEXT("NS_Smoke_RR")) : nullptr;
    FBoolProperty* IsDead = FindFProperty<FBoolProperty>(APhysicsVehicle::StaticClass(), TEXT("bIsDead"));
    if (!TestNotNull(TEXT("Vehicle"), Vehicle) ||
        !TestNotNull(TEXT("Skid marks system"), SkidMarks) ||
        !TestNotNull(TEXT("Skid smoke system"), SkidSmoke) ||
        !TestNotNull(TEXT("Death property"), IsDead))
    {
        World->DestroyWorld(false);
        return false;
    }

    TestFalse(TEXT("Skid mark system starts inactive"), SkidMarks->IsActive());
    TestFalse(TEXT("Smoke system starts inactive"), SkidSmoke->IsActive());

    Vehicle->HandleSkidMarks(SkidMarks, 700.0, true, 0.0, 0.0, false);
    TestFalse(TEXT("Skid mark slip threshold is exclusive"), SkidMarks->IsActive());
    Vehicle->HandleSkidMarks(SkidMarks, 700.1, true, 0.0, 0.0, false);
    TestTrue(TEXT("High grounded slip activates skid marks"), SkidMarks->IsActive());
    Vehicle->HandleSkidMarks(SkidMarks, 701.0, false, 1.0, 0.0, true);
    TestFalse(TEXT("Skid marks stop when the wheel is airborne"), SkidMarks->IsActive());
    Vehicle->HandleSkidMarks(SkidMarks, 0.0, true, 0.1, 0.0, true);
    TestTrue(TEXT("Handbraking while moving activates skid marks"), SkidMarks->IsActive());
    IsDead->SetPropertyValue_InContainer(Vehicle, true);
    Vehicle->HandleSkidMarks(SkidMarks, 1000.0, true, 1.0, 0.0, true);
    TestFalse(TEXT("Dead vehicles deactivate skid marks"), SkidMarks->IsActive());

    IsDead->SetPropertyValue_InContainer(Vehicle, false);
    Vehicle->HandleSkidSmoke(SkidSmoke, 1200.0, true, 1.0, 1.0, true);
    TestFalse(TEXT("Smoke slip threshold is exclusive"), SkidSmoke->IsActive());
    Vehicle->HandleSkidSmoke(SkidSmoke, 1201.0, true, 0.7, 0.0, true);
    TestFalse(TEXT("Smoke speed threshold is exclusive"), SkidSmoke->IsActive());
    Vehicle->HandleSkidSmoke(SkidSmoke, 1201.0, true, 0.701, 0.0, true);
    TestTrue(TEXT("High slip at speed activates smoke"), SkidSmoke->IsActive());
    Vehicle->HandleSkidSmoke(SkidSmoke, 1300.0, true, 1.0, 0.0, false);
    TestFalse(TEXT("Smoke requires handbraking"), SkidSmoke->IsActive());
    Vehicle->HandleSkidSmoke(SkidSmoke, 1300.0, true, 0.0, 0.1, true);
    TestTrue(TEXT("Positive acceleration can activate smoke while handbraking"), SkidSmoke->IsActive());

    World->DestroyWorld(false);
    return true;
}

#endif
