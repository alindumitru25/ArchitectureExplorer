// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HandController.h"
#include "VRCharacter.generated.h"

UCLASS()
class ARCHITECTUREEXPLORER_API AVRCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AVRCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	UPROPERTY(VisibleAnywhere)
		class UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere)
		class USceneComponent* VRRoot;

	UPROPERTY(VisibleAnywhere)
		class UStaticMeshComponent* DestinationMarker;

	UPROPERTY(EditAnywhere)
		float CameraFadeoutDuration = 1.0f;

	UPROPERTY(EditAnywhere)
		FVector TeleportProjectionExtent = FVector(100, 100, 100);

	UPROPERTY(EditAnywhere)
		int ProjectileSpeed = 800.0f;

	UPROPERTY(EditAnywhere)
		float TeleportProjectileRadius = 10.0f;

	UPROPERTY(EditAnywhere)
		float TeleportSimulationTime = 1.0f;

	UPROPERTY()
		class UPostProcessComponent* PostProcessComponent;

	UPROPERTY(EditAnywhere)
		class UMaterialInterface* BlinkerMaterialBase;

	UPROPERTY()
		UMaterialInstanceDynamic* DynamicMaterial;

	UPROPERTY(EditAnywhere)
		class UCurveFloat* RadiusVsVelocity;

	UPROPERTY()
		class AHandController* LeftController;

	UPROPERTY()
		class AHandController* RightController;

	UPROPERTY(EditDefaultsOnly)
		TSubclassOf<class AHandController> ControllerBPClass;

	void MoveForward(float AxisValue);

	void MoveRight(float AxisValue);

	void BeginTeleport();

	void Teleport();

	void UpdateDestinationMarker();

	bool FindteleportDestination(TArray<FVector>& OutPath, FVector& OutLocation);

	void StartFade(bool fromAlpha, bool toAlpha);

	void UpdateBlinkers();

	void GripLeft() { LeftController->Grip(); }

	void ReleaseLeft() { LeftController->Release(); }

	void GripRight() { RightController->Grip(); }

	void ReleaseRight() { RightController->Release(); }

	FVector2D GetBlinkerCenter();

	FTimerHandle TimerHandle;

	APlayerCameraManager* PlayerCameraManager;

	APlayerController* ThisPlayerController;

	const FName RadiusParameter = TEXT("Radius");

	const FName CenterParameter = TEXT("Center");
};
