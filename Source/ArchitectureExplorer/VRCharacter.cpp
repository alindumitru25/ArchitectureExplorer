// Fill out your copyright notice in the Description page of Project Settings.


#include "VRCharacter.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Math/Vector.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"
#include "NavigationSystem.h"
#include "Components/PostProcessComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MotionControllerComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AVRCharacter::AVRCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VRRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRRoot"));
	VRRoot->SetupAttachment(GetRootComponent());

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VRRoot);

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DestinationMarker"));
	DestinationMarker->SetupAttachment(GetRootComponent());

	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessComponent"));
	PostProcessComponent->SetupAttachment(GetRootComponent());
}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();
	DestinationMarker->SetVisibility(false);

	APlayerController* PlayerController = Cast<APlayerController>(GetController());

	if (PlayerController != nullptr)
	{
		PlayerCameraManager = PlayerController->PlayerCameraManager;
		ThisPlayerController = PlayerController;
	}

	if (BlinkerMaterialBase != nullptr)
	{
		DynamicMaterial = UMaterialInstanceDynamic::Create(BlinkerMaterialBase, this);
		DynamicMaterial->SetScalarParameterValue(RadiusParameter, 2.0);
		PostProcessComponent->AddOrUpdateBlendable(DynamicMaterial);
	}

	LeftController = GetWorld()->SpawnActor<AHandController>(ControllerBPClass);
	if (LeftController != nullptr)
	{
		LeftController->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
		LeftController->UpdateTrackingSource(EControllerHand::Left);
		LeftController->SetOwner(this);
	}

	RightController = GetWorld()->SpawnActor<AHandController>(ControllerBPClass);
	if (RightController != nullptr)
	{
		RightController->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
		RightController->UpdateTrackingSource(EControllerHand::Right);
		RightController->SetOwner(this);
	}

	LeftController->PairController(RightController);
}

// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector NewCameraOffset = Camera->GetComponentLocation() - GetActorLocation();
	NewCameraOffset.Z = 0;

	AddActorWorldOffset(NewCameraOffset);
	VRRoot->AddWorldOffset(-NewCameraOffset);
	
	UpdateBlinkers();
	UpdateDestinationMarker();
}

// Called to bind functionality to input
void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AVRCharacter::MoveForward);

	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AVRCharacter::MoveRight);

	PlayerInputComponent->BindAction(TEXT("Teleport"), IE_Released, this, &AVRCharacter::BeginTeleport);

	PlayerInputComponent->BindAction(TEXT("GripLeft"), IE_Pressed, this, &AVRCharacter::GripLeft);

	PlayerInputComponent->BindAction(TEXT("GripLeft"), IE_Released, this, &AVRCharacter::ReleaseLeft);

	PlayerInputComponent->BindAction(TEXT("GripLeft"), IE_Pressed, this, &AVRCharacter::GripLeft);

	PlayerInputComponent->BindAction(TEXT("GripLeft"), IE_Released, this, &AVRCharacter::ReleaseLeft);

	PlayerInputComponent->BindAction(TEXT("GripRight"), IE_Pressed, this, &AVRCharacter::GripRight);

	PlayerInputComponent->BindAction(TEXT("GripRight"), IE_Released, this, &AVRCharacter::ReleaseRight);
}

void AVRCharacter::MoveForward(float AxisValue)
{
	AddMovementInput(Camera->GetForwardVector() * AxisValue);
}

void AVRCharacter::MoveRight(float AxisValue)
{
	AddMovementInput(Camera->GetRightVector() * AxisValue);
}

void AVRCharacter::BeginTeleport()
{
	if (PlayerCameraManager && DestinationMarker->IsVisible())
	{
		StartFade(0.0, 1.0);

		GetWorld()->GetTimerManager().SetTimer(
			TimerHandle,
			this,
			&AVRCharacter::Teleport,
			CameraFadeoutDuration
		);
	}
}

void AVRCharacter::Teleport()
{
	GetWorldTimerManager().ClearTimer(TimerHandle);

	float CapsuleHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector DestinationMarkerLoc = DestinationMarker->GetComponentLocation();
	DestinationMarkerLoc.Z = DestinationMarkerLoc.Z + CapsuleHeight;

	SetActorLocation(DestinationMarkerLoc);

	StartFade(1.0, 0.0);
}

bool AVRCharacter::FindteleportDestination(TArray<FVector>& OutPath, FVector& OutLocation)
{
	FHitResult HitResult;

	// Line tracing teleportation
	/*FVector Start = RightController->GetComponentLocation();
	FVector ControllerDirection = RightController->GetForwardVector().RotateAngleAxis(25, RightController->GetRightVector());
	FVector End = Start + ControllerDirection * MaxRange;

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECollisionChannel::ECC_Visibility
	);*/

	// DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, -1.0F, (uint8)'\000', 10.0f);

	// Parabola Teleportation
	FVector Start = RightController->GetActorLocation();
	FVector ControllerDirection = RightController->GetActorForwardVector();

	FPredictProjectilePathParams ProjectileParams(
		TeleportProjectileRadius,
		Start,
		ControllerDirection * ProjectileSpeed,
		TeleportSimulationTime, // tweaking this will make your parabola shorter or longer
		ECollisionChannel::ECC_Visibility,
		this
	);

	ProjectileParams.bTraceComplex = true;

	FPredictProjectilePathResult ProjectileResult;
	bool bHit = UGameplayStatics::PredictProjectilePath(GetWorld(), ProjectileParams, ProjectileResult);

	if (!bHit) return false;

	for (FPredictProjectilePathPointData Data : ProjectileResult.PathData)
	{
		OutPath.Add(Data.Location);
	}


	UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	FNavLocation NavLocation;

	bool hasHitNav = NavigationSystem->ProjectPointToNavigation(
		ProjectileResult.HitResult.Location,
		NavLocation,
		TeleportProjectionExtent
	);

	if (!hasHitNav) return false;

	OutLocation = NavLocation.Location;

	return bHit && hasHitNav;
}


void AVRCharacter::UpdateDestinationMarker()
{
	FVector Location;
	TArray<FVector> Path;

	bool bFoundDestination = FindteleportDestination(Path, Location);

	if (bFoundDestination)
	{
		DestinationMarker->SetVisibility(true);
		DestinationMarker->SetWorldLocation(Location);

		RightController->UpdateSpline(Path);
		RightController->CreateTeleportBeam(Path);
	}
	else
	{
		DestinationMarker->SetVisibility(false);
		TArray<FVector> EmptyPath;
		RightController->CreateTeleportBeam(EmptyPath);
	}
}

void AVRCharacter::StartFade(bool fromAlpha, bool toAlpha)
{
	if (PlayerCameraManager != nullptr)
	{
		PlayerCameraManager->StartCameraFade(
			fromAlpha,
			toAlpha,
			CameraFadeoutDuration,
			FLinearColor::Black
		);
	}
}

void AVRCharacter::UpdateBlinkers()
{
	if (RadiusVsVelocity == nullptr) return;

	FVector Velocity = GetVelocity();
	float Radius = RadiusVsVelocity->GetFloatValue(Velocity.Size() / 100);

	FVector2D Center = GetBlinkerCenter();
	DynamicMaterial->SetScalarParameterValue(RadiusParameter, Radius);
	DynamicMaterial->SetVectorParameterValue(CenterParameter, FLinearColor(Center.X, Center.Y, 0));
}

FVector2D AVRCharacter::GetBlinkerCenter()
{
	FVector MovementDirection = GetVelocity().GetSafeNormal();

	if (MovementDirection.IsNearlyZero())
	{
		return FVector2D(0.5, 0.5);
	}

	FVector WorldStationaryLocation;
	// Dot product is needed to know the direction of the twovectors (camera forward
	// vector and movement direction)
	if (FVector::DotProduct(Camera->GetForwardVector(), MovementDirection) > 0)
	{
		WorldStationaryLocation = Camera->GetComponentLocation() + MovementDirection * 1000;
	}
	else
	{
		// if the two vectors don't point in the same direction, subtract instead of add so we can project
		WorldStationaryLocation = Camera->GetComponentLocation() - MovementDirection * 1000;
	}
	
	if (!ThisPlayerController)
	{
		return FVector2D(0.5, 0.5);
	}

	// Project the movement direction to screen location
	FVector2D ScreenLocation;
	ThisPlayerController->ProjectWorldLocationToScreen(WorldStationaryLocation, ScreenLocation);

	int32 SizeX, SizeY;
	ThisPlayerController->GetViewportSize(SizeX, SizeY);

	// Get the things in UV coordinates
	return FVector2D(ScreenLocation.X / SizeX, ScreenLocation.Y / SizeY);
}


