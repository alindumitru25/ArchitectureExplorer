// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HandController.generated.h"

UCLASS()
class ARCHITECTUREEXPLORER_API AHandController : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHandController();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void UpdateSpline(const TArray<FVector>& Path);

	void CreateTeleportBeam(const TArray<FVector>& Path);

	void UpdateTrackingSource(const EControllerHand CurrentHand);

	void PairController(AHandController* Controller);

	void Grip();
	void Release();

private:
	UPROPERTY(EditDefaultsOnly)
	class UStaticMesh* TeleportArchMesh;

	UPROPERTY(EditDefaultsOnly)
	class UMaterialInterface* TeleportArchMaterial;

	UPROPERTY()
	TArray<class USplineMeshComponent*> TeleportMeshes;

	UPROPERTY(VisibleAnywhere)
	class USplineComponent* TeleportPath;

	UPROPERTY(VisibleAnywhere)
	class UMotionControllerComponent* VRHandRoot;

	UPROPERTY(EditAnywhere)
	class UHapticFeedbackEffect_Base* HapticFeedback;

	USplineMeshComponent* CreateTeleportObject();

	EControllerHand ControllerHand;

	// Callbacks
	UFUNCTION()
	void ActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor);

	UFUNCTION()
	void ActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor);

	// State
	bool bCanClimb = false;

	bool bIsClimbing = false;

	FVector ClimbingStartLocation;

	AHandController* OtherController;

	// Helpers
	bool CanClimb() const;

	// Handle Methods
	void HandleClimbing();
};
