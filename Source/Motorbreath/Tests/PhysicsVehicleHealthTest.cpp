#if WITH_DEV_AUTOMATION_TESTS

#include "../PhysicsVehicle.h"

#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPhysicsVehicleHealthTest, "Motorbreath.PhysicsVehicle.Health",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPhysicsVehicleHealthTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world"), World))
    {
        return false;
    }

    APhysicsVehicle* Vehicle = World->SpawnActor<APhysicsVehicle>();
    FDoubleProperty* Hp = FindFProperty<FDoubleProperty>(APhysicsVehicle::StaticClass(), TEXT("Hp"));
    FDoubleProperty* Armor = FindFProperty<FDoubleProperty>(APhysicsVehicle::StaticClass(), TEXT("Armor"));
    if (!TestNotNull(TEXT("Vehicle"), Vehicle) ||
        !TestNotNull(TEXT("Hp property"), Hp) ||
        !TestNotNull(TEXT("Armor property"), Armor))
    {
        World->DestroyWorld(false);
        return false;
    }

    Hp->SetPropertyValue_InContainer(Vehicle, 100.0);
    Armor->SetPropertyValue_InContainer(Vehicle, 10.0);
    Vehicle->ApplyVehicleDamage(30.0);
    TestEqual(TEXT("Damage with armor reduces health by one third"), Hp->GetPropertyValue_InContainer(Vehicle), 90.0);
    TestEqual(TEXT("Armor may become negative"), Armor->GetPropertyValue_InContainer(Vehicle), -10.0);

    Vehicle->ApplyVehicleDamage(12.0);
    TestEqual(TEXT("Damage without armor reduces health directly"), Hp->GetPropertyValue_InContainer(Vehicle), 78.0);
    TestEqual(TEXT("Damage without armor leaves armor unchanged"), Armor->GetPropertyValue_InContainer(Vehicle), -10.0);

    Vehicle->HealHealth(50.0);
    Vehicle->HealArmor(150.0);
    TestEqual(TEXT("Healing caps health at its maximum"), Hp->GetPropertyValue_InContainer(Vehicle), 100.0);
    TestEqual(TEXT("Healing caps armor at its maximum"), Armor->GetPropertyValue_InContainer(Vehicle), 100.0);

    Vehicle->HealHealth(-120.0);
    TestEqual(TEXT("Healing does not clamp its lower bound"), Hp->GetPropertyValue_InContainer(Vehicle), -20.0);

    World->DestroyWorld(false);
    return true;
}

#endif
