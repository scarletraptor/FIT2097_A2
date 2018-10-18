// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Interactable.h"
#include "Fuse.generated.h"

/**
*
*/
UCLASS()
class FIT2097_A2_API AFuse : public AInteractable
{
	GENERATED_BODY()

public:
	//constructor
	AFuse();

	// required for network
	// void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// server side handling of being picked up by the child class
	UFUNCTION(BlueprintAuthorityOnly, Category = "Interact")
		virtual void InteractedBy(APawn* Pawn) override;
};
