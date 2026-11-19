#include "PhysicsVehicle.h"

#include "Blueprint/UserWidget.h"
#include "Components/ActorComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogPhysicsVehicle, Log, All);

static bool SetBlueprintRealProperty(UObject* Object, FName PropertyName, double Value)
{
    if (!Object)
    {
        return false;
    }

    FProperty* Property = FindFProperty<FProperty>(Object->GetClass(), PropertyName);
    if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
    {
        FloatProperty->SetPropertyValue_InContainer(Object, static_cast<float>(Value));
        return true;
    }
    if (FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Property))
    {
        DoubleProperty->SetPropertyValue_InContainer(Object, Value);
        return true;
    }
    return false;
}

// Keep Blueprint task references synchronized while the native vehicle and BP subclass coexist.
static bool SetBlueprintBoolProperty(UObject* Object, FName PropertyName, bool Value)
{
    if (!Object)
    {
        return false;
    }

    bool bPropertySet = false;
    for (TFieldIterator<FProperty> It(Object->GetClass(), EFieldIteratorFlags::IncludeSuper); It; ++It)
    {
        if (It->GetFName() == PropertyName)
        {
            if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(*It))
            {
                BoolProperty->SetPropertyValue_InContainer(Object, Value);
                bPropertySet = true;
            }
        }
    }
    return bPropertySet;
}

static bool SetBlueprintVectorProperty(UObject* Object, FName PropertyName, const FVector& Value)
{
    if (!Object)
    {
        return false;
    }

    bool bPropertySet = false;
    for (TFieldIterator<FProperty> It(Object->GetClass(), EFieldIteratorFlags::IncludeSuper); It; ++It)
    {
        if (It->GetFName() == PropertyName)
        {
            if (FStructProperty* StructProperty = CastField<FStructProperty>(*It);
                StructProperty && StructProperty->Struct == TBaseStructure<FVector>::Get())
            {
                *StructProperty->ContainerPtrToValuePtr<FVector>(Object) = Value;
                bPropertySet = true;
            }
        }
    }
    return bPropertySet;
}

APhysicsVehicle::APhysicsVehicle()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    bAlwaysRelevant = true;
    SetReplicateMovement(true);

    PhysicsMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PhysicsMesh"));
    SetRootComponent(PhysicsMesh);
    PhysicsMesh->SetCollisionProfileName(TEXT("Vehicle"));
    PhysicsMesh->SetGenerateOverlapEvents(true);
    PhysicsMesh->BodyInstance.bSimulatePhysics = true;
    PhysicsMesh->BodyInstance.bLockTranslation = true;
    PhysicsMesh->BodyInstance.bLockRotation = true;
    PhysicsMesh->BodyInstance.bNotifyRigidBodyCollision = true;
    PhysicsMesh->BodyInstance.PositionSolverIterationCount = 8;
    PhysicsMesh->BodyInstance.VelocitySolverIterationCount = 1;

    VehicleInventory = CreateDefaultSubobject<UInventory>(TEXT("VehicleInventory"));

    static ConstructorHelpers::FObjectFinder<UCurveFloat> DefaultAccelerationCurve(
        TEXT("/Game/Blueprints/PhysicsVehicles/AccelerationCurve.AccelerationCurve"));
    if (DefaultAccelerationCurve.Succeeded())
    {
        AccelerationCurve = DefaultAccelerationCurve.Object;
    }
    else
    {
        UE_LOG(LogPhysicsVehicle, Warning, TEXT("Could not load the vehicle acceleration curve."));
    }
    static ConstructorHelpers::FObjectFinder<UCurveFloat> DefaultSteerCurve(
        TEXT("/Game/Blueprints/PhysicsVehicles/SteerCurve.SteerCurve"));
    static ConstructorHelpers::FObjectFinder<UCurveFloat> DefaultTurnCurve(
        TEXT("/Game/Blueprints/PhysicsVehicles/TurnCurve.TurnCurve"));
    static ConstructorHelpers::FObjectFinder<UCurveFloat> DefaultFrictionCurve(
        TEXT("/Game/Blueprints/PhysicsVehicles/FrictionCurve.FrictionCurve"));
    static ConstructorHelpers::FObjectFinder<UCurveFloat> DefaultAngularDamping(
        TEXT("/Game/Blueprints/PhysicsVehicles/AngularDampingCurve.AngularDampingCurve"));
    SteerCurve = DefaultSteerCurve.Object;
    TurnCurve = DefaultTurnCurve.Object;
    FrictionCurve = DefaultFrictionCurve.Object;
    AngularDamping = DefaultAngularDamping.Object;

    Wheel_FR_Pos = CreateDefaultSubobject<USceneComponent>(TEXT("Wheel_FR_Pos"));
    Wheel_FR_Pos->SetupAttachment(PhysicsMesh);
    Wheel_FR_Pos->SetRelativeLocation(FVector(185.0, 100.0, 5.0));

    Wheel_FL_Pos = CreateDefaultSubobject<USceneComponent>(TEXT("Wheel_FL_Pos"));
    Wheel_FL_Pos->SetupAttachment(PhysicsMesh);
    Wheel_FL_Pos->SetRelativeLocation(FVector(185.0, -100.0, 5.0));

    Wheel_RR_Pos = CreateDefaultSubobject<USceneComponent>(TEXT("Wheel_RR_Pos"));
    Wheel_RR_Pos->SetupAttachment(PhysicsMesh);
    Wheel_RR_Pos->SetRelativeLocation(FVector(-195.0, 105.0, 25.0));

    Wheel_RL_Pos = CreateDefaultSubobject<USceneComponent>(TEXT("Wheel_RL_Pos"));
    Wheel_RL_Pos->SetupAttachment(PhysicsMesh);
    Wheel_RL_Pos->SetRelativeLocation(FVector(-195.0, -105.0, 25.0));

    ProjectileSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ProjectileSpawnPoint"));
    ProjectileSpawnPoint->SetupAttachment(PhysicsMesh);
    ProjectileSpawnPoint->SetRelativeLocation(FVector(400.309147, 0.000065, 43.519684));
    ProjectileSpawnPoint->SetRelativeRotation(FRotator(0.0, 0.000009, 0.0));

    MissileSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("MissileSpawnPoint"));
    MissileSpawnPoint->SetupAttachment(PhysicsMesh);
    MissileSpawnPoint->SetRelativeLocation(FVector(525.0, 0.0, 160.0));

    TargetPoint = CreateDefaultSubobject<USceneComponent>(TEXT("TargetPoint"));
    TargetPoint->SetupAttachment(PhysicsMesh);
    TargetPoint->SetRelativeLocation(FVector(0.0, 0.0, 45.0));
    TargetPoint->ComponentTags.AddUnique(TEXT("TargetPointTag"));

    static ConstructorHelpers::FClassFinder<AActor> EngineAudioClass(
        TEXT("/Game/Blueprints/PhysicsVehicles/BPA_F1EngineAudio"));
    static ConstructorHelpers::FClassFinder<AActor> SkidAudioClass(
        TEXT("/Game/Blueprints/PhysicsVehicles/BPA_F1SkidAudio"));
    static ConstructorHelpers::FClassFinder<AActor> RadarManagerClass(TEXT("/Game/Blueprints/HUD/BP_RadarManager"));

    // Names must keep these substrings: InitializeBlueprintRuntimeComponents finds the actors by name.
    EngineAudioComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("BPA_F1EngineAudio"));
    EngineAudioComponent->SetupAttachment(PhysicsMesh);
    if (EngineAudioClass.Succeeded())
    {
        EngineAudioComponent->SetChildActorClass(EngineAudioClass.Class);
    }

    SkidAudioComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("BPA_F1SkidAudio"));
    SkidAudioComponent->SetupAttachment(PhysicsMesh);
    if (SkidAudioClass.Succeeded())
    {
        SkidAudioComponent->SetChildActorClass(SkidAudioClass.Class);
    }

    RadarManagerComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("BP_RadarManager"));
    RadarManagerComponent->SetupAttachment(PhysicsMesh);
    if (RadarManagerClass.Succeeded())
    {
        RadarManagerComponent->SetChildActorClass(RadarManagerClass.Class);
    }

    DroppedSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("DroppedSpawnPoint"));
    DroppedSpawnPoint->SetupAttachment(PhysicsMesh);
    DroppedSpawnPoint->SetRelativeLocation(FVector(-850.0, 0.0, 160.0));

    WheelMesh_FR = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelMesh_FR"));
    WheelMesh_FR->SetupAttachment(Wheel_FR_Pos);
    WheelMesh_FL = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelMesh_FL"));
    WheelMesh_FL->SetupAttachment(Wheel_FL_Pos);
    WheelMesh_RR = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelMesh_RR"));
    WheelMesh_RR->SetupAttachment(Wheel_RR_Pos);
    WheelMesh_RR->SetRelativeScale3D(FVector(1.6));
    WheelMesh_RL = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelMesh_RL"));
    WheelMesh_RL->SetupAttachment(Wheel_RL_Pos);
    WheelMesh_RL->SetRelativeScale3D(FVector(1.6));

    for (UStaticMeshComponent* WheelMesh : {WheelMesh_FR.Get(), WheelMesh_FL.Get(), WheelMesh_RR.Get(), WheelMesh_RL.Get()})
    {
        WheelMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
        WheelMesh->BodyInstance.bLockTranslation = true;
        WheelMesh->BodyInstance.bLockRotation = true;
    }

    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> SkidMarkSystem(
        TEXT("/Game/VFX/NS_SkidMark.NS_SkidMark"));
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> SkidSmokeSystem(
        TEXT("/Game/VFX/NS_SkidSmoke.NS_SkidSmoke"));
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> MainFireSystem(
        TEXT("/Game/VFX/NS_Flame.NS_Flame"));
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> FragmentsSystem(
        TEXT("/Game/VFX/NS_TriangleFragsBig.NS_TriangleFragsBig"));
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> ExplosionSystem(
        TEXT("/Game/VFX/NS_ExplosionBig.NS_ExplosionBig"));
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Muzzleflash(
        TEXT("/Game/VFX/NS_Muzzleflash.NS_Muzzleflash"));
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> RamImpactSystem(
        TEXT("/Game/VFX/NS_TriangleFrags.NS_TriangleFrags"));
    static ConstructorHelpers::FClassFinder<AActor> MachineGunBullet(
        TEXT("/Game/Blueprints/DamageActors/Projectiles/BP_Bullet"));
    static ConstructorHelpers::FClassFinder<AActor> ArmorRepairCauser(
        TEXT("/Game/Blueprints/DamageActors/Heal/BP_RepairKit"));
    static ConstructorHelpers::FClassFinder<AActor> HealthRepairCauser(
        TEXT("/Game/Blueprints/DamageActors/Heal/BP_MedKit"));
    static ConstructorHelpers::FClassFinder<AActor> MineWeapon(
        TEXT("/Game/Blueprints/DamageActors/Dropped/BP_Mine"));
    if (MineWeapon.Succeeded())
    {
        MineWeaponClass = MineWeapon.Class;
    }

    NS_Skidmark_FR = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NS_Skidmark_FR"));
    NS_Skidmark_FR->SetupAttachment(PhysicsMesh);
    NS_Skidmark_FR->SetRelativeLocation(FVector(190.0, 100.0, -15.0));
    NS_Skidmark_FR->SetAutoActivate(false);
    if (SkidMarkSystem.Succeeded())
    {
        NS_Skidmark_FR->SetAsset(SkidMarkSystem.Object);
    }

    NS_Skidmark_FL = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NS_Skidmark_FL"));
    NS_Skidmark_FL->SetupAttachment(PhysicsMesh);
    NS_Skidmark_FL->SetRelativeLocation(FVector(190.0, -100.0, -15.0));
    NS_Skidmark_FL->SetAutoActivate(false);
    if (SkidMarkSystem.Succeeded())
    {
        NS_Skidmark_FL->SetAsset(SkidMarkSystem.Object);
    }

    NS_Skidmark_RR = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NS_Skidmark_RR"));
    NS_Skidmark_RR->SetupAttachment(PhysicsMesh);
    NS_Skidmark_RR->SetRelativeLocation(FVector(-190.0, 100.0, -15.0));
    NS_Skidmark_RR->SetAutoActivate(false);
    if (SkidMarkSystem.Succeeded())
    {
        NS_Skidmark_RR->SetAsset(SkidMarkSystem.Object);
    }

    NS_Skidmark_RL = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NS_Skidmark_RL"));
    NS_Skidmark_RL->SetupAttachment(PhysicsMesh);
    NS_Skidmark_RL->SetRelativeLocation(FVector(-190.0, -100.0, -15.0));
    NS_Skidmark_RL->SetAutoActivate(false);
    if (SkidMarkSystem.Succeeded())
    {
        NS_Skidmark_RL->SetAsset(SkidMarkSystem.Object);
    }

    NS_Smoke_FR = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NS_Smoke_FR"));
    NS_Smoke_FR->SetupAttachment(NS_Skidmark_FR);
    NS_Smoke_FR->SetRelativeLocation(FVector(-190.0, -100.0, 15.0));
    NS_Smoke_FR->SetAutoActivate(false);
    if (SkidSmokeSystem.Succeeded())
    {
        NS_Smoke_FR->SetAsset(SkidSmokeSystem.Object);
    }

    NS_Smoke_FL = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NS_Smoke_FL"));
    NS_Smoke_FL->SetupAttachment(NS_Skidmark_FL);
    NS_Smoke_FL->SetRelativeLocation(FVector(-380.0, 0.0, 30.0));
    NS_Smoke_FL->SetAutoActivate(false);
    if (SkidSmokeSystem.Succeeded())
    {
        NS_Smoke_FL->SetAsset(SkidSmokeSystem.Object);
    }

    NS_Smoke_RR = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NS_Smoke_RR"));
    NS_Smoke_RR->SetupAttachment(NS_Skidmark_RR);
    NS_Smoke_RR->SetRelativeLocation(FVector::ZeroVector);
    NS_Smoke_RR->SetAutoActivate(false);
    if (SkidSmokeSystem.Succeeded())
    {
        NS_Smoke_RR->SetAsset(SkidSmokeSystem.Object);
    }

    NS_Smoke_RL = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NS_Smoke_RL"));
    NS_Smoke_RL->SetupAttachment(NS_Skidmark_RL);
    NS_Smoke_RL->SetRelativeLocation(FVector(0.0, 0.0, 30.0));
    NS_Smoke_RL->SetAutoActivate(false);
    if (SkidSmokeSystem.Succeeded())
    {
        NS_Smoke_RL->SetAsset(SkidSmokeSystem.Object);
    }

    NS_MainFire = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NS_MainFire"));
    NS_MainFire->SetupAttachment(PhysicsMesh);
    NS_MainFire->SetRelativeLocation(FVector(235.0, 0.0, 160.0));
    NS_MainFire->SetAutoActivate(false);
    if (MainFireSystem.Succeeded())
    {
        NS_MainFire->SetAsset(MainFireSystem.Object);
    }

    if (FragmentsSystem.Succeeded())
    {
        DeathFragmentsSystem = FragmentsSystem.Object;
    }
    if (ExplosionSystem.Succeeded())
    {
        DeathExplosionSystem = ExplosionSystem.Object;
    }
    if (Muzzleflash.Succeeded())
    {
        MuzzleflashSystem = Muzzleflash.Object;
    }
    if (RamImpactSystem.Succeeded())
    {
        RamImpactNiagaraSystem = RamImpactSystem.Object;
    }
    if (MachineGunBullet.Succeeded())
    {
        MachineGunBulletClass = MachineGunBullet.Class;
    }
    if (ArmorRepairCauser.Succeeded())
    {
        ArmorRepairCauserClass = ArmorRepairCauser.Class;
    }
    if (HealthRepairCauser.Succeeded())
    {
        HealthRepairCauserClass = HealthRepairCauser.Class;
    }

    if (!SkidMarkSystem.Succeeded())
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("Could not load NS_SkidMark."));
    }
    if (!SkidSmokeSystem.Succeeded())
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("Could not load NS_SkidSmoke."));
    }
    if (!MainFireSystem.Succeeded() || !FragmentsSystem.Succeeded() || !ExplosionSystem.Succeeded())
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("Could not load one or more vehicle death Niagara systems."));
    }
    if (!Muzzleflash.Succeeded() || !MachineGunBullet.Succeeded())
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("Could not load the machine gun projectile class or muzzleflash system."));
    }
    if (!RamImpactSystem.Succeeded())
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("Could not load the vehicle ram impact Niagara system."));
    }
    if (!ArmorRepairCauser.Succeeded() || !HealthRepairCauser.Succeeded())
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("Could not load one or more vehicle repair kit classes."));
    }
    if (!MineWeapon.Succeeded())
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("Could not load the mine weapon class."));
    }

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(PhysicsMesh);
    SpringArm->SetRelativeLocation(FVector(0.0, 0.0, 100.0));
    SpringArm->SetRelativeRotation(FRotator(-10.0, 0.0, 0.0));
    SpringArm->TargetArmLength = 1200.0f;
    SpringArm->bUsePawnControlRotation = false;

    ChaseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ChaseCamera"));
    ChaseCamera->SetupAttachment(SpringArm);
    ChaseCamera->SetRelativeLocation(FVector(-2400.0, 0.0, 0.0));
    ChaseCamera->SetFieldOfView(90.0f);
    ChaseCamera->SetActive(true);

    RearSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("RearSpringArm"));
    RearSpringArm->SetupAttachment(PhysicsMesh);
    RearSpringArm->SetRelativeLocation(FVector(0.0, 0.0, 100.0));
    RearSpringArm->SetRelativeRotation(FRotator(-10.0, 180.0, 0.0));
    RearSpringArm->TargetArmLength = 1200.0f;
    RearSpringArm->bUsePawnControlRotation = false;

    RearChaseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("RearChaseCamera"));
    RearChaseCamera->SetupAttachment(RearSpringArm);
    RearChaseCamera->SetRelativeLocation(FVector(-2400.0, 0.0, 0.0));
    RearChaseCamera->SetFieldOfView(90.0f);
    RearChaseCamera->SetAutoActivate(false);
    RearChaseCamera->SetActive(false);

    InCarCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("InCarCamera"));
    InCarCamera->SetupAttachment(PhysicsMesh);
    InCarCamera->SetRelativeLocation(FVector(30.0, 10.0, 80.0));
    InCarCamera->SetRelativeRotation(FRotator(-10.0, 0.0, 0.0));
    InCarCamera->SetFieldOfView(90.0f);
    InCarCamera->SetAutoActivate(false);
    InCarCamera->SetActive(false);

    EnemyHealthBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("EnemyHealthBar"));
    EnemyHealthBar->SetupAttachment(PhysicsMesh);
    EnemyHealthBar->SetRelativeLocation(FVector(-510.0, 0.0, 450.0));
    EnemyHealthBar->SetDrawSize(FVector2D(500.0, 200.0));
    EnemyHealthBar->SetWidgetSpace(EWidgetSpace::World);

    FrontCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("FrontCollisionBox"));
    FrontCollisionBox->SetupAttachment(PhysicsMesh);
    FrontCollisionBox->SetRelativeLocation(FVector(630.0, 0.0, 90.0));
    FrontCollisionBox->SetBoxExtent(FVector(32.0));
    FrontCollisionBox->SetCollisionProfileName(TEXT("OverlapAll"));
    FrontCollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    FrontCollisionBox->SetCollisionResponseToAllChannels(ECR_Overlap);
    FrontCollisionBox->SetGenerateOverlapEvents(true);
    FrontCollisionBox->SetHiddenInGame(true);

    static ConstructorHelpers::FClassFinder<UUserWidget> HealthBarClass(TEXT("/Game/UI/WB_EnemyHealthBar"));
    if (HealthBarClass.Succeeded())
    {
        EnemyHealthBar->SetWidgetClass(HealthBarClass.Class);
    }
    else
    {
        UE_LOG(LogPhysicsVehicle, Warning, TEXT("Could not load WB_EnemyHealthBar."));
    }
}

void APhysicsVehicle::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    SetTickGroup(TG_PostPhysics);

    if (HasAuthority())
    {
        CalculateVariables();
        if (!bIsDead)
        {
            HandleSuspensions();
            UpdateSteering(GetWorld()->GetDeltaSeconds());
            ApplyTurnTorque();
            ApplyLateralFriction();
            CalculateRearWheelSlip();
            UpdateAngularDamping();
            BrakeAndAccelerate();
        }
        HandleSkidEffects();
        UpdateVehicleAudio();
        double NearestTargetDistance = 0.0;
        AActor* FoundNearestTarget = nullptr;
        FindNearestTarget(NearestTargetDistance, FoundNearestTarget);
        NearestTarget = FoundNearestTarget;
        static_cast<void>(NearestTargetDistance);
        FireMachinegun();
        FireSelectedWeapon();
        if (MachineGunTimer < DeltaSeconds)
        {
            MachineGunTimer = 0.0;
        }
        else
        {
            MachineGunTimer -= DeltaSeconds;
        }
        if (WeaponTimer < DeltaSeconds)
        {
            WeaponTimer = 0.0;
        }
        else
        {
            WeaponTimer -= DeltaSeconds;
        }
        if (!bIsDead && Hp <= 0.0)
        {
            Die();
        }
    }
    if (IsLocallyControlled())
    {
        HandleCameras();
    }
    UpdateHealthBar();
}

void APhysicsVehicle::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    if (!PlayerInputComponent)
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("Cannot bind vehicle camera input without an input component."));
        return;
    }

    PlayerInputComponent->BindAxis(TEXT("LookRight"), this, &APhysicsVehicle::LookRight);
    PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &APhysicsVehicle::LookUp);
    PlayerInputComponent->BindAction(TEXT("ToggleCamera"), IE_Pressed, this, &APhysicsVehicle::ToggleVehicleCamera);
}

void APhysicsVehicle::ToggleVehicleCamera()
{
    bUsingRearCamera = !bUsingRearCamera;
    ChaseCamera->SetActive(!bUsingRearCamera);
    RearChaseCamera->SetActive(bUsingRearCamera);
}

void APhysicsVehicle::LookRight(float AxisValue)
{
    SpringArm->AddRelativeRotation(FRotator(0.0, AxisValue, 0.0));
}

void APhysicsVehicle::LookUp(float AxisValue)
{
    SpringArm->AddRelativeRotation(FRotator(AxisValue, 0.0, 0.0));
}

void APhysicsVehicle::CalculateVariables()
{
    LinearVelocity = PhysicsMesh->GetPhysicsLinearVelocity();
    CurrentSpeed = FVector::DotProduct(PhysicsMesh->GetComponentVelocity(), PhysicsMesh->GetForwardVector());
    const double CarSpeed = LinearVelocity.Size() > 0.0
        ? FVector::DotProduct(PhysicsMesh->GetForwardVector(), LinearVelocity)
        : 0.01;
    NormalizedSpeed = FMath::Clamp(UKismetMathLibrary::Divide_DoubleDouble(FMath::Abs(CarSpeed), MaxSpeed), 0.0, 1.0);
    if (RamTimer < 10.0)
    {
        RamTimer += GetWorld()->GetDeltaSeconds();
    }
}

void APhysicsVehicle::HandleSuspensions()
{
    CalculateWheelSuspension(Wheel_FR_Pos, WheelMesh_FR, FrontSuspensionRestDistance, FrontWheelRadius, bGroundedFR, ForceFR);
    CalculateWheelSuspension(Wheel_FL_Pos, WheelMesh_FL, FrontSuspensionRestDistance, FrontWheelRadius, bGroundedFL, ForceFL);
    CalculateWheelSuspension(Wheel_RL_Pos, WheelMesh_RL, RearSuspensionRestDistance, RearWheelRadius, bGroundedRL, ForceRL);
    CalculateWheelSuspension(Wheel_RR_Pos, WheelMesh_RR, RearSuspensionRestDistance, RearWheelRadius, bGroundedRR, ForceRR);
}

void APhysicsVehicle::CalculateWheelSuspension(USceneComponent* Wheel, UStaticMeshComponent* WheelMesh,
    double RestDistance, double WheelRadius, bool& bGrounded, double& Force)
{
    const FVector WheelPosition = Wheel->GetComponentLocation();
    const FVector SuspensionDirection = Wheel->GetUpVector();
    const FVector WheelVelocity = PhysicsMesh->GetPhysicsLinearVelocityAtPoint(WheelPosition);
    FHitResult Hit;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(VehicleSuspension), true, this);
    bGrounded = GetWorld()->LineTraceSingleByProfile(Hit, WheelPosition,
        WheelPosition - SuspensionDirection * RestDistance, TEXT("BlockAll"), QueryParams);

    FVector WheelMeshLocation;
    Force = 0.0;
    if (bGrounded)
    {
        WheelMeshLocation = Hit.ImpactPoint + SuspensionDirection * WheelRadius;
        const double Offset = RestDistance - Hit.Distance;
        const double Velocity = FVector::DotProduct(SuspensionDirection, WheelVelocity);
        const double SpringForce = FMath::Clamp(Offset * SpringStrength - Velocity * SpringDamping, 0.0, SpringStrength * 50.0);
        const FVector ForceVector = SuspensionDirection * SpringForce;
        PhysicsMesh->AddForceAtLocation(ForceVector, WheelPosition);
        Force = ForceVector.Size();

        if (bDebug)
        {
            UKismetSystemLibrary::DrawDebugArrow(this, WheelPosition, WheelPosition + ForceVector * 0.01,
                100.0f, FLinearColor(0.0f, 0.135226f, 1.0f, 1.0f), 0.0f, 5.0f);
        }
    }
    else
    {
        WheelMeshLocation = WheelPosition - SuspensionDirection * (RestDistance - WheelRadius);
    }
    WheelMesh->SetWorldLocation(WheelMeshLocation, false, nullptr, ETeleportType::None);
}

void APhysicsVehicle::Server_SetSteeringAngle_Implementation(double NewSteeringAngle, bool IsSteering)
{
    TargetSteeringAngle = NewSteeringAngle;
    bIsSteering = IsSteering;
}

void APhysicsVehicle::SetVehicleAcceleration(double NewAcceleration)
{
    if (IsLocallyControlled())
    {
        Server_SetAcceleration(NewAcceleration);
    }
}

void APhysicsVehicle::SetVehicleBrake(double NewBrake, bool Handbrake)
{
    if (IsLocallyControlled())
    {
        Server_SetBrake(NewBrake, Handbrake);
    }
}

void APhysicsVehicle::Server_SetAcceleration_Implementation(double NewAcceleration)
{
    TargetAcceleration = NewAcceleration;
}

void APhysicsVehicle::Server_SetBrake_Implementation(double NewBrake, bool Handbrake)
{
    TargetBrake = NewBrake;
    bHandbrake = Handbrake;
}

void APhysicsVehicle::Server_SendTransform_Implementation()
{
    Multicast_UpdateTransform(PhysicsMesh->GetComponentLocation(), PhysicsMesh->GetComponentRotation());
}

void APhysicsVehicle::Multicast_UpdateTransform_Implementation(FVector NewLocation, FRotator NewRotation)
{
    ReplicatedLocation = NewLocation;
    ReplicatedRotation = NewRotation;
}

void APhysicsVehicle::UpdateSteering(double DeltaSeconds)
{
    const double SteeringSpeed = bIsSteering ? 6.0 : 12.0;
    CurrentSteeringAngle = UKismetMathLibrary::FInterpTo(CurrentSteeringAngle, TargetSteeringAngle, DeltaSeconds, SteeringSpeed);
    const float Alpha = static_cast<float>((CurrentSteeringAngle + 1.0) / 2.0);
    const FRotator Rotation = UKismetMathLibrary::RLerp(FRotator(0.0, -45.0, 0.0), FRotator(0.0, 45.0, 0.0), Alpha, false);
    Wheel_FR_Pos->SetRelativeRotation(Rotation);
    Wheel_FL_Pos->SetRelativeRotation(Rotation);
}

bool APhysicsVehicle::CheckHandlingCurve(const UCurveFloat* Curve, const TCHAR* CurveName, bool& bReportedMissing)
{
    if (!Curve)
    {
        if (!bReportedMissing)
        {
            UE_LOG(LogPhysicsVehicle, Error, TEXT("%s is missing on %s."), CurveName, *GetName());
            bReportedMissing = true;
        }
        return false;
    }
    bReportedMissing = false;
    return true;
}

void APhysicsVehicle::ApplyTurnTorque()
{
    const bool bHasSteerCurve = CheckHandlingCurve(SteerCurve, TEXT("SteerCurve"), bReportedMissingSteerCurve);
    const bool bHasTurnCurve = CheckHandlingCurve(TurnCurve, TEXT("TurnCurve"), bReportedMissingTurnCurve);
    if (!bHasSteerCurve || !bHasTurnCurve)
    {
        return;
    }

    const double Sign = CurrentSpeed >= 0.0 ? 1.0 : -1.0;
    const double Torque = static_cast<double>(SteerCurve->GetFloatValue(static_cast<float>(CurrentSteeringAngle)))
        * static_cast<double>(TurnCurve->GetFloatValue(static_cast<float>(NormalizedSpeed))) * 500.0
        * (bGroundedRL ? 1.0 : 0.0) * Sign;
    PhysicsMesh->AddTorqueInDegrees(FVector(0.0, 0.0, Torque), NAME_None, true);
}

void APhysicsVehicle::ApplyLateralFriction()
{
    if (!CheckHandlingCurve(FrictionCurve, TEXT("FrictionCurve"), bReportedMissingFrictionCurve))
    {
        return;
    }

    const FVector Right = PhysicsMesh->GetRightVector();
    const FVector Force = Right * FVector::DotProduct(LinearVelocity, Right) * -1.0
        * FrictionCurve->GetFloatValue(static_cast<float>(NormalizedSpeed)) * (bGroundedRL ? 1.0 : 0.0);
    PhysicsMesh->AddForce(Force);
    if (bDebug)
    {
        const FVector Start = PhysicsMesh->GetComponentLocation() + Right;
        UKismetSystemLibrary::DrawDebugArrow(this, Start, Start + Force * 0.01,
            100.0f, FLinearColor(0.942751f, 1.0f, 0.0f, 1.0f), 0.0f, 5.0f);
    }
}

void APhysicsVehicle::CalculateRearWheelSlip()
{
    const FVector RearPosition = (Wheel_RR_Pos->GetComponentLocation() + Wheel_RL_Pos->GetComponentLocation()) * 0.5;
    const FVector RearVelocity = PhysicsMesh->GetPhysicsLinearVelocityAtPoint(RearPosition);
    WheelSlipAmount = FMath::Abs(FVector::DotProduct(RearVelocity, PhysicsMesh->GetRightVector())) * Acceleration;
}

void APhysicsVehicle::UpdateAngularDamping()
{
    if (!CheckHandlingCurve(AngularDamping, TEXT("AngularDamping"), bReportedMissingAngularDamping))
    {
        return;
    }

    const double Factor = Acceleration > 0.8 && NormalizedSpeed < 0.3 ? 0.2 : 1.0;
    const double Damping = bGroundedRR
        ? AngularDamping->GetFloatValue(static_cast<float>(NormalizedSpeed)) * Factor
        : 0.05;
    PhysicsMesh->SetAngularDamping(static_cast<float>(Damping));
}

void APhysicsVehicle::CheckUpsideDown()
{
    if (PhysicsMesh->GetUpVector().Z > 0.1)
    {
        return;
    }

    if (UpsideDownTimer < MaxUpsideDownTime)
    {
        UpsideDownTimer += GetWorld()->GetDeltaSeconds();
    }
    else
    {
        SetActorRotation(FRotator(0.0, PhysicsMesh->GetComponentRotation().Yaw, 0.0), ETeleportType::TeleportPhysics);
        SetActorLocation(PhysicsMesh->GetComponentLocation() + FVector(0.0, 0.0, 1000.0),
            false, nullptr, ETeleportType::TeleportPhysics);
        UpsideDownTimer = 0.0;
    }
}

void APhysicsVehicle::HandleCameras()
{
    SpringArmRoll = -PhysicsMesh->GetComponentRotation().Roll;
    if (bIsDead)
    {
        SpringArm->bInheritPitch = false;
        SpringArm->bInheritYaw = false;
        SpringArm->bInheritRoll = false;
        SpringArmPitch = -30.0;
        SpringArmYaw = SpringArm->GetRelativeRotation().Yaw + 0.5;
        SpringArmRoll = 0.0;
        SpringArm->TargetArmLength = 2000.0f;
    }
    else
    {
        if (NormalizedSpeed > 0.1)
        {
            SpringArmPitch = UKismetMathLibrary::MapRangeClamped(NormalizedSpeed, 0.5, 1.0, -10.0, -12.0);
            const double SpeedYaw = UKismetMathLibrary::MapRangeClamped(NormalizedSpeed, 0.1, 1.0, 0.0, 10.0);
            const double SteeringYaw = UKismetMathLibrary::MapRangeClamped(CurrentSteeringAngle, -1.0, 1.0, 1.0, -1.0);
            SpringArmYaw = SpeedYaw * SteeringYaw;
            const double LocationX = UKismetMathLibrary::MapRangeClamped(NormalizedSpeed, 0.5, 1.0, -0.5, -75.0);
            SpringArm->SetRelativeLocation(FVector(LocationX, 0.0, 100.0));
        }
        else
        {
            SpringArmPitch = -10.0;
            SpringArmYaw = 0.0;
        }

        SpringArmPitch = UKismetMathLibrary::MapRangeClamped(
            SpringArm->GetComponentRotation().Pitch, -10.0, 5.0, -10.0, -20.0);
    }
    SpringArm->SetRelativeRotation(FRotator(SpringArmPitch, SpringArmYaw, SpringArmRoll));
}

void APhysicsVehicle::BrakeAndAccelerate()
{
    USceneComponent* Wheels[] = {Wheel_RL_Pos, Wheel_RR_Pos, Wheel_FL_Pos, Wheel_FR_Pos};
    const bool Grounded[] = {bGroundedFL, bGroundedFR, bGroundedRL, bGroundedRR};
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Wheels); ++Index)
    {
        BrakeAtWheel(Wheels[Index], Grounded[Index], Index > 1 ? 2.0 : 0.3);
        // The Blueprint continues from the loop body, not its Completed pin.
        if (Accelerate())
        {
            RollWheels();
            if (!bGroundedRL)
            {
                PhysicsMesh->AddForce(FVector::DownVector * PhysicsMesh->GetMass() * 9.81 * 1.2, NAME_None, true);
                CheckUpsideDown();
            }
        }
    }
}

void APhysicsVehicle::BrakeAtWheel(USceneComponent* Wheel, bool bGrounded, double Multiplier)
{
    if (!bGrounded)
    {
        return;
    }

    const FVector Force = -LinearVelocity.GetSafeNormal(0.0001f) * TargetBrake * 80000.0 * Multiplier;
    const FVector Position = Wheel->GetComponentLocation();
    PhysicsMesh->AddForceAtLocation(Force, Position);
    if (bDebug)
    {
        UKismetSystemLibrary::DrawDebugArrow(this, Position, Position + Force * 0.01,
            100.0f, FLinearColor(1.0f, 0.0f, 0.838575f, 1.0f), 0.0f, 5.0f);
    }
}

bool APhysicsVehicle::Accelerate()
{
    Acceleration = UKismetMathLibrary::FInterpTo(Acceleration, TargetAcceleration, GetWorld()->GetDeltaSeconds(), 6.0);
    switch (WheelDriveType)
    {
    case EPhysicsVehicleWheelDrive::FrontWheelDrive:
        AccelerateAtWheel(Wheel_FR_Pos, bGroundedFR);
        AccelerateAtWheel(Wheel_FL_Pos, bGroundedFL);
        return true;
    case EPhysicsVehicleWheelDrive::RearWheelDrive:
        AccelerateAtWheel(Wheel_RR_Pos, bGroundedRR);
        AccelerateAtWheel(Wheel_RL_Pos, bGroundedRL);
        return true;
    case EPhysicsVehicleWheelDrive::AllWheelDrive:
        AccelerateAtWheel(Wheel_FR_Pos, bGroundedFR);
        AccelerateAtWheel(Wheel_FL_Pos, bGroundedFL);
        AccelerateAtWheel(Wheel_RR_Pos, bGroundedRR);
        AccelerateAtWheel(Wheel_RL_Pos, bGroundedRL);
        return true;
    default:
        UE_LOG(LogPhysicsVehicle, Error, TEXT("Invalid wheel drive type on %s."), *GetName());
        return false;
    }
}

void APhysicsVehicle::AccelerateAtWheel(USceneComponent* Wheel, bool bGrounded)
{
    if (!bGrounded)
    {
        return;
    }
    if (!AccelerationCurve)
    {
        if (!bReportedMissingAccelerationCurve)
        {
            UE_LOG(LogPhysicsVehicle, Error, TEXT("Acceleration curve is missing on %s."), *GetName());
            bReportedMissingAccelerationCurve = true;
        }
        return;
    }
    bReportedMissingAccelerationCurve = false;

    const FVector Direction = Wheel->GetForwardVector();
    const FVector Force = Direction * (AccelerationCurve->GetFloatValue(static_cast<float>(NormalizedSpeed)) * Acceleration) * MaxAcceleration;
    if (CurrentSpeed >= 0.0)
    {
        CurrentSpeed *= FVector::DotProduct(Force, Direction) > 0.0 ? 1.0 : -1.0;
    }
    const FVector Position = Wheel->GetComponentLocation();
    if (LinearVelocity.Size() < MaxSpeed)
    {
        PhysicsMesh->AddForceAtLocation(Force, Position);
    }
    if (bDebug)
    {
        UKismetSystemLibrary::DrawDebugArrow(this, Position, Position + Force * 0.1,
            100.0f, FLinearColor::Red, 0.0f, 5.0f);
    }
}

void APhysicsVehicle::RollWheels()
{
    RollWheel(WheelMesh_FR, Wheel_FR_Pos, PreviousWheelLocation_FR);
    RollWheel(WheelMesh_FL, Wheel_FL_Pos, PreviousWheelLocation_FL);
    RollWheel(WheelMesh_RL, Wheel_RL_Pos, PreviousWheelLocation_RL);
    RollWheel(WheelMesh_RR, Wheel_RR_Pos, PreviousWheelLocation_RR);
}

void APhysicsVehicle::RollWheel(UStaticMeshComponent* WheelMesh, USceneComponent* WheelPosition, FVector& PreviousLocation)
{
    const FVector CurrentLocation = WheelPosition->GetComponentLocation();
    const double Distance = FVector::DotProduct(CurrentLocation - PreviousLocation, WheelPosition->GetForwardVector());
    WheelMesh->AddLocalRotation(UKismetMathLibrary::NegateRotator(FRotator(Distance, 0.0, 0.0)));
    PreviousLocation = CurrentLocation;
}

void APhysicsVehicle::BeginPlay()
{
    Super::BeginPlay();

    CheckMeshes();

    if (FrontCollisionBox)
    {
        FrontCollisionBox->OnComponentBeginOverlap.AddDynamic(this, &APhysicsVehicle::HandleFrontCollisionOverlap);
    }
    else
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("Vehicle ram collision box is missing."));
    }

    TArray<USceneComponent*> SceneComponents;
    GetComponents<USceneComponent>(SceneComponents);
    for (USceneComponent* Component : SceneComponents)
    {
        if (Component && Component->GetName().Contains(TEXT("ContactPoint"), ESearchCase::IgnoreCase))
        {
            WheelsContactPointArray.Add(Component);
        }
    }

    TArray<AActor*> VehicleActors;
    UGameplayStatics::GetAllActorsOfClass(this, APhysicsVehicle::StaticClass(), VehicleActors);
    TargetList.Reset(VehicleActors.Num());
    for (AActor* VehicleActor : VehicleActors)
    {
        if (VehicleActor != this)
        {
            TargetList.Add(VehicleActor);
        }
    }

    Hp = MaxHp;
    OnRep_Hp();
    Armor = MaxArmor;
    OnRep_Armor();

    if (!GetClass()->IsFunctionImplementedInScript(FName(TEXT("ReceiveAnyDamage"))))
    {
        OnTakeAnyDamage.AddDynamic(this, &APhysicsVehicle::HandleAnyDamage);
    }

    InitializeBlueprintRuntimeComponents();

    NetUpdateFrequency = 66.0f;
    MinNetUpdateFrequency = 33.0f;
    GetWorldTimerManager().SetTimer(
        TransformUpdateTimerHandle, this, &APhysicsVehicle::Server_SendTransform, 0.05f, true);
}

void APhysicsVehicle::InitializeBlueprintRuntimeComponents()
{
    TArray<UChildActorComponent*> ChildActorComponents;
    GetComponents<UChildActorComponent>(ChildActorComponents);
    for (UChildActorComponent* Component : ChildActorComponents)
    {
        if (!Component)
        {
            continue;
        }

        const FString ComponentName = Component->GetName();
        if (ComponentName.Contains(TEXT("BPA_F1EngineAudio"), ESearchCase::IgnoreCase))
        {
            EngineAudioActor = Component->GetChildActor();
        }
        else if (ComponentName.Contains(TEXT("BPA_F1SkidAudio"), ESearchCase::IgnoreCase))
        {
            SkidAudioActor = Component->GetChildActor();
        }
        else if (ComponentName.Contains(TEXT("BP_RadarManager"), ESearchCase::IgnoreCase))
        {
            RadarManagerActor = Component->GetChildActor();
        }
    }

    TArray<UActorComponent*> ActorComponents;
    GetComponents<UActorComponent>(ActorComponents);
    for (UActorComponent* Component : ActorComponents)
    {
        if (Component && Component->GetName().Contains(TEXT("FrontObstructionSensing"), ESearchCase::IgnoreCase))
        {
            FrontObstructionSensorComponent = Component;
            break;
        }
    }

    TArray<UWidgetComponent*> WidgetComponents;
    GetComponents<UWidgetComponent>(WidgetComponents);
    UWidgetComponent* InventoryWidget = nullptr;
    for (UWidgetComponent* Component : WidgetComponents)
    {
        if (Component && Component->GetName().Contains(TEXT("BP_Inventory"), ESearchCase::IgnoreCase))
        {
            InventoryWidget = Component;
            break;
        }
    }

    const bool bHasBlueprintVehicleComponents = GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint);
    if (bHasBlueprintVehicleComponents)
    {
        if (!EngineAudioActor || !SkidAudioActor || !RadarManagerActor || !FrontObstructionSensorComponent ||
            !InventoryWidget)
        {
            UE_LOG(LogPhysicsVehicle, Warning,
                TEXT("One or more Blueprint vehicle runtime components could not be resolved on %s."),
                *GetName());
        }
    }

    if (RadarManagerActor)
    {
        FObjectPropertyBase* OwningActorProperty =
            FindFProperty<FObjectPropertyBase>(RadarManagerActor->GetClass(), TEXT("OwningActor"));
        if (OwningActorProperty && this->IsA(OwningActorProperty->PropertyClass))
        {
            OwningActorProperty->SetObjectPropertyValue_InContainer(RadarManagerActor.Get(), this);
        }
        else
        {
            UE_LOG(LogPhysicsVehicle, Error,
                TEXT("Radar manager on %s has no compatible OwningActor property."), *GetName());
        }
    }

    if (FrontObstructionSensorComponent)
    {
        FObjectPropertyBase* SensorProperty =
            FindFProperty<FObjectPropertyBase>(GetClass(), TEXT("FrontObstructionSensor"));
        if (SensorProperty && FrontObstructionSensorComponent->IsA(SensorProperty->PropertyClass))
        {
            SensorProperty->SetObjectPropertyValue_InContainer(this, FrontObstructionSensorComponent.Get());
        }
        else if (bHasBlueprintVehicleComponents)
        {
            UE_LOG(LogPhysicsVehicle, Error,
                TEXT("Vehicle %s has no compatible FrontObstructionSensor property."), *GetName());
        }
    }

    if (!InventoryWidget)
    {
        return;
    }

    const bool bShowInventory = IsPlayerControlled();
    FBoolProperty* ShowInventoryProperty =
        FindFProperty<FBoolProperty>(InventoryWidget->GetClass(), TEXT("bMustShowInventory"));
    if (ShowInventoryProperty)
    {
        ShowInventoryProperty->SetPropertyValue_InContainer(InventoryWidget, bShowInventory);
    }
    else
    {
        UE_LOG(LogPhysicsVehicle, Error,
            TEXT("Inventory widget component on %s has no bMustShowInventory property."), *GetName());
    }

    UUserWidget* InventoryUserWidget = InventoryWidget->GetUserWidgetObject();
    if (InventoryUserWidget)
    {
        InventoryUserWidget->SetVisibility(bShowInventory ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }
    else
    {
        UE_LOG(LogPhysicsVehicle, Warning,
            TEXT("Inventory widget on %s is not initialized; its visibility was not updated."), *GetName());
    }
}

void APhysicsVehicle::UpdateVehicleAudio()
{
    NormalizedSkid = FMath::Clamp((WheelSlipAmount - 200.0) / 3000.0, 0.0, 1.0)
        * (bGroundedRR ? 1.0 : 0.0);

    const double OnLoad = TargetAcceleration > 0.0 ? 1.0 : 0.0;
    const bool bRevLimiter = (Acceleration > 0.0 && !bGroundedRR)
        || (bGroundedRR && NormalizedSkid > 0.6);
    const double RevLimiter = bRevLimiter ? 1.0 : 0.0;

    if (EngineAudioActor)
    {
        const bool bUpdatedOnLoad = SetBlueprintRealProperty(EngineAudioActor, TEXT("OnLoad"), OnLoad);
        const bool bUpdatedRevLimiter = SetBlueprintRealProperty(EngineAudioActor, TEXT("RevLimiter"), RevLimiter);
        if ((!bUpdatedOnLoad || !bUpdatedRevLimiter) && !bReportedMissingEngineAudioProperties)
        {
            UE_LOG(LogPhysicsVehicle, Error,
                TEXT("Engine audio actor on %s is missing compatible OnLoad or RevLimiter properties."),
                *GetName());
            bReportedMissingEngineAudioProperties = true;
        }
    }

    if (SkidAudioActor && !SetBlueprintRealProperty(SkidAudioActor, TEXT("Skid"), NormalizedSkid)
        && !bReportedMissingSkidAudioProperty)
    {
        UE_LOG(LogPhysicsVehicle, Error,
            TEXT("Skid audio actor on %s is missing a compatible Skid property."), *GetName());
        bReportedMissingSkidAudioProperty = true;
    }
}

void APhysicsVehicle::HandleFrontCollisionOverlap(UPrimitiveComponent*, AActor* OtherActor,
    UPrimitiveComponent*, int32, bool, const FHitResult&)
{
    if (!HasAuthority() || bIsDead || NormalizedSpeed <= 0.3 || RamTimer <= 1.0)
    {
        return;
    }

    APhysicsVehicle* OtherVehicle = Cast<APhysicsVehicle>(OtherActor);
    if (!OtherVehicle || !PhysicsMesh || !FrontCollisionBox)
    {
        return;
    }

    const FVector DirectionToOther = UKismetMathLibrary::Normal(
        OtherVehicle->GetActorLocation() - PhysicsMesh->GetComponentLocation(), 0.0001f);
    const double DirectionFactor = FVector::DotProduct(PhysicsMesh->GetForwardVector(), DirectionToOther);
    const double Damage = DirectionFactor * NormalizedSpeed * RamBaseDamage;
    AController* EventInstigator = UGameplayStatics::GetPlayerController(this, 0);
    UGameplayStatics::ApplyDamage(OtherVehicle, static_cast<float>(Damage), EventInstigator, this,
        UDamageType::StaticClass());

    RamTimer = 0.0;
    if (RamImpactNiagaraSystem)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, RamImpactNiagaraSystem.Get(),
            FrontCollisionBox->GetComponentLocation(), FRotator::ZeroRotator,
            FVector::OneVector, true, true, ENCPoolMethod::None, true);
    }
    else
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("Vehicle ram impact system is not configured."));
    }
}

void APhysicsVehicle::CheckMeshes()
{
    bool bMissingMeshAsset = false;

    if (!PhysicsMesh || !PhysicsMesh->GetSkeletalMeshAsset())
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("SkeletelMesh null"));
        bMissingMeshAsset = true;
    }

    const UStaticMeshComponent* WheelMeshes[] = {
        WheelMesh_FL.Get(), WheelMesh_RL.Get(), WheelMesh_RR.Get(), WheelMesh_FR.Get()
    };
    for (const UStaticMeshComponent* WheelMesh : WheelMeshes)
    {
        if (!WheelMesh || !WheelMesh->GetStaticMesh())
        {
            UE_LOG(LogPhysicsVehicle, Error, TEXT("WheelMesh null"));
            bMissingMeshAsset = true;
        }
    }

    if (bMissingMeshAsset)
    {
        UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
    }
}

void APhysicsVehicle::FindNearestTarget(double& Distance, AActor*& NearestActor) const
{
    Distance = 0.0;
    NearestActor = nullptr;
    if (TargetList.Num() == 0)
    {
        return;
    }

    TArray<AActor*> ActorsToCheck;
    ActorsToCheck.Reserve(TargetList.Num());
    for (const TObjectPtr<AActor>& Target : TargetList)
    {
        ActorsToCheck.Add(Target.Get());
    }

    float NearestDistance = 0.0f;
    NearestActor = UGameplayStatics::FindNearestActor(GetActorLocation(), ActorsToCheck, NearestDistance);
    Distance = static_cast<double>(NearestDistance);
}

void APhysicsVehicle::RequestMachineGunFire()
{
    if (HasAuthority())
    {
        bFireMachinegun = true;
    }
    else if (IsLocallyControlled())
    {
        Server_RequestMachineGunFire();
    }
}

void APhysicsVehicle::Server_RequestMachineGunFire_Implementation()
{
    bFireMachinegun = true;
}

void APhysicsVehicle::RequestSelectedWeaponFire()
{
    if (HasAuthority())
    {
        bFireSelectedWeapon = true;
    }
    else if (IsLocallyControlled())
    {
        Server_RequestSelectedWeaponFire();
    }
}

void APhysicsVehicle::Server_RequestSelectedWeaponFire_Implementation()
{
    bFireSelectedWeapon = true;
}

void APhysicsVehicle::FireMachinegun()
{
    if (!HasAuthority() || MachineGunTimer != 0.0 || !bFireMachinegun)
    {
        return;
    }

    if (!ProjectileSpawnPoint || !GetWorld())
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("Cannot fire machine gun without a projectile spawn point and world."));
    }
    else
    {
        const FVector SpawnLocation = ProjectileSpawnPoint->GetComponentLocation();
        const FRotator SpawnRotation = ProjectileSpawnPoint->GetComponentRotation();
        if (MuzzleflashSystem)
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, MuzzleflashSystem.Get(),
                SpawnLocation, FRotator(SpawnRotation.Pitch, SpawnRotation.Yaw + 270.0, SpawnRotation.Roll),
                FVector::OneVector, true, true, ENCPoolMethod::None, true);
        }
        else
        {
            UE_LOG(LogPhysicsVehicle, Error, TEXT("Machine gun muzzleflash system is not configured."));
        }

        if (MachineGunBulletClass)
        {
            const FTransform SpawnTransform = ProjectileSpawnPoint->GetComponentTransform();
            AActor* Bullet = UGameplayStatics::BeginDeferredActorSpawnFromClass(this, MachineGunBulletClass,
                SpawnTransform, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn,
                nullptr, ESpawnActorScaleMethod::OverrideRootScale);
            if (Bullet)
            {
                FNumericProperty* DamageProperty = CastField<FNumericProperty>(
                    FindFProperty<FProperty>(Bullet->GetClass(), TEXT("Damage")));
                if (!DamageProperty || !DamageProperty->IsFloatingPoint())
                {
                    UE_LOG(LogPhysicsVehicle, Error, TEXT("Machine gun projectile has no floating-point Damage property."));
                    Bullet->Destroy();
                }
                else
                {
                    DamageProperty->SetFloatingPointPropertyValue(
                        DamageProperty->ContainerPtrToValuePtr<void>(Bullet), 2.0);
                    UGameplayStatics::FinishSpawningActor(
                        Bullet, SpawnTransform, ESpawnActorScaleMethod::OverrideRootScale);
                }
            }
            else
            {
                UE_LOG(LogPhysicsVehicle, Error, TEXT("Failed to spawn the machine gun projectile."));
            }
        }
        else
        {
            UE_LOG(LogPhysicsVehicle, Error, TEXT("Machine gun projectile class is not configured."));
        }
    }

    MachineGunTimer = MaxMachineGunTime;
    bFireMachinegun = false;
}

void APhysicsVehicle::FireSelectedWeapon()
{
    if (!HasAuthority() || !bFireSelectedWeapon)
    {
        return;
    }

    bFireSelectedWeapon = false;
    if (WeaponTimer != 0.0)
    {
        return;
    }

    if (!VehicleInventory)
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("Vehicle has no inventory component."));
        bFireSelectedWeapon = false;
        return;
    }

    if (VehicleInventory->Weapons.IsEmpty())
    {
        return;
    }

    if (!VehicleInventory->Weapons.IsValidIndex(VehicleInventory->InventoryIndex))
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("Inventory index is outside the weapon array."));
        bFireSelectedWeapon = false;
        return;
    }

    const FWeaponStruct& SelectedWeapon = VehicleInventory->Weapons[VehicleInventory->InventoryIndex];
    if (SelectedWeapon.Quantity <= 0)
    {
        return;
    }
    UClass* WeaponClass = SelectedWeapon.Weapon.Get();
    if (!WeaponClass || !WeaponClass->IsChildOf(AActor::StaticClass()))
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("Selected inventory item has no valid weapon actor class."));
        bFireSelectedWeapon = false;
        return;
    }

    const bool bIsMine = MineWeaponClass && WeaponClass == MineWeaponClass.Get();
    USceneComponent* WeaponSpawnPoint = bIsMine ? DroppedSpawnPoint.Get() : MissileSpawnPoint.Get();
    if (!WeaponSpawnPoint || !GetWorld())
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("Selected weapon spawn point or world is unavailable."));
        bFireSelectedWeapon = false;
        return;
    }

    FNumericProperty* DamageProperty = CastField<FNumericProperty>(
        FindFProperty<FProperty>(WeaponClass, TEXT("Damage")));
    FObjectPropertyBase* TargetActorProperty = CastField<FObjectPropertyBase>(
        FindFProperty<FProperty>(WeaponClass, TEXT("TargetActor")));
    if (!DamageProperty || !DamageProperty->IsFloatingPoint() || !TargetActorProperty)
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("Selected weapon class must expose floating-point Damage and object TargetActor properties."));
        bFireSelectedWeapon = false;
        return;
    }

    double NearestTargetDistance = 0.0;
    AActor* ClosestTargetActor = nullptr;
    FindNearestTarget(NearestTargetDistance, ClosestTargetActor);

    if (ProjectileSpawnPoint && MuzzleflashSystem)
    {
        const FVector MuzzleLocation = ProjectileSpawnPoint->GetComponentLocation();
        const FRotator MuzzleRotation = ProjectileSpawnPoint->GetComponentRotation();
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, MuzzleflashSystem.Get(),
            MuzzleLocation, FRotator(MuzzleRotation.Pitch, MuzzleRotation.Yaw + 270.0, MuzzleRotation.Roll),
            FVector::OneVector, true, true, ENCPoolMethod::None, true);
    }

    const FTransform SpawnTransform = WeaponSpawnPoint->GetComponentTransform();
    AActor* SpawnedWeapon = GetWorld()->SpawnActorDeferred<AActor>(
        WeaponClass, SpawnTransform, nullptr, this,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn,
        ESpawnActorScaleMethod::SelectDefaultAtRuntime);
    if (!SpawnedWeapon)
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("Failed to spawn selected weapon class '%s'."), *WeaponClass->GetName());
        bFireSelectedWeapon = false;
        return;
    }

    DamageProperty->SetFloatingPointPropertyValue(
        DamageProperty->ContainerPtrToValuePtr<void>(SpawnedWeapon), 0.0);
    TargetActorProperty->SetObjectPropertyValue_InContainer(SpawnedWeapon, ClosestTargetActor);
    SpawnedWeapon = UGameplayStatics::FinishSpawningActor(
        SpawnedWeapon, SpawnTransform, ESpawnActorScaleMethod::SelectDefaultAtRuntime);
    if (!SpawnedWeapon)
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("Selected weapon was destroyed during spawn construction."));
        bFireSelectedWeapon = false;
        return;
    }

    VehicleInventory->SubtractItem();
    VehicleInventory->UpdateCurrentWeapon.Broadcast();
    VehicleInventory->WeaponChanged.Broadcast();
    WeaponTimer = MaxWeaponTime;
    bFireSelectedWeapon = false;
}

void APhysicsVehicle::AddWeapon(const FWeaponStruct& NewWeapon)
{
    if (!VehicleInventory)
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("Vehicle has no inventory component."));
        return;
    }

    VehicleInventory->AddItem(NewWeapon);
}

void APhysicsVehicle::CycleInventoryAscending()
{
    if (!VehicleInventory)
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("Vehicle has no inventory component."));
        return;
    }

    VehicleInventory->CycleInventoryAscending();
}

void APhysicsVehicle::CycleInventoryDescending()
{
    if (!VehicleInventory)
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("Vehicle has no inventory component."));
        return;
    }

    VehicleInventory->CycleInventoryDescending();
}

void APhysicsVehicle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(APhysicsVehicle, Hp);
    DOREPLIFETIME(APhysicsVehicle, Armor);
    DOREPLIFETIME(APhysicsVehicle, TargetLocation);
    DOREPLIFETIME(APhysicsVehicle, TargetRotation);
}

void APhysicsVehicle::ApplyVehicleDamage(double Damage)
{
    if (Armor > 0.0)
    {
        const double HealthDamage = Damage / 3.0;
        Armor -= HealthDamage * 2.0;
        OnRep_Armor();
        Hp -= HealthDamage;
    }
    else
    {
        Hp -= Damage;
    }
    OnRep_Hp();
}

void APhysicsVehicle::HealHealth(double Heal)
{
    const double NewHp = Hp + Heal;
    Hp = NewHp > MaxHp ? MaxHp : NewHp;
    OnRep_Hp();
}

void APhysicsVehicle::HealArmor(double Heal)
{
    const double NewArmor = Armor + Heal;
    Armor = NewArmor > MaxArmor ? MaxArmor : NewArmor;
    OnRep_Armor();
}

double APhysicsVehicle::GetHealth() const
{
    return Hp;
}

double APhysicsVehicle::GetArmor() const
{
    return Armor;
}

double APhysicsVehicle::GetMaxHealth() const
{
    return MaxHp;
}

double APhysicsVehicle::GetMaxArmor() const
{
    return MaxArmor;
}

bool APhysicsVehicle::IsDead() const
{
    return bIsDead;
}

FLinearColor APhysicsVehicle::GetPlayerColor() const
{
    return PlayerColor;
}

void APhysicsVehicle::SetDetectedChaseTarget(bool bDetected)
{
    if (!SetBlueprintBoolProperty(this, GET_MEMBER_NAME_CHECKED(APhysicsVehicle, bDetectedChaseTarget), bDetected))
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("%s is missing bDetectedChaseTarget."), *GetName());
    }
}

void APhysicsVehicle::SetChaseActorLocation(const FVector& NewLocation)
{
    if (!SetBlueprintVectorProperty(
        this, GET_MEMBER_NAME_CHECKED(APhysicsVehicle, ChaseActorLocation), NewLocation))
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("%s is missing ChaseActorLocation."), *GetName());
    }
}

UInventory* APhysicsVehicle::GetVehicleInventory() const
{
    return VehicleInventory;
}

void APhysicsVehicle::Die()
{
    if (!HasAuthority() || bIsDead)
    {
        return;
    }

    const FVector PhysicsLocation = PhysicsMesh->GetComponentLocation();
    const FVector ImpulseLocation = PhysicsLocation + FVector(
        FMath::FRandRange(-100.0f, 100.0f),
        FMath::FRandRange(-100.0f, 100.0f),
        0.0f);
    const FVector VehicleImpulse(
        FMath::FRandRange(-50000.0f, 50000.0f),
        FMath::FRandRange(-50000.0f, 50000.0f),
        85000.0f);
    PhysicsMesh->AddImpulseAtLocation(VehicleImpulse, ImpulseLocation);

    USceneComponent* WheelPositions[] = {Wheel_FL_Pos, Wheel_RL_Pos, Wheel_FR_Pos, Wheel_RR_Pos};
    for (USceneComponent* WheelPosition : WheelPositions)
    {
        if (WheelPosition)
        {
            WheelPosition->K2_DetachFromComponent(
                EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, true);
        }
    }

    UStaticMeshComponent* WheelMeshes[] = {
        WheelMesh_FR.Get(), WheelMesh_FL.Get(), WheelMesh_RL.Get(), WheelMesh_RR.Get()
    };
    for (UStaticMeshComponent* WheelMesh : WheelMeshes)
    {
        if (!WheelMesh)
        {
            continue;
        }
        WheelMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        WheelMesh->SetSimulatePhysics(true);
    }
    for (UStaticMeshComponent* WheelMesh : WheelMeshes)
    {
        if (!WheelMesh)
        {
            continue;
        }
        const FVector WheelLocation = WheelMesh->GetComponentLocation();
        const FVector WheelImpulse(
            FMath::FRandRange(-1000.0f, 1000.0f),
            FMath::FRandRange(-1000.0f, 1000.0f),
            1000.0f);
        const FVector WheelImpulseLocation = WheelLocation + FVector(
            FMath::FRandRange(-10.0f, 10.0f),
            FMath::FRandRange(-10.0f, 10.0f),
            0.0f);
        WheelMesh->AddImpulseAtLocation(WheelImpulse, WheelImpulseLocation);
    }

    if (NS_MainFire)
    {
        NS_MainFire->Activate(false);
    }

    if (DeathFragmentsSystem)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, DeathFragmentsSystem.Get(),
            PhysicsLocation, FRotator::ZeroRotator, FVector(3.0, 3.0, 1.0),
            true, true, ENCPoolMethod::None, true);
    }
    else
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("Death fragments Niagara system is not configured."));
    }

    if (DeathExplosionSystem && NS_MainFire)
    {
        FVector ExplosionLocation = NS_MainFire->GetComponentLocation();
        ExplosionLocation.Z += 200.0;
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, DeathExplosionSystem.Get(),
            ExplosionLocation, FRotator::ZeroRotator, FVector::OneVector,
            true, true, ENCPoolMethod::None, true);
    }
    else
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("Death explosion system or main fire component is not configured."));
    }

    bIsDead = true;
}

void APhysicsVehicle::HandleAnyDamage(AActor*, float Damage, const UDamageType*,
    AController*, AActor* DamageCauser)
{
    if (!HasAuthority() || bIsDead)
    {
        return;
    }

    if (DamageCauser && ArmorRepairCauserClass && DamageCauser->GetClass() == ArmorRepairCauserClass.Get())
    {
        HealArmor(Damage);
    }
    else if (DamageCauser && HealthRepairCauserClass && DamageCauser->GetClass() == HealthRepairCauserClass.Get())
    {
        HealHealth(Damage);
    }
    else
    {
        ApplyVehicleDamage(Damage);
    }
}

void APhysicsVehicle::OnRep_Hp()
{
    UpdateHealthWidget(TEXT("Event Change Enemy HealthBar Percent"), Hp, MaxHp);
}

void APhysicsVehicle::OnRep_Armor()
{
    UpdateHealthWidget(TEXT("Event Change Enemy ArmorBar Percent"), Armor, MaxArmor);
}

void APhysicsVehicle::OnRep_TargetLocation()
{
    if (!IsLocallyControlled())
    {
        InterpTargetLocation = TargetLocation;
    }
}

void APhysicsVehicle::OnRep_TargetRotation()
{
    if (!IsLocallyControlled())
    {
        InterpTargetRotation = TargetRotation;
    }
}

void APhysicsVehicle::HandleSkidEffects()
{
    HandleSkidMarks(NS_Skidmark_RR, WheelSlipAmount, bGroundedRR, NormalizedSpeed, Acceleration, bHandbrake);
    HandleSkidMarks(NS_Skidmark_RL, WheelSlipAmount, bGroundedRL, NormalizedSpeed, Acceleration, bHandbrake);
    HandleSkidSmoke(NS_Smoke_RR, WheelSlipAmount, bGroundedRR, NormalizedSpeed, Acceleration, bHandbrake);
    HandleSkidSmoke(NS_Smoke_RL, WheelSlipAmount, bGroundedRL, NormalizedSpeed, Acceleration, bHandbrake);
}

void APhysicsVehicle::HandleSkidMarks(UNiagaraComponent* NiagaraSystem, double Slip, bool Grounded,
    double Velocity, double InAcceleration, bool Handbraking)
{
    if (!NiagaraSystem)
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("Cannot update skid marks: Niagara component is null."));
        return;
    }

    const bool bShouldActivate = !bIsDead && Grounded &&
        (Slip > 700.0 || ((Velocity > 0.0 || InAcceleration > 0.0) && Handbraking));
    if (bShouldActivate)
    {
        NiagaraSystem->Activate(false);
    }
    else
    {
        NiagaraSystem->Deactivate();
    }
}

void APhysicsVehicle::HandleSkidSmoke(UNiagaraComponent* NiagaraSystem, double Slip, bool Grounded,
    double Velocity, double InAcceleration, bool Handbraking)
{
    if (!NiagaraSystem)
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("Cannot update skid smoke: Niagara component is null."));
        return;
    }

    const bool bShouldActivate = !bIsDead && Grounded && Slip > 1200.0 &&
        (Velocity > 0.7 || InAcceleration > 0.0) && Handbraking;
    if (bShouldActivate)
    {
        NiagaraSystem->Activate(false);
    }
    else
    {
        NiagaraSystem->Deactivate();
    }
}

void APhysicsVehicle::SpawnSkidMarks()
{
    UMaterialInterface* SkidMarkMaterial = LoadObject<UMaterialInterface>(
        nullptr, TEXT("/Game/Materials/M_SkidMark.M_SkidMark"));
    UNiagaraSystem* SkidSmokeSystem = LoadObject<UNiagaraSystem>(
        nullptr, TEXT("/Game/VFX/NS_SkidSmoke.NS_SkidSmoke"));
    if (!SkidMarkMaterial || !SkidSmokeSystem)
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("Could not load assets required to spawn skid marks."));
        return;
    }

    for (USceneComponent* ContactPoint : WheelsContactPointArray)
    {
        if (!IsValid(ContactPoint) ||
            !ContactPoint->GetName().Contains(TEXT("ContactPoint"), ESearchCase::IgnoreCase))
        {
            continue;
        }

        const FVector Location = ContactPoint->GetComponentLocation();
        const FName SocketName(*ContactPoint->GetName().Left(8));
        const FRotator Rotation = ContactPoint->GetSocketRotation(SocketName);
        UGameplayStatics::SpawnDecalAtLocation(this, SkidMarkMaterial,
            FVector(30.0, 30.0, 10.0), Location, Rotation, 5.0f);
        UNiagaraComponent* SpawnedSmoke = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this,
            SkidSmokeSystem, Location, FRotator::ZeroRotator, FVector(1.0, 1.0, 0.0),
            true, true, ENCPoolMethod::None, true);
        if (!SpawnedSmoke)
        {
            UE_LOG(LogPhysicsVehicle, Warning, TEXT("Failed to spawn skid smoke at %s."), *Location.ToString());
        }
    }
}

void APhysicsVehicle::UpdateHealthWidget(FName FunctionName, double Value, double Maximum)
{
    UUserWidget* Widget = EnemyHealthBar->GetUserWidgetObject();
    if (!Widget)
    {
        return;
    }

    UFunction* Function = Widget->FindFunction(FunctionName);
    FDoubleProperty* Parameter = nullptr;
    if (Function)
    {
        for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
        {
            if (Parameter || It->HasAnyPropertyFlags(CPF_ReturnParm))
            {
                Parameter = nullptr;
                break;
            }
            Parameter = CastField<FDoubleProperty>(*It);
            if (!Parameter)
            {
                break;
            }
        }
    }
    if (!Function || !Parameter)
    {
        UE_LOG(LogPhysicsVehicle, Error, TEXT("Health widget function %s is missing or has an unexpected signature."), *FunctionName.ToString());
        return;
    }

    FStructOnScope Parameters(Function);
    Parameter->SetPropertyValue_InContainer(Parameters.GetStructMemory(), UKismetMathLibrary::Divide_DoubleDouble(Value, Maximum));
    Widget->ProcessEvent(Function, Parameters.GetStructMemory());
}

void APhysicsVehicle::UpdateHealthBar()
{
    if (!EnemyHealthBar)
    {
        if (!bReportedMissingEnemyHealthBar)
        {
            UE_LOG(LogPhysicsVehicle, Error, TEXT("Vehicle health bar component is missing."));
            bReportedMissingEnemyHealthBar = true;
        }
        return;
    }

    if (APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
    {
        const FRotator FacingRotation = UKismetMathLibrary::FindLookAtRotation(
            EnemyHealthBar->GetComponentLocation(), CameraManager->GetCameraLocation());
        EnemyHealthBar->SetWorldRotation(FacingRotation);
    }

    UUserWidget* Widget = EnemyHealthBar->GetUserWidgetObject();
    if (!Widget)
    {
        return;
    }

    UFunction* Function = Widget->FindFunction(TEXT("Event Change Arrow Color"));
    FStructProperty* ColorParameter = Function
        ? FindFProperty<FStructProperty>(Function, TEXT("NewLinearColor"))
        : nullptr;
    if (!ColorParameter || ColorParameter->Struct != TBaseStructure<FLinearColor>::Get())
    {
        if (!bReportedMissingArrowColorFunction)
        {
            UE_LOG(LogPhysicsVehicle, Error,
                TEXT("Health widget arrow-color event is missing or has an unexpected signature."));
            bReportedMissingArrowColorFunction = true;
        }
        return;
    }

    FStructOnScope Parameters(Function);
    *ColorParameter->ContainerPtrToValuePtr<FLinearColor>(Parameters.GetStructMemory()) = PlayerColor;
    Widget->ProcessEvent(Function, Parameters.GetStructMemory());
}
