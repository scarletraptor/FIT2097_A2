// Fill out your copyright notice in the Description page of Project Settings.

#include "FIT2097_A2.h"
#include "FuseDoor.h"
#include "Net/UnrealNetwork.h"

AFuseDoor::AFuseDoor()
{
	bReplicateMovement = true;
}


void AFuseDoor::InteractedBy(APawn* Pawn)
{
	Super::InteractedBy(Pawn);

	if (Role == ROLE_Authority)
	{
		// give clients time to play effects and stuff before battery destruction
		// SetLifeSpan(2.0f);
	}
}



