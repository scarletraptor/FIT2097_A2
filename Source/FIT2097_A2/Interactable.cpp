// Fill out your copyright notice in the Description page of Project Settings.

#include "FIT2097_A2.h"
#include "Interactable.h"
#include "Net/UnrealNetwork.h"

AInteractable::AInteractable()
{
	// replicates actor
	bReplicates = true;

	// static mesh actors disables overlap events by default, we need to re-enable this
	GetStaticMeshComponent()->bGenerateOverlapEvents = true;

	if (Role == ROLE_Authority) // authority guard
	{
		bIsActive = true;
	}
}

void AInteractable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AInteractable, bIsActive);
}

bool AInteractable::IsActive()
{
	return bIsActive;
}

void AInteractable::SetActive(bool newActive)
{
	if (Role == ROLE_Authority)
	{
		bIsActive = newActive;
	}
}

void AInteractable::OnRep_IsActive()
{

}

void AInteractable::WasInteracted_Implementation()
{
	// log a debug message
	UE_LOG(LogClass, Log, TEXT("AInteractable::WasCollected_Implementation %s"), *GetName());
}

void AInteractable::InteractedBy(APawn* Pawn)
{
	if (Role == ROLE_Authority)
	{
		// store the pawn who picked up the Interactable
		InteractableInstigator = Pawn;

		// notify clients of the picked up action
		ClientOnInteractedBy(Pawn);
	}
}

void AInteractable::ClientOnInteractedBy_Implementation(APawn* Pawn)
{
	// store the pawn who picked up the Interactable on the client
	InteractableInstigator = Pawn;

	//fire the blueprint native event, which cannot be replicated
	WasInteracted();
}