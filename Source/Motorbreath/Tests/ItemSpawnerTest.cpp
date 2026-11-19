#if WITH_DEV_AUTOMATION_TESTS

#include "../Item.h"
#include "../ItemSpawner.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemSpawnerTest, "Motorbreath.Inventory.ItemSpawner",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FItemSpawnerTest::RunTest(const FString& Parameters)
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

    UStaticMesh* ItemMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    AItem* SourceItem = World->SpawnActorDeferred<AItem>(
        AItem::StaticClass(), FTransform(FVector(1000.0, 0.0, 0.0)));
    if (!TestNotNull(TEXT("Source weapon item"), SourceItem) ||
        !TestNotNull(TEXT("Test item mesh"), ItemMesh))
    {
        GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
        return false;
    }

    SourceItem->Item.Id = 11;
    SourceItem->Item.Name = TEXT("Spawner weapon");
    SourceItem->Item.Quantity = 2;
    SourceItem->Item.Weapon = AItem::StaticClass();
    SourceItem->Item.Mesh = ItemMesh;
    SourceItem->FinishSpawning(FTransform(FVector(1000.0, 0.0, 0.0)));
    TestTrue(TEXT("Source item displays its configured mesh"), SourceItem->StaticMesh->GetStaticMesh() == ItemMesh);

    AItemSpawner* Spawner = World->SpawnActorDeferred<AItemSpawner>(
        AItemSpawner::StaticClass(), FTransform::Identity);
    if (!TestNotNull(TEXT("Item spawner"), Spawner))
    {
        GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
        return false;
    }

    Spawner->Item = SourceItem;
    Spawner->ItemClass = AItem::StaticClass();
    Spawner->FinishSpawning(FTransform::Identity);

    AItem* FirstSpawnedItem = Cast<AItem>(Spawner->SpawnedItem);
    if (!TestNotNull(TEXT("Initially spawned weapon item"), FirstSpawnedItem))
    {
        GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
        return false;
    }
    TestEqual(TEXT("Spawner copies the source item ID"), FirstSpawnedItem->Item.Id, 11);
    TestEqual(TEXT("Spawner copies the source item quantity"), FirstSpawnedItem->Item.Quantity, 2);
    TestTrue(TEXT("Spawned item displays the copied mesh"), FirstSpawnedItem->StaticMesh->GetStaticMesh() == ItemMesh);

    FirstSpawnedItem->Destroy();
    Spawner->Tick(0.1f);
    TestFalse(TEXT("Respawn delay does not expire on its first tick"),
        IsValid(Spawner->SpawnedItem));

    Spawner->Tick(0.1f);
    AItem* RespawnedItem = Cast<AItem>(Spawner->SpawnedItem);
    TestNotNull(TEXT("Spawner creates another weapon item after the delay"), RespawnedItem);
    if (RespawnedItem)
    {
        TestTrue(TEXT("Respawned item retains weapon data"),
            RespawnedItem->Item.Id == 11 && RespawnedItem->Item.Quantity == 2);
        TestTrue(TEXT("Respawned item displays the copied mesh"), RespawnedItem->StaticMesh->GetStaticMesh() == ItemMesh);
    }

    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    return true;
}

#endif
