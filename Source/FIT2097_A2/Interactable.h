// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/StaticMeshActor.h"
#include "Interactable.generated.h"

UCLASS()
class FIT2097_A2_API AInteractable : public AStaticMeshActor
{
	GENERATED_BODY()

public:
	// constructor
	AInteractable();

	// required network scaffolding
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// return Interactable state
	UFUNCTION(BlueprintPure, Category = "Interactable")
		bool IsActive();

	// allows other class to set Interactable state
	UFUNCTION(BlueprintCallable, Category = "Interactable")
		void SetActive(bool newActive);

	// call this when the Interactable is collected
	UFUNCTION(BlueprintNativeEvent, Category = "Interactable")
		void WasInteracted();

	virtual void WasInteracted_Implementation();

	// server side handling of being picked up
	UFUNCTION(BlueprintAuthorityOnly, Category = "Interactable")
		virtual void InteractedBy(APawn* Pawn);

protected:
	// true when Interactable can be used and false when the Interactable is not active
	UPROPERTY(ReplicatedUsing = OnRep_IsActive)
		bool bIsActive;

	// called whenever bIsActive is updated
	UFUNCTION()
		virtual void OnRep_IsActive();

	// the pawn that picked up the pick up
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interactable")
		APawn* InteractableInstigator;

private:
	// client side of being picked up
	UFUNCTION(NetMulticast, Unreliable)
		void ClientOnInteractedBy(APawn* Pawn);
};
