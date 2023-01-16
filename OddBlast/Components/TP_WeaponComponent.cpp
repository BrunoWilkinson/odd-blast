// Copyright Epic Games, Inc. All Rights Reserved.


#include "TP_WeaponComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"
#include "../Characters/PlayerCharacter.h"
#include "../Items/Projectile.h"

// Sets default values for this component's properties
UTP_WeaponComponent::UTP_WeaponComponent()
{
	// Default offset from the character location for projectiles to spawn
	MuzzleOffset = FVector(100.0f, 0.0f, 10.0f);
}

void UTP_WeaponComponent::BeginPlay()
{
	FTimerHandle ProjectileTypeIntervalHandle;
	GetWorld()->GetTimerManager().SetTimer(ProjectileTypeIntervalHandle, this, &UTP_WeaponComponent::UpdateProjectileType, ProjectileTypeInterval, true);
}


void UTP_WeaponComponent::Fire()
{
	if(Character == nullptr || Character->GetController() == nullptr)
	{
		return;
	}

	// Try and fire a projectile
	if (ProjectileList.Num() > 0 && CanFire)
	{
		CanFire = false;
		UWorld* const World = GetWorld();
		if (World != nullptr)
		{
			APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
			const FRotator SpawnRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
			// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
			const FVector SpawnLocation = GetOwner()->GetActorLocation() + SpawnRotation.RotateVector(MuzzleOffset);
	
			//Set Spawn Collision Handling Override
			FActorSpawnParameters ActorSpawnParams;
			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
	
			// Spawn the projectile at the muzzle
			for (const FProjectileInfo& Projectile : ProjectileList)
			{
				if (DebugMode && DebugType == Projectile.Type)
				{
					if (Projectile.Class != nullptr)
					{
						World->SpawnActor<AProjectile>(Projectile.Class, SpawnLocation, SpawnRotation, ActorSpawnParams);
						FTimerHandle FireDelayHandle;
						GetWorld()->GetTimerManager().SetTimer(FireDelayHandle, this, &UTP_WeaponComponent::ResetCanFire, FireRate, false);
					}
				}

				if (!DebugMode && CurrentProjectileType == Projectile.Type)
				{
					if (Projectile.Class != nullptr)
					{
						World->SpawnActor<AProjectile>(Projectile.Class, SpawnLocation, SpawnRotation, ActorSpawnParams);
						FTimerHandle FireDelayHandle;
						GetWorld()->GetTimerManager().SetTimer(FireDelayHandle, this, &UTP_WeaponComponent::ResetCanFire, FireRate, false);
					}
				}
			}
		}

		// Try and play the sound if specified
		if (FireSound != nullptr)
		{
			UGameplayStatics::PlaySoundAtLocation(this, FireSound, Character->GetActorLocation());
		}

		// Try and play a firing animation if specified
		if (FireAnimation != nullptr)
		{
			// Get the animation object for the arms mesh
			UAnimInstance* AnimInstance = Character->GetMesh1P()->GetAnimInstance();
			if (AnimInstance != nullptr)
			{
				AnimInstance->Montage_Play(FireAnimation, 1.f);
			}
		}
	}
}

TEnumAsByte<EProjectileType> UTP_WeaponComponent::SetCurrentProjectileType(TEnumAsByte<EProjectileType> Type)
{
	CurrentProjectileType = Type;
	return CurrentProjectileType;
}

void UTP_WeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if(Character != nullptr)
	{
		// Unregister from the OnUseItem Event
		Character->OnUseItem.RemoveDynamic(this, &UTP_WeaponComponent::Fire);
	}
}

void UTP_WeaponComponent::UpdateProjectileType()
{
	const UEnum* EnumProjectileType = FindObject<UEnum>(ANY_PACKAGE, TEXT("EProjectileType"), true);
	int64 Min = 0;

	TEnumAsByte<EProjectileType> NextType = EProjectileType(FMath::RandRange(Min, EnumProjectileType->GetMaxEnumValue() - 1));
	while (CurrentProjectileType == NextType)
	{
		NextType = EProjectileType(FMath::RandRange(Min, EnumProjectileType->GetMaxEnumValue() - 1));
	}
	CurrentProjectileType = NextType;
}

void UTP_WeaponComponent::ResetCanFire()
{
	CanFire = true;
}

void UTP_WeaponComponent::AttachWeapon(APlayerCharacter* TargetCharacter)
{
	Character = TargetCharacter;
	if(Character != nullptr)
	{
		// Attach the weapon to the First Person Character
		FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
		GetOwner()->AttachToComponent(Character->GetMesh1P(),AttachmentRules, FName(TEXT("GripPoint")));

		// Register so that Fire is called every time the character tries to use the item being held
		Character->OnUseItem.AddDynamic(this, &UTP_WeaponComponent::Fire);
	}
}

