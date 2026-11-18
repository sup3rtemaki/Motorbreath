#include "MenuGameMode.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogMenuGameMode, Log, All);

AMenuGameMode::AMenuGameMode()
{
    static ConstructorHelpers::FClassFinder<UUserWidget> MainMenuFinder(TEXT("/Game/UI/UMG_MainMenu"));
    if (MainMenuFinder.Succeeded())
    {
        MainMenuClass = MainMenuFinder.Class;
    }
}

void AMenuGameMode::BeginPlay()
{
    Super::BeginPlay();

    UUserWidget* MainMenu = nullptr;
    if (MainMenuClass)
    {
        MainMenu = UWidgetBlueprintLibrary::Create(this, MainMenuClass, nullptr);
    }
    if (MainMenu)
    {
        MainMenu->AddToViewport(0);
    }
    else
    {
        UE_LOG(LogMenuGameMode, Error, TEXT("Failed to create main menu widget %s."),
            *GetNameSafe(MainMenuClass.Get()));
    }

    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
    if (!PlayerController)
    {
        UE_LOG(LogMenuGameMode, Error, TEXT("Could not find player controller 0 for the main menu."));
        return;
    }

    UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(
        PlayerController, MainMenu, EMouseLockMode::DoNotLock, false, false);
    PlayerController->bShowMouseCursor = true;
}
