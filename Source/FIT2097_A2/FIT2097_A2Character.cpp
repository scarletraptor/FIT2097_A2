// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "FIT2097_A2.h"
#include "Kismet/HeadMountedDisplayFunctionLibrary.h"
#include "FIT2097_A2Character.h"
#include "Net/UnrealNetwork.h"
#include "Engine.h"
#include "DrawDebugHelpers.h"
#include "Interactable.h"
#include "Fuse.h"
#include "FuseDoor.h"
#include "WallFuse.h"
#include "RemoteAccess.h"

//////////////////////////////////////////////////////////////////////////
// AFIT2097_A2Character

AFIT2097_A2Character::AFIT2097_A2Character()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

												// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

												   // Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
												   // are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

void AFIT2097_A2Character::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFIT2097_A2Character, MaxHealth);
	DOREPLIFETIME(AFIT2097_A2Character, CurrentHealth);
	DOREPLIFETIME(AFIT2097_A2Character, bHasFuse);
}

//////////////////////////////////////////////////////////////////////////
// Input

void AFIT2097_A2Character::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &AFIT2097_A2Character::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AFIT2097_A2Character::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AFIT2097_A2Character::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AFIT2097_A2Character::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AFIT2097_A2Character::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AFIT2097_A2Character::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AFIT2097_A2Character::OnResetVR);

	// handle collecting pickups
	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AFIT2097_A2Character::Interact);
}


void AFIT2097_A2Character::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AFIT2097_A2Character::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void AFIT2097_A2Character::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void AFIT2097_A2Character::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AFIT2097_A2Character::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AFIT2097_A2Character::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AFIT2097_A2Character::MoveRight(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}


float AFIT2097_A2Character::GetMaxHealth()
{
	return MaxHealth;
}

float AFIT2097_A2Character::GetCurrentHealth()
{
	return CurrentHealth;
}

bool AFIT2097_A2Character::GetHasFuse()
{
	return bHasFuse;
}

bool AFIT2097_A2Character::GetOpenFuseDoor()
{
	return bOpenFuseDoor;
}

void AFIT2097_A2Character::UpdateHealth(float DeltaHealth)
{
	if (Role == ROLE_Authority)
	{
		OnRep_CurrentHealth();
	}
}

void AFIT2097_A2Character::OnPlayerDeath_Implementation()
{
	// disconnect the controller from the pawn
	DetachFromControllerPendingDestroy();

	if (GetMesh())
	{
		static FName CollisionProfileName(TEXT("Ragdoll"));
		GetMesh()->SetCollisionProfileName(CollisionProfileName);
	}

	SetActorEnableCollision(true);

	// ragdoll and initiate physics
	GetMesh()->SetAllBodiesSimulatePhysics(true);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->WakeAllRigidBodies();
	GetMesh()->bBlendPhysics = true;

	// disable movements
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->SetComponentTickEnabled(false);

	// disable capsule
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
}

FString AFIT2097_A2Character::MyRole()
{
	if (Role == ROLE_Authority)
	{
		return TEXT("Server");
	}
	else
	{
		return TEXT("Client");
	}
}

void AFIT2097_A2Character::OnRep_CurrentHealth()
{
	//TODO
}

void AFIT2097_A2Character::Interact()
{
	// ask server to collect pickups
	ServerInteract();
}

bool AFIT2097_A2Character::ServerInteract_Validate()
{
	return true;
}

void AFIT2097_A2Character::ServerInteract_Implementation()
{
	if (Role = ROLE_Authority)
	{
		CallMyTrace();
	}
}

//***************************************************************************************************
//** Trace functions - used to detect items we are looking at in the world
//***************************************************************************************************
//***************************************************************************************************

//***************************************************************************************************
//** Trace() - called by our CallMyTrace() function which sets up our parameters and passes them through
//***************************************************************************************************

bool AFIT2097_A2Character::Trace(
	UWorld* World,
	TArray<AActor*>& ActorsToIgnore,
	const FVector& Start,
	const FVector& End,
	FHitResult& HitOut,
	ECollisionChannel CollisionChannel = ECC_Pawn,
	bool ReturnPhysMat = false
)
{

	// The World parameter refers to our game world (map/level) 
	// If there is no World, abort
	if (!World)
	{
		return false;
	}

	// Set up our TraceParams object
	FCollisionQueryParams TraceParams(FName(TEXT("My Trace")), true, ActorsToIgnore[0]);

	// Should we simple or complex collision?
	TraceParams.bTraceComplex = true;

	// We don't need Physics materials 
	TraceParams.bReturnPhysicalMaterial = ReturnPhysMat;

	// Add our ActorsToIgnore
	TraceParams.AddIgnoredActors(ActorsToIgnore);

	// When we're debugging it is really useful to see where our trace is in the world
	// We can use World->DebugDrawTraceTag to tell Unreal to draw debug lines for our trace
	// (remove these lines to remove the debug - or better create a debug switch!)
	const FName TraceTag("MyTraceTag");

	// World->DebugDrawTraceTag = TraceTag;
	TraceParams.TraceTag = TraceTag;
	DrawDebugLine(GetWorld(), Start, End, FColor::Green, true, 2.0f, 2, 2.0f);

	// Force clear the HitData which contains our results
	HitOut = FHitResult(ForceInit);

	// Perform our trace
	World->LineTraceSingleByChannel
	(
		HitOut,		//result
		Start,	//start
		End, //end
		CollisionChannel, //collision channel
		TraceParams
	);

	// If we hit an actor, return true
	return (HitOut.GetActor() != NULL);
}

//***************************************************************************************************
//** CallMyTrace() - sets up our parameters and then calls our Trace() function
//***************************************************************************************************

void AFIT2097_A2Character::CallMyTrace()
{
	// Get the location of the camera (where we are looking from) and the direction we are looking in
	const FVector Start = this->GetActorLocation();
	const FVector ForwardVector = FollowCamera->GetForwardVector();

	// How far in front of our character do we want our trace to extend?
	// ForwardVector is a unit vector, so we multiply by the desired distance
	const FVector End = Start + ForwardVector * 256;

	// Force clear the HitData which contains our results
	FHitResult HitData(ForceInit);

	// What Actors do we want our trace to Ignore?
	TArray<AActor*> ActorsToIgnore;

	//Ignore the player character - so you don't hit yourself!
	ActorsToIgnore.Add(this);

	// Call our Trace() function with the paramaters we have set up
	// If it Hits anything
	if (Trace(GetWorld(), ActorsToIgnore, Start, End, HitData, ECC_Visibility, false))
	{
		// Process our HitData
		if (HitData.GetActor())
		{

			// UE_LOG(LogClass, Warning, TEXT("This a testing statement. %s"), *HitData.GetActor()->GetName());
			ProcessTraceHit(HitData);
		}
		else
		{
			// The trace did not return an Actor
			// An error has occurred
			// Record a message in the error log
		}
	}
	else
	{
		// We did not hit an Actor
		// ClearPickupInfo();

	}

}

//***************************************************************************************************
//** ProcessTraceHit() - process our Trace Hit result
//***************************************************************************************************

void AFIT2097_A2Character::ProcessTraceHit(FHitResult& HitOut)
{
	// Cast the actor to AInteractable
	AInteractable* const TestPickup = Cast<AInteractable>(HitOut.GetActor());

	if (TestPickup)
	{
		if (TestPickup != NULL && !TestPickup->IsPendingKill() && TestPickup->IsActive())
		{
			// change hasFuse to true if player has a fuse
			if (AFuse* const fusePicked = Cast<AFuse>(TestPickup))
			{
				// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("picked"));

				bHasFuse = true;

				// collect pickup
				TestPickup->InteractedBy(this);
				TestPickup->SetActive(false);
			}

			// used the fuse
			if (AWallFuse* const fuseWallInteract = Cast<AWallFuse>(TestPickup))
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("interacted"));

				if (bHasFuse == true)
				{
					// Interact
					TestPickup->InteractedBy(this);

					bHasFuse = false;
					bFusedUsed = true;
				}
			}

			// opened the door
			if (AFuseDoor* const fuseDoorInteract = Cast<AFuseDoor>(TestPickup))
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("interacted"));

				if (bFusedUsed == true)
				{
					// interact
					TestPickup->InteractedBy(this);

					bOpenFuseDoor = true;
					bFusedUsed = false;
				}
			}

			// remotely accessed a door
			if (ARemoteAccess* const remoteAccessInteract = Cast<ARemoteAccess>(TestPickup))
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("interacted"));

				remoteAccessInteract->InteractedBy(this);

			}
		}
	}
	else
	{
		//UE_LOG(LogClass, Warning, TEXT("TestPickup is NOT a Pickup!"));
		// ClearPickupInfo();
	}
}
