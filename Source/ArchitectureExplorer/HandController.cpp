// Fill out your copyright notice in the Description page of Project Settings.


#include "HandController.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "MotionControllerComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
AHandController::AHandController()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VRHandRoot = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("VRHandRoot"));
	SetRootComponent(VRHandRoot);

	TeleportPath = CreateDefaultSubobject<USplineComponent>(TEXT("TeleportPath"));
	TeleportPath->SetupAttachment(VRHandRoot);
}

// Called when the game starts or when spawned
void AHandController::BeginPlay()
{
	Super::BeginPlay();

	OnActorBeginOverlap.AddDynamic(this, &AHandController::ActorBeginOverlap);
	OnActorEndOverlap.AddDynamic(this, &AHandController::ActorEndOverlap);
}

// Called every frame
void AHandController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	HandleClimbing();
}

void AHandController::UpdateSpline(const TArray<FVector>& Path)
{
	TeleportPath->ClearSplinePoints(false);

	for (int32 i = 0; i < Path.Num(); i++)
	{
		FVector LocalPosition = TeleportPath->GetComponentTransform().InverseTransformPosition(Path[i]);
		FSplinePoint Point(i, LocalPosition, ESplinePointType::Curve);
		TeleportPath->AddPoint(Point, false);
	}

	TeleportPath->UpdateSpline();
}

void AHandController::CreateTeleportBeam(const TArray<FVector>& Path)
{
	for (USplineMeshComponent* SplineMesh : TeleportMeshes)
	{
		// set visibility false for all occurances so we won't have glitches
		SplineMesh->SetVisibility(false);
	}

	for (int32 i = 0; i < Path.Num() - 1; i++)
	{
		USplineMeshComponent* TeleportMesh;

		if (i < TeleportMeshes.Num())
		{
			TeleportMesh = TeleportMeshes[i];
		}
		else
		{
			TeleportMesh = CreateTeleportObject();
			TeleportMeshes.Add(TeleportMesh);
		}

		// toggle visibility on for what we use
		TeleportMesh->SetVisibility(true);

		// get the positions of two adjiacent points
		FVector StartPos, StartTangent, EndPos, EndTangent;
		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i, StartPos, StartTangent);
		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i + 1, EndPos, EndTangent);

		// set mesh position in local space
		TeleportMesh->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);
	}
}

void AHandController::UpdateTrackingSource(const EControllerHand CurrentHand)
{
	ControllerHand = CurrentHand;
	VRHandRoot->SetTrackingSource(CurrentHand);
}

void AHandController::PairController(AHandController* Controller)
{
	OtherController = Controller;
	OtherController->OtherController = this;
}

void AHandController::Grip()
{
	if (bCanClimb && !bIsClimbing)
	{
		OtherController->bIsClimbing = false;

		ClimbingStartLocation = GetActorLocation();
		bIsClimbing = true;

		ACharacter* CurrentChar = Cast<ACharacter>(GetAttachParentActor());

		if (CurrentChar != nullptr)
		{
			// disable gravitation when climbing
			CurrentChar->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
		}
	}
}

void AHandController::Release()
{
	if (bIsClimbing)
	{
		bIsClimbing = false;
		ACharacter* CurrentChar = Cast<ACharacter>(GetAttachParentActor());

		if (CurrentChar != nullptr)
		{
			// enable gravitation back
			CurrentChar->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
		}
	}
}

USplineMeshComponent* AHandController::CreateTeleportObject()
{
	USplineMeshComponent* ObjectMesh = NewObject<USplineMeshComponent>(this);
	ObjectMesh->SetMobility(EComponentMobility::Movable);
	ObjectMesh->AttachToComponent(TeleportPath, FAttachmentTransformRules::KeepRelativeTransform);

	ObjectMesh->SetStaticMesh(TeleportArchMesh);
	ObjectMesh->SetMaterial(0, TeleportArchMaterial);

	ObjectMesh->RegisterComponent();

	return ObjectMesh;
}

void AHandController::ActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	bool bNewCanClimb = CanClimb();

	APawn* CurrentChar = Cast<APawn>(GetAttachParentActor());
	if (!bCanClimb && bNewCanClimb && CurrentChar != nullptr)
	{
		APlayerController* PlayerController = Cast<APlayerController>(CurrentChar->GetController());

		if (PlayerController)
		{
			PlayerController->PlayHapticEffect(HapticFeedback, ControllerHand);
		}
	}

	bCanClimb = bNewCanClimb;
}

void AHandController::ActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	bCanClimb = CanClimb();
}

bool AHandController::CanClimb() const
{
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors);

	for (AActor* Actor : OverlappingActors)
	{
		if (Actor->ActorHasTag(TEXT("Climbable")))
		{
			return true;
		}
	}

	return false;
}

void AHandController::HandleClimbing()
{
	if (bIsClimbing)
	{
		APawn* CurrentChar = Cast<APawn>(GetAttachParentActor());

		if (CurrentChar != nullptr)
		{
			FVector HandOffset = GetActorLocation() - ClimbingStartLocation;
			CurrentChar->AddActorWorldOffset(-HandOffset);
		}
	}
}
