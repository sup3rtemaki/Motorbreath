#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Inventory.h"
#include "TimerManager.h"
#include "PhysicsVehicle.generated.h"

class USkeletalMeshComponent;
class USceneComponent;
class UStaticMeshComponent;
class UActorComponent;
class UChildActorComponent;
class UBoxComponent;
class UPrimitiveComponent;
class UWidgetComponent;
class UCurveFloat;
class UCameraComponent;
class USpringArmComponent;
class UInputComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UDamageType;
class AController;

UENUM(BlueprintType)
enum class EPhysicsVehicleWheelDrive : uint8
{
    FrontWheelDrive,
    RearWheelDrive,
    AllWheelDrive
};

UCLASS()
class MOTORBREATH_API APhysicsVehicle : public APawn
{
    GENERATED_BODY()

public:
    APhysicsVehicle();

    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Vehicle|Camera")
    void ToggleVehicleCamera();

    UFUNCTION(BlueprintCallable, Category = "Vehicle|Effects")
    void SpawnSkidMarks();

    UFUNCTION(BlueprintCallable, Category = "Vehicle|Components")
    void CheckMeshes();

    UFUNCTION(BlueprintCallable, Category = "Vehicle|Targeting")
    void FindNearestTarget(double& Distance, AActor*& NearestActor) const;

    UFUNCTION(BlueprintCallable, Category = "Vehicle|Weapons")
    void RequestMachineGunFire();

    UFUNCTION(Server, Unreliable, Category = "Vehicle|Weapons")
    void Server_RequestMachineGunFire();

    UFUNCTION(BlueprintCallable, Category = "Vehicle|Weapons")
    void FireMachinegun();

    UFUNCTION(BlueprintCallable, Category = "Vehicle|Weapons")
    void RequestSelectedWeaponFire();

    UFUNCTION(Server, Unreliable, Category = "Vehicle|Weapons")
    void Server_RequestSelectedWeaponFire();

    UFUNCTION(BlueprintCallable, Category = "Vehicle|Weapons")
    void FireSelectedWeapon();

    UFUNCTION(BlueprintCallable, Category = "Vehicle|Inventory")
    void AddWeapon(const FWeaponStruct& NewWeapon);

    UFUNCTION(BlueprintCallable, Category = "Vehicle|Inventory")
    void CycleInventoryAscending();

    UFUNCTION(BlueprintCallable, Category = "Vehicle|Inventory")
    void CycleInventoryDescending();

    UFUNCTION(BlueprintCallable, Category = "Vehicle|Effects")
    void HandleSkidMarks(UNiagaraComponent* NiagaraSystem, double Slip, bool Grounded,
        double Velocity, double InAcceleration, bool Handbraking);

    UFUNCTION(BlueprintCallable, Category = "Vehicle|Effects")
    void HandleSkidSmoke(UNiagaraComponent* NiagaraSystem, double Slip, bool Grounded,
        double Velocity, double InAcceleration, bool Handbraking);

    UFUNCTION(BlueprintCallable, Server, Unreliable, Category = "Vehicle|Steering")
    void Server_SetSteeringAngle(double NewSteeringAngle, bool IsSteering);

    UFUNCTION(BlueprintCallable, Category = "Vehicle|Movement")
    void SetVehicleAcceleration(double NewAcceleration);

    UFUNCTION(BlueprintCallable, Category = "Vehicle|Movement")
    void SetVehicleBrake(double NewBrake, bool Handbrake);

    UFUNCTION(BlueprintCallable, Server, Unreliable, Category = "Vehicle|Movement")
    void Server_SetAcceleration(double NewAcceleration);

    UFUNCTION(BlueprintCallable, Server, Unreliable, Category = "Vehicle|Movement")
    void Server_SetBrake(double NewBrake, bool Handbrake);

    UFUNCTION(BlueprintCallable, Server, Unreliable, Category = "Vehicle|Replication")
    void Server_SendTransform();

    UFUNCTION(BlueprintCallable, NetMulticast, Unreliable, Category = "Vehicle|Replication")
    void Multicast_UpdateTransform(FVector NewLocation, FRotator NewRotation);

    UFUNCTION(BlueprintCallable, Category = "Vehicle|Health")
    void ApplyVehicleDamage(double Damage);

    UFUNCTION(BlueprintCallable, Category = "Vehicle|Health")
    void Die();

    UFUNCTION(BlueprintCallable, Category = "Vehicle|Health")
    void HealHealth(double Heal);

    UFUNCTION(BlueprintCallable, Category = "Vehicle|Health")
    void HealArmor(double Heal);

    double GetHealth() const;
    double GetArmor() const;
    double GetMaxHealth() const;
    double GetMaxArmor() const;
    bool IsDead() const;
    FLinearColor GetPlayerColor() const;
    void SetDetectedChaseTarget(bool bDetected);
    void SetChaseActorLocation(const FVector& NewLocation);
    UInventory* GetVehicleInventory() const;

    UFUNCTION()
    void HandleFrontCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Components")
    TObjectPtr<USkeletalMeshComponent> PhysicsMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Components")
    TObjectPtr<UWidgetComponent> EnemyHealthBar;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Inventory")
    TObjectPtr<UInventory> VehicleInventory;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Components")
    TObjectPtr<UBoxComponent> FrontCollisionBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Camera")
    TObjectPtr<USpringArmComponent> SpringArm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Camera")
    TObjectPtr<USpringArmComponent> RearSpringArm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Camera")
    TObjectPtr<UCameraComponent> ChaseCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Camera")
    TObjectPtr<UCameraComponent> RearChaseCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Camera")
    TObjectPtr<UCameraComponent> InCarCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Components")
    TObjectPtr<USceneComponent> Wheel_FR_Pos;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Components")
    TObjectPtr<USceneComponent> Wheel_FL_Pos;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Components")
    TObjectPtr<USceneComponent> Wheel_RR_Pos;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Components")
    TObjectPtr<USceneComponent> Wheel_RL_Pos;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Weapons")
    TObjectPtr<USceneComponent> ProjectileSpawnPoint;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Weapons")
    TObjectPtr<USceneComponent> MissileSpawnPoint;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Weapons")
    TObjectPtr<USceneComponent> DroppedSpawnPoint;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Weapons")
    TObjectPtr<USceneComponent> TargetPoint;

    // Child actor class can be swapped per vehicle in the Blueprint's details panel.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Audio")
    TObjectPtr<UChildActorComponent> EngineAudioComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Audio")
    TObjectPtr<UChildActorComponent> SkidAudioComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|HUD")
    TObjectPtr<UChildActorComponent> RadarManagerComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Components")
    TObjectPtr<UStaticMeshComponent> WheelMesh_FR;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Components")
    TObjectPtr<UStaticMeshComponent> WheelMesh_FL;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Components")
    TObjectPtr<UStaticMeshComponent> WheelMesh_RR;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Components")
    TObjectPtr<UStaticMeshComponent> WheelMesh_RL;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Effects")
    TObjectPtr<UNiagaraComponent> NS_Skidmark_FR;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Effects")
    TObjectPtr<UNiagaraComponent> NS_Skidmark_FL;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Effects")
    TObjectPtr<UNiagaraComponent> NS_Skidmark_RR;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Effects")
    TObjectPtr<UNiagaraComponent> NS_Skidmark_RL;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Effects")
    TObjectPtr<UNiagaraComponent> NS_Smoke_FR;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Effects")
    TObjectPtr<UNiagaraComponent> NS_Smoke_FL;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Effects")
    TObjectPtr<UNiagaraComponent> NS_Smoke_RR;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Effects")
    TObjectPtr<UNiagaraComponent> NS_Smoke_RL;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Effects")
    TObjectPtr<UNiagaraComponent> NS_MainFire;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Effects")
    TObjectPtr<UNiagaraSystem> DeathFragmentsSystem;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Effects")
    TObjectPtr<UNiagaraSystem> DeathExplosionSystem;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Effects")
    TObjectPtr<UNiagaraSystem> RamImpactNiagaraSystem;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Weapons")
    TObjectPtr<UNiagaraSystem> MuzzleflashSystem;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Weapons")
    TSubclassOf<AActor> MachineGunBulletClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Steering")
    double CurrentSteeringAngle = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Steering")
    double TargetSteeringAngle = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Steering")
    bool bIsSteering = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Camera")
    double SpringArmPitch = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Camera")
    double SpringArmYaw = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Camera")
    double SpringArmRoll = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Health")
    bool bIsDead = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Suspension")
    double FrontSuspensionRestDistance = 40.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Suspension")
    double RearSuspensionRestDistance = 64.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Suspension")
    double FrontWheelRadius = 35.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Suspension")
    double RearWheelRadius = 56.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Suspension")
    double SpringStrength = 1000.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Suspension")
    double SpringDamping = 200.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Suspension")
    bool bDebug = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Suspension")
    bool bGroundedFR = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Suspension")
    bool bGroundedFL = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Suspension")
    bool bGroundedRL = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Suspension")
    bool bGroundedRR = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Suspension")
    double ForceFR = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Suspension")
    double ForceFL = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Suspension")
    double ForceRL = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Suspension")
    double ForceRR = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Movement")
    double MaxSpeed = 5000.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Movement")
    double MaxAcceleration = 20000.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Movement")
    EPhysicsVehicleWheelDrive WheelDriveType = EPhysicsVehicleWheelDrive::AllWheelDrive;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Movement")
    TObjectPtr<UCurveFloat> AccelerationCurve;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Movement")
    TObjectPtr<UCurveFloat> SteerCurve;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Movement")
    TObjectPtr<UCurveFloat> TurnCurve;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Movement")
    TObjectPtr<UCurveFloat> FrictionCurve;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Movement")
    TObjectPtr<UCurveFloat> AngularDamping;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Movement")
    double Acceleration = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Movement")
    double TargetAcceleration = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Movement")
    double TargetBrake = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Movement")
    bool bHandbrake = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Movement")
    FVector LinearVelocity = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Movement")
    double CurrentSpeed = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Movement")
    double NormalizedSpeed = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Movement")
    double WheelSlipAmount = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Movement")
    double NormalizedSkid = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Weapons")
    double MachineGunTimer = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Weapons")
    double MaxMachineGunTime = 0.1;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Weapons")
    double WeaponTimer = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Weapons")
    double MaxWeaponTime = 1.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Weapons")
    bool bFireMachinegun = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Weapons")
    bool bFireSelectedWeapon = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Movement")
    double RamTimer = 20.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Movement")
    double RamBaseDamage = 20.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Movement")
    double UpsideDownTimer = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Movement")
    double MaxUpsideDownTime = 5.0;

    UPROPERTY(ReplicatedUsing = OnRep_TargetLocation, VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Replication")
    FVector TargetLocation = FVector::ZeroVector;

    UPROPERTY(ReplicatedUsing = OnRep_TargetRotation, VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Replication")
    FRotator TargetRotation = FRotator::ZeroRotator;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Replication")
    FVector InterpTargetLocation = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Replication")
    FRotator InterpTargetRotation = FRotator::ZeroRotator;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Replication")
    FVector ReplicatedLocation = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Replication")
    FRotator ReplicatedRotation = FRotator::ZeroRotator;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Vehicle|Effects")
    TArray<TObjectPtr<USceneComponent>> WheelsContactPointArray;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Targeting", meta = (ExposeOnSpawn = "true"))
    TArray<TObjectPtr<AActor>> TargetList;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Targeting")
    TObjectPtr<AActor> NearestTarget;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Targeting")
    bool bDetectedChaseTarget = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Targeting")
    FVector ChaseActorLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Targeting",
        meta = (ExposeOnSpawn = "true"))
    FLinearColor PlayerColor = FLinearColor(1.0f, 0.823312f, 0.0f, 1.0f);

    UPROPERTY(ReplicatedUsing = OnRep_Hp, VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Health")
    double Hp = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Health")
    double MaxHp = 100.0;

    UPROPERTY(ReplicatedUsing = OnRep_Armor, VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle|Health")
    double Armor = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Health")
    double MaxArmor = 100.0;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Health")
    TSubclassOf<AActor> ArmorRepairCauserClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Health")
    TSubclassOf<AActor> HealthRepairCauserClass;

    UFUNCTION()
    void OnRep_Hp();

    UFUNCTION()
    void OnRep_Armor();

    UFUNCTION()
    void OnRep_TargetLocation();

    UFUNCTION()
    void OnRep_TargetRotation();

private:
    void UpdateHealthWidget(FName FunctionName, double Value, double Maximum);
    void UpdateHealthBar();
    void InitializeBlueprintRuntimeComponents();
    void UpdateVehicleAudio();
    void CalculateVariables();
    void HandleSuspensions();
    void CalculateWheelSuspension(USceneComponent* Wheel, UStaticMeshComponent* WheelMesh,
        double RestDistance, double WheelRadius, bool& bGrounded, double& Force);
    void UpdateSteering(double DeltaSeconds);
    bool CheckHandlingCurve(const UCurveFloat* Curve, const TCHAR* CurveName, bool& bReportedMissing);
    void ApplyTurnTorque();
    void ApplyLateralFriction();
    void CalculateRearWheelSlip();
    void UpdateAngularDamping();
    void CheckUpsideDown();
    void HandleCameras();
    void HandleSkidEffects();
    void LookRight(float AxisValue);
    void LookUp(float AxisValue);
    void BrakeAndAccelerate();
    void BrakeAtWheel(USceneComponent* Wheel, bool bGrounded, double Multiplier);
    bool Accelerate();
    void AccelerateAtWheel(USceneComponent* Wheel, bool bGrounded);
    void RollWheels();
    void RollWheel(UStaticMeshComponent* WheelMesh, USceneComponent* WheelPosition, FVector& PreviousLocation);
    UFUNCTION()
    void HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
        AController* InstigatedBy, AActor* DamageCauser);

    FVector PreviousWheelLocation_FR = FVector::ZeroVector;
    FVector PreviousWheelLocation_FL = FVector::ZeroVector;
    FVector PreviousWheelLocation_RR = FVector::ZeroVector;
    FVector PreviousWheelLocation_RL = FVector::ZeroVector;
    bool bReportedMissingAccelerationCurve = false;
    bool bReportedMissingSteerCurve = false;
    bool bReportedMissingTurnCurve = false;
    bool bReportedMissingFrictionCurve = false;
    bool bReportedMissingAngularDamping = false;
    bool bUsingRearCamera = false;
    bool bReportedMissingEnemyHealthBar = false;
    bool bReportedMissingArrowColorFunction = false;
    bool bReportedMissingEngineAudioProperties = false;
    bool bReportedMissingSkidAudioProperty = false;

    FTimerHandle TransformUpdateTimerHandle;

    UPROPERTY(Transient)
    TObjectPtr<AActor> EngineAudioActor;

    UPROPERTY(Transient)
    TObjectPtr<AActor> SkidAudioActor;

    UPROPERTY(Transient)
    TObjectPtr<AActor> RadarManagerActor;

    UPROPERTY(Transient)
    TObjectPtr<UActorComponent> FrontObstructionSensorComponent;

    UPROPERTY()
    TSubclassOf<AActor> MineWeaponClass;
};
