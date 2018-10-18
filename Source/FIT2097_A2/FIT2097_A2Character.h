// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameFramework/Character.h"
#include "FIT2097_A2Character.generated.h"

UCLASS(config=Game)
class AFIT2097_A2Character : public ACharacter
{
	GENERATED_BODY()

		/** Camera boom positioning the camera behind the character */
		UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class UCameraComponent* FollowCamera;
public:
	AFIT2097_A2Character();

	// required for networking
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
		float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
		float BaseLookUpRate;

protected:

	/** Resets HMD orientation in VR. */
	void OnResetVR();

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/**
	* Called via input to turn at a given rate.
	* @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	*/
	void TurnAtRate(float Rate);

	/**
	* Called via input to turn look up/down at a given rate.
	* @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	*/
	void LookUpAtRate(float Rate);

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	// Get health
	UFUNCTION(BlueprintPure, Category = "Stats")
		float GetMaxHealth();

	UFUNCTION(BlueprintPure, Category = "Stats")
		float GetCurrentHealth();

	// Get has fuse
	UFUNCTION(BlueprintPure, Category = "Stats")
		bool GetHasFuse();

	// get open door
	UFUNCTION(BlueprintPure, Category = "Stats")
		bool GetOpenFuseDoor();

	/*
	* Call this function to update the player's health
	* can be negative or positive
	*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Stats")
		void UpdateHealth(float DeltaHealth);

	// shut down the pawn and make it rag doll
	UFUNCTION(NetMulticast, Reliable)
		void OnPlayerDeath();

	UFUNCTION(BlueprintPure, Category = "Role")
		FString MyRole();

protected:
	// These are the player's max starting health
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Stats", Meta = (BlueprintProtected = "true"))
		float MaxHealth;

	// entry, called when player presses key to collect pickups
	UFUNCTION(BlueprintCallable, category = "Pickup")
		void Interact();

	// this is called on server to process collection of pick ups
	UFUNCTION(Reliable, Server, WithValidation)
		void ServerInteract();

private:
	// get player's current stats
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth, VisibleAnywhere, Category = "Stats")
		float CurrentHealth;

	// player has a fuse
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth, VisibleAnywhere, Category = "Stats")
		bool bHasFuse;

	// player has used the fuse
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth, VisibleAnywhere, Category = "Stats")
		bool bFusedUsed;

	// player's open door
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth, VisibleAnywhere, Category = "Stats")
		bool bOpenFuseDoor;

	// stats updating on all client
	UFUNCTION()
		void OnRep_CurrentHealth();

	//***************************************************************************************************
	//** Trace functions - used to detect items we are looking at in the world
	//** Adapted from code found on the unreal wiki https://wiki.unrealengine.com/Trace_Functions
	//***************************************************************************************************

	bool Trace(
		UWorld* World,
		TArray<AActor*>& ActorsToIgnore,
		const FVector& Start,
		const FVector& End,
		FHitResult& HitOut,
		ECollisionChannel CollisionChannel,
		bool ReturnPhysMat
	);

	void CallMyTrace();

	void ProcessTraceHit(FHitResult& HitOut);

	//***************************************************************************************************
	//** for pick ups
	//***************************************************************************************************
};
